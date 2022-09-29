#include <GameMain.h>
#include <Server.h>
#include <GameMessageType.h>
#include <RakNetTypes.h>
#include <helpers.h>

#include <BitStream.h>

#include <RProto.pb.h>
  
namespace RGame
{
  
GameState& GetGameState()
{
  static GameState _game;
  return _game;
}

void       EndGameNow()
{
  GetGameState().Shutdown();
}



/// //////////////////////////////////////////////////////////////////
//
const char* k_game_err_messages[] =
{
  /*00*/"No Error."
  /*01*/,"Unknown Error."
};

const char* GetGameError()
{
  return k_game_err_messages[ GetGameState()._game_error ];
}

bool HasGameError()
{
  return ( GetGameState()._game_error > 0 );
}


const rproto::GameNotice_NoticeType k_to_notice[4] =
{
  rproto::GameNotice::m_null,
  rproto::GameNotice::m_normal,
  rproto::GameNotice::m_error,
  rproto::GameNotice::m_notice,
};


void SendGameNotice(const std::string& dest_ip, const std::string& msg, int type)
{
  rproto::GameNotice game_msg;
  game_msg.set_content( msg );
  
  game_msg.set_msg_type( k_to_notice[type] );
  
  RSERVER_SEND_MSG( rproto::msg_game_notice, game_msg, dest_ip );
}

void SendGameNoticeAll(const std::string& msg, int type) //1=normal, 2=error
{
  rproto::GameNotice game_msg;
  game_msg.set_content( msg );
  game_msg.set_msg_type( k_to_notice[type] );
  
  RSERVER_BROADCAST_MSG( rproto::msg_game_notice, game_msg );
}

PlayerInfo* CreateNewPlayer(const std::string& name, const std::string& ip)
{
  PlayerInfo* player = new PlayerInfo();
  player->_name = name;
  player->_ip = ip;
  return player;
}

//
/// //////////////////////////////////////////////////////////////////

//return  0 = success, normal game bootup ok
//return anything else = failed to start and there is an error set
int InitGame()
{  
  KC_MSG("Preparing to load game...\r\n");
  GameState& the_game = GetGameState();
  the_game.SetCurrentState( RGame::GameState::FirstInitState );
  
  /// //////////////////////////////////////////////////////////////////
  // Begin Network Callback list
  
  RServer::GetServer().SetGameCallback( rproto::msg_chat,            GameState::ProcessChatMessage, "Process chat message." );
  RServer::GetServer().SetGameCallback( rproto::msg_shutdown_server, GameState::ProcessShutdownMessage, "Shutdown game." );
  
  RServer::GetServer().SetGameCallback( rproto::msg_player_info,     GameState::ProcessPlayerInfoMessage, "Update Player Info." );
    
  // End Network Callback list
  /// //////////////////////////////////////////////////////////////////
  
  
  the_game._chat_log.open( "./GameServerChatLog.txt", std::ios::out );

  return 0;
}


//return    0  = shutdown normally
//return >= 1  = crash game and there is an error message set
//return <=-1  = game currently running
int GameMain(double dt)
{
  GameState& the_game = GetGameState();
  if( the_game.GameComplete() )
    return 0;
  
  if( HasGameError() )
  {
    EndGameNow();
    return 1;
  }
  
  the_game.RunGame(dt);
  return -1;
}

bool NetworkTrafficLocked()
{
  return GetGameState().GetTrafficLock();
}


}