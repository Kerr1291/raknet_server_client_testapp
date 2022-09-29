#include <LobbyState.h>
#include <GameMain.h>
#include <Server.h>
#include <GameMessageType.h>
#include <RakNetTypes.h>
#include <helpers.h>

#include <BitStream.h>

#include <RProto.pb.h>

namespace RState
{
 
LobbyState::LobbyState()
:_running(true)
,_loaded(false)
{
}

LobbyState::~LobbyState()
{
}

void LobbyState::Load(double dt)
{
  KC_MSG("Loading lobby...\r\n");
  
  RServer::GetServer().SetGameCallback( rproto::msg_lobby_start_game,     
                                        LobbyState::ProcessStartGame, "Start the game." );
    
  
  _loaded = true;
}

bool LobbyState::Loaded()
{
  return _loaded;
}

bool LobbyState::Running()
{
  return _running;
}

void LobbyState::Run(double dt)
{
}

void LobbyState::OnExit()
{
  KC_MSG("Exiting lobby...\r\n");
  RGame::GetGameState().SetCurrentState( RGame::GameState::PrepareToLoadGame );
}
 
 
/// //////////////////////////////////////////////////////////////////
GAME_MESSAGE__BEGIN__CALLBACK( LobbyState::ProcessStartGame, rproto::RMsg, rgame, rmsg )

  LobbyState* me = RGame::GetSubStateType<LobbyState>();
  if( me )
  {
    KC_MSG("Recieved start game message in lobby!...\r\n");
    
    //tell the players the server is now loading the game
    rproto::RMsg loading_msg;
    loading_msg.set_message_id( rproto::msg_notify_players_loading );
  
    RSERVER_BROADCAST_MSG( rproto::msg_notify_players_loading, loading_msg );
    
    me->_running = false;
  }  
  
GAME_MESSAGE__END____CALLBACK

}