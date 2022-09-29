#pragma once
#include <string>
#include <vector>

namespace RakNet {
  struct Packet;
}

namespace RMessage
{

//used to signal to raknet that our message is a special user-defined game message
const int k_special_game_message_constant = 2;
  
typedef  void (*packet_callback)(RakNet::Packet* p);

int GetRakNetIndex(int real_id);

///Use these to extract data from a packet
int GetGameID(RakNet::Packet* p);
int GetMessageDataBytes(RakNet::Packet* p);
char* GetMessageData(RakNet::Packet* p);

class GameCallbacks
{
public:
  GameCallbacks();
  
  ~GameCallbacks();
  
  //0 indexed id, NOT one pulled from a packet
  void SetCallback(int real_id, void (*callback)(RakNet::Packet* p), const std::string& desc="");
  
  const std::string& GetCallbackDesc(int real_id);
  const std::string& GetCallbackDesc(RakNet::Packet* p);
  
  packet_callback GetCallback(int real_id);
  packet_callback GetCallback(RakNet::Packet* p);
  
  void RunCallback(int real_id, RakNet::Packet* p);
  void RunCallback(RakNet::Packet* p);
  
private:
  
  void IncreaseCallbackStorage(int min_increase);
  
  std::vector<packet_callback> _callbacks;
  std::vector<std::string>     _callback_desc;
};

}



#define GAME_MESSAGE__BEGIN__CALLBACK( function_name, message_type, game_var, msg_var ) \
void  function_name(RakNet::Packet* p) { \
  RGame::GameState& game_var = RGame::GetGameState(); \
  int bytes = RMessage::GetMessageDataBytes(p); \
  char* raw_data_ptr = RMessage::GetMessageData(p); \
  message_type msg_var;     \
  if( bytes > 0 ) {    \
    msg_var.ParseFromArray(raw_data_ptr, bytes); \
  }






#define GAME_MESSAGE__END____CALLBACK }