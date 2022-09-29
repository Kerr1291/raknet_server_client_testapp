#include <GameMain.h>
#include <Server.h>
#include <GameMessageType.h>
#include <RakNetTypes.h>
#include <helpers.h>

#include <BitStream.h>

#include <RProto.pb.h>

namespace RGame
{
  
  
/// //////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////
// Begin Network Callbacks
/// //////////////////////////////////////////////////////////////////



/// //////////////////////////////////////////////////////////////////
GAME_MESSAGE__BEGIN__CALLBACK( GameState::ProcessChatMessage, rproto::ChatMessage, rgame, rmsg )
///Extract chat info
  std::string owner_ip = p->systemAddress.ToString( false );
  std::string owner_name = rmsg.owner_name();
  rproto::ChatMessage::ChatType chat_type = rmsg.msg_type();
  
  std::string chat_msg = rmsg.content();
///Debug print
  std::cerr<<"["<<owner_name <<"(" <<owner_ip <<")" <<" chat_msg]: \""<<chat_msg <<"\"\r\n";
///Save to log if we're using one
  if( rgame._chat_log.good() )
  {
    rgame._chat_log <<"["<<owner_name <<"(" <<owner_ip <<")" <<" chat_msg]: \""<<chat_msg <<"\"\r\n";
    rgame._chat_log.flush();
  } 

  ///Check the message type
  if( chat_type == rproto::ChatMessage::chat_normal )
  {
    ///Forward to clients if it's normal
    
    RSERVER_BROADCAST_MSG( rproto::msg_chat, rmsg );
  }
  else if( chat_type == rproto::ChatMessage::chat_whisper )
  { 
    ///if it's a whisper we need to get the id's for the destination clients
    if( rgame.HasPlayerName( rmsg.reciever_name() ) )
    {      
      std::string dest_ip = rgame.GetPlayerIP( rmsg.reciever_name() );
      RSERVER_SEND_MSG( rproto::msg_chat, rmsg, dest_ip );
    }
    else
    {
      rmsg.set_content("Player name not found.");
    }
    
    RSERVER_SEND_MSG( rproto::msg_chat, rmsg, owner_ip );
  }
  else
  {
    ///??? fill in behavior other chat types here....
  }
  
GAME_MESSAGE__END____CALLBACK



/// //////////////////////////////////////////////////////////////////
GAME_MESSAGE__BEGIN__CALLBACK( GameState::ProcessShutdownMessage, rproto::RMsg, rgame, rmsg )

  rgame.SetCurrentState( GameState::ShutdownGameState );
  
GAME_MESSAGE__END____CALLBACK


/// //////////////////////////////////////////////////////////////////
GAME_MESSAGE__BEGIN__CALLBACK( GameState::ProcessPlayerInfoMessage, rproto::PlayerInfo, rgame, rmsg )

  std::string player_ip = p->systemAddress.ToString( false );
  std::string player_name = rmsg.name();
  
  //Create new player
  PlayerInfo* new_player = RGame::CreateNewPlayer(player_name, player_ip);
  rgame._players.push_back( new_player );
  rgame.AddPlayer( new_player );

  std::string notice = new_player->_name + std::string(" has connected.");
  SendGameNoticeAll(notice);
  KC_PRINT( notice );
  
  //notify other players?
  
GAME_MESSAGE__END____CALLBACK

/// //////////////////////////////////////////////////////////////////
//GAME_MESSAGE__BEGIN__CALLBACK( GameState::ProcessPlayerQuit, rproto::PlayerInfo, rgame, rmsg )



//GAME_MESSAGE__END____CALLBACK

/// //////////////////////////////////////////////////////////////////
// End Network Callbacks
/// //////////////////////////////////////////////////////////////////

}