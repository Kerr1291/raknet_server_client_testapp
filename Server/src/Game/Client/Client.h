#pragma once
#include <RakNetTypes.h>
#include <string>

///Should be used by ClientInterface (CI) only
int CI_InitConnectToServer(int i0, int i1, int i2, int i3, int port, int client_port, int client_id);

int CI_DisconnectClient(int client_id);

int CI_ClientIsConnected(int client_id);

int CI_SendPacket(int raw_message_id, int game_message_id, char* packet_data, int bytes, int client_id);

int CI_RecievePacket(int* raw_message_id, int* game_message_id, char** packet_data, int* bytes, int client_id);

int CI_FreeLastPacket(int client_id);

int CI_AllocateNewClient();

namespace RakNet {
class RakPeerInterface;
}

namespace RClient
{

class Client;

Client& GetClient(int& out_client_index);

struct Client
{
private:
  friend Client& GetClient(int& out_client_index);
  Client();
public:  
  ~Client();
  
  void SetClientPort(int port);
  
  int  GetClientPort();
  
  void Connect(const std::string& server_ip, int port);
  
  void Disconnect();
  
  bool Connected();
  
  int GetPacketFromServer(int* raw_message_id, int* game_message_id, char** packet_data, int* bytes);
  
  int SendPacketToServer(int raw_message_id, int game_message_id, char* packet_data, int bytes);
  
  void FreeLastRecievedPacket();
  
  
private:
  
  RakNet::RakPeerInterface* Get();
  
private:
  
  RakNet::RakPeerInterface* _client;
  
  RakNet::Packet* _last_recieved_packet;
  
  RakNet::SystemAddress _server_addr;
  
  int           _client_port;
  
  std::string   _server_ip;
  int           _server_port;
  
  bool          _connected;
  bool          _disconnect_me;
  bool          _check_connect;
};









}