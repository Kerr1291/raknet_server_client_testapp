#include <Client.h> 
#include <ClientInterface.h>

#include <Utils.h>
#include <helpers.h>

#include <RakPeerInterface.h>

#include <MessageIdentifiers.h>

#include <DefaultMessageTypes.h>
#include <GameMessageType.h>

#include <BitStream.h>

#include <RakThread.h>
#include <RakSleep.h>

#include <vector>



namespace RClient
{
  std::vector<Client>& GetClientList()
  {
    static std::vector<Client> clients;
    return clients;
  }
  
  int GetNumClients()
  {
    return GetClientList().size();
  }
  
  Client& GetClient(int& out_client_index)
  {
    RUtils::Clamp(out_client_index, 0, GetNumClients());
    
    if( out_client_index >= GetNumClients() )
    {
      GetClientList().push_back( Client() );
    }
    return GetClientList()[ out_client_index ];
  }
  
  bool PortUsedByOtherClient(int port, int client_id)
  {
    for(int i = 0; i < GetNumClients(); ++i)
    {
      if( i == client_id )
        continue;
      
      if( GetClient(i).GetClientPort() == port )
        return true;
    }
    return false;
  }
  
  bool HasClientID(int client_id)
  {
    return ( client_id < RClient::GetNumClients() );
  }
  
}

///Should be used by ClientInterface (CI) only
int CI_AllocateNewClient()
{
  int new_id = RClient::GetNumClients();
  RClient::GetClient(new_id);
  return new_id;
}

int CI_InitConnectToServer(int i0, int i1, int i2, int i3, int port, int client_port, int client_id)
{
  if( client_id >= RClient::GetNumClients() )
  {
    RClientSetErrorState( RCLIENT_BAD_CLIENT_ID );
    return 0;
  }
  
  if( RClient::PortUsedByOtherClient( client_port, client_id ) )
  {
    RClientSetErrorState( RCLIENT_CONNECTION_FAILED_PORT_IN_USE );
    return 0;
  }
  
  RClientSetErrorState( RCLIENT_CONNECTING );
  
  RClient::Client& client = RClient::GetClient( client_id );
  client.SetClientPort( client_port );
  
  std::stringstream ip_builder;
  ip_builder << i0 <<"." << i1 <<"." << i2 <<"." << i3;
  
  std::string server_ip_string = ip_builder.str();
  
  client.Connect( server_ip_string, port );
  return 1;
}

int CI_DisconnectClient(int client_id)
{
  if( client_id >= RClient::GetNumClients() )
    return 0;
  RClient::Client& client = RClient::GetClient( client_id );
  client.Disconnect();
  RClientSetErrorState( RCLIENT_DISCONNECTED );
  return 1;
}

int CI_ClientIsConnected(int client_id)
{
  if( client_id >= RClient::GetNumClients() )
    return 0;
  
  RClient::Client& client = RClient::GetClient( client_id );
  if( client.Connected() )
    return 1;
  return 0;
}

int CI_SendPacket(int raw_message_id, int game_message_id, char* packet, int bytes, int client_id)
{
  if( client_id >= RClient::GetNumClients() )
    return 0;
  
  RClient::Client& client = RClient::GetClient( client_id );
  return client.SendPacketToServer(raw_message_id, game_message_id, packet, bytes);
}

int CI_RecievePacket(int* raw_message_id, int* game_message_id, char** packet, int* bytes, int client_id)
{
  if( client_id >= RClient::GetNumClients() )
    return 0;
  
  RClient::Client& client = RClient::GetClient( client_id );
  return client.GetPacketFromServer(raw_message_id, game_message_id, packet, bytes);
}

int CI_FreeLastPacket(int client_id)
{
  if( client_id >= RClient::GetNumClients() )
    return 0;
  
  RClient::Client& client = RClient::GetClient( client_id );
  client.FreeLastRecievedPacket();
  return 1;
}

namespace RClient
{
  
  
  Client::Client()
  :_client(0)
  ,_last_recieved_packet(0)
  ,_client_port(25505)
  ,_server_ip("127.0.0.1")
  ,_server_port(25505)
  ,_connected(false)
  ,_disconnect_me(false)
  ,_check_connect(true)
  {
  }
  
  Client::~Client()
  {
    Disconnect();
  }
  
  void Client::SetClientPort(int port)
  {
    _client_port = port;
  }
  
  int  Client::GetClientPort()
  {
    return _client_port;
  }
  
  void Client::Connect(const std::string& server_ip, int port)
  {
    _client = RakNet::RakPeerInterface::GetInstance();
    
    _server_ip   = server_ip;
    _server_port = port;
    
    RakNet::SocketDescriptor sdesc( _client_port, 0 );
    
    //mark this for raknet as a pure client
    Get()->SetMaximumIncomingConnections(0);
          
    RakNet::StartupResult startup_result = Get()->Startup( 1, &sdesc, 1 );  
    
    RakSleep( 500 );
    
    if( startup_result != RakNet::RAKNET_STARTED )
    {
      KC_LABEL("Failed to start client. Reason: ", k_startup_result_string[ startup_result ]);
      
      if( startup_result == 5 )
      {
        RClientSetErrorState( RCLIENT_CONNECTION_FAILED_PORT_IN_USE );
      }
      else
      {
        RClientSetErrorState( RCLIENT_RAKNET_FAILED_START );
      }
      return;
    }
    else
    {
      KC_LABEL("RakNet started for client on port: ", port);
    }
    
    RakNet::ConnectionAttemptResult connection_result = Get()->Connect( _server_ip.c_str(), _server_port, 0, 0);
    
    KC_LABEL("Attempting to connect to server. Initial result: ", k_connection_result_string[ connection_result ]);
    
    _check_connect = true;
    
    if( connection_result != RakNet::CONNECTION_ATTEMPT_STARTED )
    {
      RClientSetErrorState( RCLIENT_CONNECTION_FAILED_UNKNOWN );
    }
  }
  
  void Client::Disconnect()
  {
    if( !Connected() )
      return;
      
    RakNet::SystemAddress server_addr( _server_ip.c_str(), _server_port );
    Get()->CloseConnection(server_addr, true );
    
    RClientSetErrorState( RCLIENT_DISCONNECTED );
    
    _check_connect = true;
    _connected = false;
  }
  
  bool Client::Connected()
  {
    if(!Get())
      return false;
    
    if( _check_connect )
    {
      RakNet::SystemAddress server_addr( _server_ip.c_str(), _server_port );
      
      int connection_result = Get()->GetConnectionState(server_addr);
      _connected = ( connection_result == RakNet::IS_CONNECTED );
      
      KC_LABEL("Connection result: ", k_connection_state_result_string[ connection_result ]);

      if( _connected )
      {
        _server_addr = RakNet::SystemAddress( _server_ip.c_str(), _server_port );
        
        KC_LABEL("Client is connected to: ",_server_ip);
        KC_LABEL("\t\t...on port: ",_server_port);
        
        _check_connect = false;
        
        RClientSetErrorState( RCLIENT_CONNECTED );
      }
    }
    return _connected;
  }
  
  int Client::GetPacketFromServer(int* raw_message_id, int* game_message_id, char** packet_data, int* bytes)
  {
    if( !Connected() )
      return 0;
    
    FreeLastRecievedPacket();
    
    _last_recieved_packet = Get()->Receive();
    
    if( !_last_recieved_packet )
      return 0;
    
    *raw_message_id = (int)_last_recieved_packet->data[0];
    
    //read in the data
    RakNet::BitStream bsIn(_last_recieved_packet->data,_last_recieved_packet->length,false);
    bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
    
    if( *raw_message_id < ID_USER_PACKET_ENUM )
    {
      KC_LABEL("Client Recieved RakNet Packet: ", k_raknet_packet_id_strings[*raw_message_id]);
      *game_message_id = -1;
      *packet_data = 0;
    }   
    else//( *raw_message_id >= ID_USER_PACKET_ENUM )
    {    
      bsIn.Read( *game_message_id );
      bsIn.Read( *bytes );
      KC_LABEL("Client Recieved Game Packet with Game ID: ", *game_message_id);
      
      KC_PRINT( *bytes );
  
      if( *bytes > 0 )
      {        
        *packet_data = (char*)(&_last_recieved_packet->data[ sizeof(RakNet::MessageID) + sizeof(int)*2 ]);
      }
      else
      {
        *packet_data = 0;
      }
    }
    
    return 1;
  }
  
  int Client::SendPacketToServer(int raw_message_id, int game_message_id, char* packet_data, int bytes)
  {
    if( !Connected() )
      return 0;

    RakNet::BitStream b_stream_data;
    b_stream_data.Write((RakNet::MessageID)(raw_message_id));
    if( game_message_id >= 0 )
    {
      b_stream_data.Write((int)(game_message_id));
      b_stream_data.Write((int)(bytes));
    }
    b_stream_data.Serialize(true, packet_data, (unsigned)bytes);
    Get()->Send(&b_stream_data,HIGH_PRIORITY,RELIABLE_ORDERED,0,_server_addr,false);
    return 1;
  }
  
  void Client::FreeLastRecievedPacket()
  {
    if( _last_recieved_packet )
      Get()->DeallocatePacket(_last_recieved_packet);
    _last_recieved_packet = 0;
  }
  
  RakNet::RakPeerInterface* Client::Get()
  {
    return _client;
  }


}