#pragma once
#include <string>

///Used by ServerInterface (SI) only
void SI_Shutdown_Server(int param_id, void* data);
void SI_Start_Server(int param_id, void* data);
void SI_Start_Dedicated_Server(int param_id, void* data);
void SI_Process_Parameter(int param_id, void* data);
void SI_Get_Server_Port(int param_id, void* data);
void SI_Set_Server_Port(int param_id, void* data);
void SI_Get_Server_Ready(int param_id, void* data);
void SI_Has_Error(int param_id, void* data);
void SI_Get_Error(int param_id, void* data);

namespace RakNet {
class RakPeerInterface;
struct Packet;
}

namespace RServer
{
  class Server;
  Server& GetServer();

class Server
{
  friend Server& GetServer();
  friend void ::SI_Shutdown_Server(int param_id, void* data);
  friend void ::SI_Start_Server(int param_id, void* data);
  friend void ::SI_Start_Dedicated_Server(int param_id, void* data);
  friend void ::SI_Get_Server_Port(int param_id, void* data);
  friend void ::SI_Set_Server_Port(int param_id, void* data);
  friend void ::SI_Get_Server_Ready(int param_id, void* data);
  friend void ::SI_Has_Error(int param_id, void* data);
  friend void ::SI_Get_Error(int param_id, void* data);
  friend void ::SI_Process_Parameter(int param_id, void* data);
  
  private:
    
    Server();
    ~Server();
    
    bool HasError();
    const char* GetLastError();
    
    void SetMaxClients(int max_clients);
    int  GetMaxClients();
    
    int  GetAvailableConnections();
    
    int  GetNumConnectedClients();
    
    void SetListenPort(int port);
    int  GetListenPort();
    
  public:
    
    //used to query if we have a valid RakNet instance loaded
    bool GetServerLoaded();
    
    //used to query if the RakNet server started and is listening for connections
    bool GetListenState();
    
    //used by game thread to notify the server that the game is ready
    void SetGameServerRunning(bool running);
    bool GetGameServerRunning();
    
    //only true for a brief period while the server shuts down
    bool GetGameServerShutdown();
    
    //used by watchdog thread to notify the server that the thread is running
    void SetGameWatchdogRunning(bool running);
    bool GetGameWatchdogRunning();
    
    //prevents new clients from joining
    void DisableNewConnections();
    void EnableNewConnections();
    
    //use this to clear out any packets that were waiting to be processed
    //without actually processing them
    void PurgeOldPackets();
    
  private:
    
    RakNet::RakPeerInterface* Get();
    
    void Init();
    void InitDedicated();

    void KillClients();
    void KillClient();
    void KillServer();
    
    void StartConnectionListener();
    
  public:
    
    //used by the watchdog thread only to start the game, do not call this directly
    void StartGameServer();
    
    //called from main game thread only, processes all incomming network traffic to the server
    void ProcessNetworkPackets();
    
  private:
    
    //call once from ctor only
    void InitDefaultCallbackMap();
    
    void ProcessRakNetPacket(RakNet::Packet* packet);
    
    static void NotifyNewConnection(RakNet::Packet* p_connection_info);
    
    void ProcessGamePacket(RakNet::Packet* packet);
    
  public:
    
    int  GetClientID(const std::string& ip);
    int  GetClientID(int ip0, int ip1, int ip2, int ip3);
    
    //the game should use this to add callbacks, start with index 0 for ids
    void SetGameCallback(int game_message_id, void (*callback)(RakNet::Packet* p), const std::string& desc="");
    
    //the game should use this to send data over the network to one client
    void SendGameData(int game_message_id, int dest_id, void* data, unsigned bytes);
    
    //just another way
    void SendGameData(int game_message_id, const std::string& ip, void* data, unsigned bytes);
    
    //this will send game data to everyone else
    void BroadcastGameData(int game_message_id, void* data, unsigned bytes);
  
  private:
    
    RakNet::RakPeerInterface* _rserver;
    
    
    bool _game_server_running;
    bool _game_server_shutdown;
    bool _game_watchdog_running;
    
    int _port;
    
    int _max_clients;
    
    int _last_error_code;
    int _last_error_code_why;
};


}


#define RSERVER_BROADCAST_MSG( message_id, message ) do { \
std::string message_data; \
message.SerializeToString( &message_data ); \
RServer::GetServer().BroadcastGameData( message_id, const_cast<char*>(message_data.c_str()), message_data.size() ); \
}while(0)

#define RSERVER_CLIENT_ID( client_ip ) RServer::GetServer().GetClientID( client_ip )

#define RSERVER_SEND_MSG( message_id, message, client ) do { \
std::string message_data; \
message.SerializeToString( &message_data ); \
RServer::GetServer().SendGameData( message_id, client, const_cast<char*>(message_data.c_str()), message_data.size() ); \
}while(0)































