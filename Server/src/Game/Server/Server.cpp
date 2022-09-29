#include <Server.h>

#include <helpers.h>

#include <RakPeerInterface.h>

#include <MessageIdentifiers.h>
#include <DefaultMessageTypes.h>
#include <GameMessageType.h>
#include <BitStream.h>

#include <RakThread.h>
#include <RakSleep.h>

#include <GameMain.h>
#include <RProto.pb.h>

namespace RServer
{
  
///Hard coded server settings/limits
const int k_max_possible_clients = 64;

const int k_default_max_clients = 12;
const int k_default_listen_port = 25505;
  
const int k_num_error_codes = 2;

//every 10 seconds, check to see if the game stopped running for a bad reason
const int k_watchdog_rate = 10000;


const char* k_error_codes[k_num_error_codes] =
{
  "No Errors."
  ,"Server failed to start."
};

namespace R_ERR
{
  const int NONE = 0;
  const int FAILED_START = 1;
};
  
///Only way to access the server  
Server& GetServer();
  
///Game thread, this runs all game logic
RAK_THREAD_DECLARATION( RunServer )
{
  KC_MSG("Starting game server.\r\n");
  int result = RGame::InitGame();
  if( result != 0 )
    KC_LABEL("Game failed to initialize with error: ", RGame::GetGameError());
  
  GetServer().SetGameServerRunning(true);
  
  //run until server is in shutdown state
  while( GetServer().GetListenState() )
  {
    //TODO: calculate dt?
    double dt = 0.016f;
    
    if( !RGame::NetworkTrafficLocked() )
      GetServer().ProcessNetworkPackets();
    
    int run_result = RGame::GameMain( dt );
    if( run_result >= 0 )
      break;
  }
  
  if( result > 0 )
    KC_LABEL("Game crashed with error: ", RGame::GetGameError());
  else //result == 0
    SI_Shutdown_Server(0,0);
  GetServer().SetGameServerRunning(false);
  return 0;
}

///This thread watches the game once started
///If the game crashes, it attempts to reboot it
RAK_THREAD_DECLARATION( ServerWatchdog )
{
  KC_MSG("Watchdog thread loaded.\r\n");
  bool server_was_started = false;
    
  while( GetServer().GetListenState() )
  {
    GetServer().SetGameWatchdogRunning(true);
    
    bool server_state = GetServer().GetGameServerRunning();
    
    if( !server_was_started && server_state )
    {
      KC_MSG("Game started. Starting watchdog.\r\n");
      while( GetServer().GetGameServerRunning() )
      {
        //watch the game
        RakSleep( k_watchdog_rate );
      }
      
      //is this an expected shutdown?
      if( GetServer().GetGameServerShutdown() )
      {
        KC_MSG("Watchdog reports server is shutting down.\r\n");
        //then go ahead and exit
        GetServer().SetGameWatchdogRunning(false);
        return 0;
      }
      else
      {
        server_state = false;
        
        KC_WARNING("Watching reports game has crashed. Attempting to recover.");
        
        //this was unexpected, try to restart the game
        GetServer().StartGameServer();
        
        //TODO: check for failed recovery and force shutdown if true
      }
    }
    
    server_was_started = server_state;
    
    //server is started, but the game isn't yet
    RakSleep( k_watchdog_rate );
  }
  
  //this code path should never be reached
  KC_ERROR("Watchdog reports server crashed without starting game somehow.");
  
  GetServer().SetGameWatchdogRunning(false);
  //run until server is in shutdown state
  return 0;
}



Server::Server()
:_rserver(0)
,_game_server_running(false)
,_game_server_shutdown(false)
,_game_watchdog_running(false)
,_port(k_default_listen_port)
,_max_clients(k_default_max_clients)
,_last_error_code(0)
{
  InitDefaultCallbackMap();
}

Server::~Server()
{
  KillServer();
}

bool Server::HasError()
{
  return ( _last_error_code > 0 );
}

const char* Server::GetLastError()
{
  if(_last_error_code == R_ERR::FAILED_START)
    return k_startup_result_string[ _last_error_code_why ];
  else
    return k_error_codes[ _last_error_code ];
}

void Server::SetMaxClients(int max_clients)
{
  if( _max_clients < 1 )
    _max_clients = 1;
  
  if( max_clients > k_max_possible_clients )
    max_clients = k_max_possible_clients;
  
  _max_clients = max_clients;
  
  if( GetServerLoaded() )
    Get()->SetMaximumIncomingConnections( _max_clients );
}

int  Server::GetMaxClients()
{
  return _max_clients;
}

int  Server::GetAvailableConnections()
{
  if( !GetListenState() )
    return 0;
  
  return Get()->NumberOfConnections();
}

int  Server::GetNumConnectedClients()
{
  if( !GetListenState() )
    return 0;
  unsigned short num_connections = 0;
  Get()->GetConnectionList(0,&num_connections);
  return num_connections;
}

void Server::SetListenPort(int port)
{
  if( GetListenState() )
    return;
  _port = port;
  if( !GetServerLoaded() )
    return;
}

int  Server::GetListenPort()
{
  return _port;
}

bool Server::GetServerLoaded()
{
  return ( Get() != 0 );
}

bool Server::GetListenState()
{
  if( !GetServerLoaded() )
    return false;
  return Get()->IsActive();
}

void Server::SetGameServerRunning(bool running)
{
  _game_server_running = running;
}

bool Server::GetGameServerRunning()
{
  return _game_server_running;
}

bool Server::GetGameServerShutdown()
{
  return _game_server_shutdown;
}

void Server::SetGameWatchdogRunning(bool running)
{
  _game_watchdog_running = running;
}

bool Server::GetGameWatchdogRunning()
{
  return _game_watchdog_running;
}

void Server::DisableNewConnections()
{
  if( GetServerLoaded() )
    Get()->SetMaximumIncomingConnections( 0 );
}

void Server::EnableNewConnections()
{
  if( GetServerLoaded() )
    Get()->SetMaximumIncomingConnections( _max_clients );
}

void Server::PurgeOldPackets()
{
  if( !Get() )
    return;
  
  //purge all pending packets
  RakNet::Packet* p = Get()->Receive();
  while( p )
  {
    if( !p )
      continue;
    
    //free packet and get next
    Get()->DeallocatePacket(p);
    p = Get()->Receive();
  }
}


RakNet::RakPeerInterface* Server::Get()
{
  return _rserver;
}

void Server::Init()
{ 
  if( GetServerLoaded() )
    return;
  _game_server_shutdown = false;
  
  //get the raknet interface
  _rserver = RakNet::RakPeerInterface::GetInstance();
  
  //setup server
  StartConnectionListener();
  
  if( !GetListenState() )
  {
    KC_MSG("Server failed to start, deallocating server resources.\r\n");
    KillServer();
    return;
  }
  
  StartGameServer();
}

void Server::InitDedicated()
{
  if( GetServerLoaded() )
    return;
  
  //TODO: maybe do something different here one we figure out how the server interface works
  Init();
  
  /*
  //setup dedicated server
  StartConnectionListener();
  
  //spawn a thread for this
  RunServer(*this);
  */
}

void Server::KillClients()
{
  //disconnect all clients
  if( GetListenState() )
  {
    Get()->Shutdown( 3000 );
  }
}

void Server::KillClient()
{
  //disconnect given client
}

void Server::KillServer()
{
  if( _game_server_shutdown )
    return;
  
  _game_server_shutdown = true;
  KillClients();
  
  //perform shutdown
  if( GetServerLoaded() )
  {
    RakNet::RakPeerInterface::DestroyInstance(_rserver);
    _rserver = 0;
  }
}

void Server::StartConnectionListener()
{
  RakNet::SocketDescriptor sdesc( GetListenPort(), 0 );
  
  KC_LABEL( "Starting server on port: ", GetListenPort() );
  
  //start server
  RakNet::StartupResult result = Get()->Startup( GetMaxClients(), &sdesc, 1); 

  if( result != RakNet::RAKNET_STARTED)
  {
    KC_LABEL( "Failed to start server: ", k_startup_result_string[(int)result] );
    
    _last_error_code = R_ERR::FAILED_START;
    _last_error_code_why = (int)result;
    
    return;
  }
  
  RakSleep(500);
  
  Get()->SetMaximumIncomingConnections( GetMaxClients() );
}

void Server::StartGameServer()
{
  if( GetGameServerRunning() )
    return;
  
  //Start the game
  RakNet::RakThread::Create(&RunServer, 0);
  
  //Load the watchdog if it's not running
  if( !GetGameWatchdogRunning() )
  {
    RakNet::RakThread::Create(&ServerWatchdog, 0);
  }
}

//NOTE: This function is called from the game thread, it should never cause a performance
//      hit to the game client so never call this from the main thread.
void Server::ProcessNetworkPackets()
{  
  if( !Get() )
    return;
  
  //process all pending packets
  RakNet::Packet* p = Get()->Receive();
  while( p )
  {
    if( !p )
      continue;
    
    int message_id = (int)p->data[0];
    
    if( message_id < ID_USER_PACKET_ENUM )
      ProcessRakNetPacket(p);
    else
      ProcessGamePacket(p);
    
    //free packet and get next
    Get()->DeallocatePacket(p);
    p = Get()->Receive();
  }
}


//TODO: move the packet processing to a new .cpp file maybe? better organization

/// ///////////////////////////////////////////////////////////////////
//Used internally to respond to packets
struct PacketIDCallback
{
  PacketIDCallback()
  :_id(-1)
  ,_callback(0){}
  int _id;
  void (*_callback)(RakNet::Packet* p);
};
/// ///////////////////////////////////////////////////////////////////

/// ///////////////////////////////////////////////////////////////////
// Begin default packet processing
///

PacketIDCallback& GetDefaultCallbackStruct(int id)
{
  static PacketIDCallback callback_map[ID_USER_PACKET_ENUM];
  return callback_map[id];
}

#define RINIT_CB( id, callback_function ) \
  GetDefaultCallbackStruct( id )._id = id; \
  GetDefaultCallbackStruct( id )._callback = RServer::Server::callback_function; 

void Server::InitDefaultCallbackMap()
{
  RINIT_CB( ID_NEW_INCOMING_CONNECTION, NotifyNewConnection )
}

#undef RINIT_CB

void Server::ProcessRakNetPacket(RakNet::Packet* packet)
{
  if(!packet)
    return;
  
  int message_id = (int)packet->data[0];
  
  KC_LABEL("Server Recieved Packet: ",k_raknet_packet_id_strings[message_id]);
  
  PacketIDCallback& cb = GetDefaultCallbackStruct( message_id );
  if( cb._id < 0 )
  {
    KC_LABEL("No callback set for message id: ", k_raknet_packet_id_strings[message_id]);
  }
  else
  {
    //do callback
    cb._callback(packet);
  }
}

void Server::NotifyNewConnection(RakNet::Packet* p_connection_info)
{
  KC_MSG("A client has connected to the server.\r\n");
  DataStructures::List<RakNet::SystemAddress> connected_systems;
  DataStructures::List<RakNet::RakNetGUID>    connected_guids;
  
  GetServer().Get()->GetSystemList(connected_systems,connected_guids);
  
  int id = -1;
  for( unsigned i = 0; i < connected_systems.Size(); ++i )
  {
    if( connected_systems[i].EqualsExcludingPort( p_connection_info->systemAddress ) )
    {
      id = i;
      break;
    }
  }
  
  if( id == -1 )
    return;
  
  //request player info from the newly connected client
  rproto::SRequest request;
  request.set_request_id( rproto::SRequest::r_pinfo );
  
  RSERVER_SEND_MSG( rproto::msg_srequest, request, id ); 
}

///
// End default packet processing
/// ///////////////////////////////////////////////////////////////////







/// ///////////////////////////////////////////////////////////////////
// Begin game packet processing
///
RMessage::GameCallbacks& GetGameCallbacks()
{
  static RMessage::GameCallbacks game_callbacks;
  return game_callbacks;
}


void Server::ProcessGamePacket(RakNet::Packet* packet)
{
  const std::string& desc = GetGameCallbacks().GetCallbackDesc( packet );
  
  if( !desc.empty() )
  {
    KC_LABEL("Game Callback: ", desc);
  }
  
  GetGameCallbacks().RunCallback( packet );
}
///
// End game packet processing
/// ///////////////////////////////////////////////////////////////////

int  Server::GetClientID(const std::string& ip)
{
  DataStructures::List<RakNet::SystemAddress> connected_systems;
  DataStructures::List<RakNet::RakNetGUID>    connected_guids;
  
  Get()->GetSystemList(connected_systems,connected_guids);
  
  RakNet::SystemAddress to_find(ip.c_str());
  
  for( unsigned i = 0; i < connected_systems.Size(); ++i )
  {
    if( connected_systems[i].EqualsExcludingPort( to_find ) )
      return i;
  }
  return -1;
}

int  Server::GetClientID(int ip0, int ip1, int ip2, int ip3)
{
  std::stringstream to_ip_str;
  to_ip_str << ip0 << "." << ip1 << "." << ip2 << "." << ip3;
  std::string ip_str = to_ip_str.str();
  return GetClientID(ip_str);
}

//the game should use this to add callbacks
void Server::SetGameCallback(int game_message_id, void (*callback)(RakNet::Packet* p), const std::string& desc)
{
  GetGameCallbacks().SetCallback( game_message_id, callback, desc );
}

//the game should use this to send data over the network to one client
void Server::SendGameData(int game_message_id, int dest_id, void* data, unsigned bytes)
{  
  DataStructures::List<RakNet::SystemAddress> connected_systems;
  DataStructures::List<RakNet::RakNetGUID>    connected_guids;
  
  int raknet_message_id = RMessage::GetRakNetIndex( RMessage::k_special_game_message_constant );
  
  Get()->GetSystemList(connected_systems,connected_guids);
  
  if( (unsigned)dest_id >= connected_systems.Size() )
    return;
  
  KC_LABEL("Sending data of game id: ", game_message_id);
  
  RakNet::BitStream b_stream_data;
  b_stream_data.Write((RakNet::MessageID)(raknet_message_id));
  b_stream_data.Write((int)(game_message_id));
  b_stream_data.Write((int)(bytes));
  b_stream_data.Serialize(true, (char*)data, bytes);
  Get()->Send(&b_stream_data,HIGH_PRIORITY,RELIABLE_ORDERED,0,connected_systems[dest_id],false);
}

void Server::SendGameData(int game_message_id, const std::string& ip, void* data, unsigned bytes)
{
  int id = GetClientID(ip);
  if( id < 0 )
    return;
  SendGameData(game_message_id,id,data,bytes);
}


//this will send game data to everyone else
void Server::BroadcastGameData(int game_message_id, void* data, unsigned bytes)
{  
  DataStructures::List<RakNet::SystemAddress> connected_systems;
  DataStructures::List<RakNet::RakNetGUID>    connected_guids;
  
  int raknet_message_id = RMessage::GetRakNetIndex( RMessage::k_special_game_message_constant  );
  
  Get()->GetSystemList(connected_systems,connected_guids);

  RakNet::BitStream b_stream_data;
  b_stream_data.Write((RakNet::MessageID)(raknet_message_id));
  b_stream_data.Write((int)(game_message_id));
  b_stream_data.Write((int)(bytes));
  b_stream_data.Serialize(true, (char*)data, bytes);
  
  KC_LABEL("Broadcasting data of game id: ", game_message_id);
    
  for( unsigned i = 0; i < connected_systems.Size(); ++i )
  {
    Get()->Send(&b_stream_data,HIGH_PRIORITY,RELIABLE_ORDERED,0,connected_systems[i],false);
  }
}




//Only access through this
Server& GetServer()
{
  static Server server;
  return server;
}

}///end RServer


void SI_Shutdown_Server(int param_id, void* data)
{
  RServer::GetServer().KillServer();
}

void SI_Start_Server(int param_id, void* data)
{
  RServer::GetServer().Init();
}

void SI_Start_Dedicated_Server(int param_id, void* data)
{
  RServer::GetServer().InitDedicated();
}

void SI_Get_Server_Port(int param_id, void* data)
{
  if(!data)
    return;
  int* out_port = (int*)data;
  *out_port = RServer::GetServer().GetListenPort();
}

void SI_Set_Server_Port(int param_id, void* data)
{
  if(!data)
    return;
  int* in_port = (int*)data;
  RServer::GetServer().SetListenPort( *in_port );
}

void SI_Get_Server_Ready(int param_id, void* data)
{
  if(!data)
    return;
  int* out_is_ready = (int*)data;

  if (RServer::GetServer().GetGameServerRunning())
    *out_is_ready = 1;
  else
    *out_is_ready = 0;
}

void SI_Has_Error(int param_id, void* data)
{
  if(!data)
    return;
  
  int* out_has_error = (int*)data;
  *out_has_error = RServer::GetServer()._last_error_code;
}

void SI_Get_Error(int param_id, void* data)
{
  if(!data)
    return;
  
  const char** out_error_string = (const char**)data;
  *out_error_string = RServer::GetServer().GetLastError();
}


void SI_Process_Parameter(int param_id, void* data)
{
  if( param_id == 0 )
  {
    SI_Shutdown_Server(0,0);
  }
  else if( param_id == 1 )
  {
    SI_Start_Server(param_id,data);
  }
  else if( param_id == 2 )
  {
    SI_Start_Dedicated_Server(param_id,data);
  }
  else if( param_id == 3 )
  {
    SI_Get_Server_Port(param_id,data);
  }
  else if( param_id == 4 )
  {
    SI_Set_Server_Port(param_id,data);
  }
  else if( param_id == 5 )
  {
    SI_Get_Server_Ready(param_id,data);
  }
  else if( param_id == 6 )
  {
    SI_Has_Error(param_id,data);
  }
  else if( param_id == 7 )
  {
    SI_Get_Error(param_id,data);
  }
  else if( param_id == 8 )
  {
    int* int_data = (int*)data;
    *int_data = RServer::GetServer().GetMaxClients();
  }
  else if( param_id == 9 )
  {
    int* int_data = (int*)data;
    RServer::GetServer().SetMaxClients( *int_data );
  }
  else if( param_id == 10 )
  {
    int* int_data = (int*)data;
    *int_data = RServer::GetServer().GetAvailableConnections();
  }
  else if( param_id == 11 )
  {
    int* int_data = (int*)data;
    *int_data = RServer::GetServer().GetNumConnectedClients();
  }
  else{}
  
}



