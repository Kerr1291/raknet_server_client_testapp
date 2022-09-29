#include <GameMain.h>
#include <Server.h>
#include <GameMessageType.h>
#include <RakNetTypes.h>
#include <helpers.h>

#include <BitStream.h>

#include <RProto.pb.h>

///Sub states
#include <LobbyState.h>
#include <TestRunGame.h>


namespace RGame
{

/// //////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////
// Begin Game States
/// //////////////////////////////////////////////////////////////////


void GameState::FirstInitState(GameState& me, double dt)
{
  if(!me.GetTrafficLock())
    me.SetTrafficLock(true);
  
  KC_MSG("Loading game...\r\n");
  me.SetCurrentState( GameState::LobbyMainState );
  me.SetSubState( new RState::LobbyState() );
}

void GameState::ShutdownGameState(GameState& me, double dt)
{
  KC_MSG("Shutting down game...\r\n");
  me.SetCurrentState( 0 );
}


void GameState::TempMainState(GameState& me, double dt)
{
  //run.... do nothing for now
}

void GameState::LobbyMainState(GameState& me, double dt)
{
  if(me.GetTrafficLock())
  {
    RServer::GetServer().PurgeOldPackets();
    me.SetTrafficLock(false);
  }
  
  me.RunSubState( dt );
}

void GameState::PrepareToLoadGame(GameState& me, double dt)
{
  if(!me.GetTrafficLock())
    me.SetTrafficLock(true);
  
  me.SetCurrentState( GameState::LoadGame_Server );
}

void GameState::LoadGame_Server(GameState& me, double dt)
{
  me.SetCurrentState( GameState::LoadGame_Clients );
}

void GameState::LoadGame_Clients(GameState& me, double dt)
{
  me.SetCurrentState( GameState::PrepareToRunGame );
}

void GameState::PrepareToRunGame(GameState& me, double dt)
{  
  if(me.GetTrafficLock())
  {
    RServer::GetServer().PurgeOldPackets();
    me.SetTrafficLock(false);
  }
  
  me.SetCurrentState( GameState::RunGameMain );
  me.SetSubState( new RState::TestRunGame() );
}

void GameState::RunGameMain(GameState& me, double dt)
{
  me.RunSubState( dt );
}

void GameState::PrepareToCloseGame(GameState& me, double dt)
{
  if(!me.GetTrafficLock())
    me.SetTrafficLock(true);
  
  me.SetCurrentState( GameState::CloseGame_Clients );
}

void GameState::CloseGame_Clients(GameState& me, double dt)
{
  me.SetCurrentState( GameState::CloseGame_Server );
}

void GameState::CloseGame_Server(GameState& me, double dt)
{
  me.SetCurrentState( GameState::ShutdownGameState );
}

// End Game States
/// //////////////////////////////////////////////////////////////////


}