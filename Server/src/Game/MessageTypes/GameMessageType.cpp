#include <GameMessageType.h>

#include <RakNetTypes.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

#include <algorithm>

namespace RMessage
{
  const std::string k_bad_cb_index = "Bad callback index.";
  const std::string k_null_packet = "Null packet.";
  const std::string k_no_cb_set = "No callback set.";
  
  
  int GetRakNetIndex(int real_id)
  {
    return ID_USER_PACKET_ENUM + real_id;
  }
  
  int GetGameID(RakNet::Packet* p)
  {
    if( !p )
      return -1;
    
    if( p->data[0] < ID_USER_PACKET_ENUM )
      return -1;
    
    int message_id = 0;
    RakNet::BitStream bsIn(p->data,p->length,false);
    bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
    bsIn.Read(message_id);
    
    return message_id;
  }
  
  int GetMessageDataBytes(RakNet::Packet* p)
  {
    if( !p )
      return -1;
    
    if( p->data[0] < ID_USER_PACKET_ENUM )
      return -1;
    
    int bytes = 0;
    RakNet::BitStream bsIn(p->data,p->length,false);
    bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
    bsIn.IgnoreBytes(sizeof(int));
    bsIn.Read(bytes);
    
    return bytes;
  }
  
  char* GetMessageData(RakNet::Packet* p)
  {
    if( !p )
      return 0;
    
    if( p->data[0] < ID_USER_PACKET_ENUM )
      return 0;
    
    return (char*)(&p->data[ sizeof(RakNet::MessageID) + sizeof(int)*2 ]);
  }
  
  
  GameCallbacks::GameCallbacks()
  {
  }
  
  GameCallbacks::~GameCallbacks()
  {
  }
  
  void GameCallbacks::SetCallback(int real_id, void (*callback)(RakNet::Packet* p), const std::string& desc)
  {
    if( real_id >= static_cast<int>(_callbacks.size()) )
    {
      IncreaseCallbackStorage(real_id);
    }
    
    _callbacks[ real_id ] = callback;
    _callback_desc[ real_id ] = desc;
  }
  
  const std::string& GameCallbacks::GetCallbackDesc(int real_id)
  {
    if( real_id < 0 || real_id >= static_cast<int>(_callback_desc.size()) )
      return k_bad_cb_index;
    return _callback_desc[ real_id ];
  }
  
  const std::string& GameCallbacks::GetCallbackDesc(RakNet::Packet* p)
  {
    if(!p)
      return k_null_packet;
    int callback_index = GetGameID(p);
    
    if( callback_index < 0 || callback_index >= static_cast<int>(_callback_desc.size()) )
      return k_bad_cb_index;
    
    return _callback_desc[ callback_index ];
  }
  
  packet_callback GameCallbacks::GetCallback(int real_id)
  {
    if( real_id < 0 || real_id >= static_cast<int>(_callbacks.size()) )
      return 0;
    return _callbacks[ real_id ];
  }
  
  packet_callback GameCallbacks::GetCallback(RakNet::Packet* p)
  {
    if(!p)
      return 0;
    int callback_index = GetGameID(p);
    
    if( callback_index < 0 || callback_index >= static_cast<int>(_callbacks.size()) )
      return 0;
    
    return _callbacks[ callback_index ];
  }
  
  void GameCallbacks::RunCallback(int real_id, RakNet::Packet* p)
  {
    if( real_id < 0 || real_id >= static_cast<int>(_callbacks.size()) )
      return;
    
    if(!_callbacks[ real_id ])
      return;
    
    _callbacks[ real_id ](p);
  }
  
  void GameCallbacks::RunCallback(RakNet::Packet* p)
  {
    if(!p)
      return;
    int callback_index = GetGameID(p);
    
    if( callback_index < 0 || callback_index >= static_cast<int>(_callbacks.size()) )
      return;
    
    if(!_callbacks[ callback_index ])
      return;
    
    _callbacks[ callback_index ](p);
  }
  
  
  void GameCallbacks::IncreaseCallbackStorage(int min_increase)
  {
    int new_size = _callbacks.size() * 2;
    
    //make sure new size is big enough
    if( new_size <= min_increase )
    {
      new_size = min_increase + 1;
    }
    
    packet_callback pnull_cb = (packet_callback)0;
    
    std::vector<packet_callback> new_callbacks(new_size,pnull_cb);
    std::copy( _callbacks.begin(), _callbacks.end(), new_callbacks.begin() );
    _callbacks = new_callbacks;
    
    std::vector<std::string> new_callbacks_desc(new_size,k_no_cb_set);
    std::copy( _callback_desc.begin(), _callback_desc.end(), new_callbacks_desc.begin() );
    _callback_desc = new_callbacks_desc;
  }
  
  
  
  
  
  
  
}
