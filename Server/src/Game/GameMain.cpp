#include <GameMain.h>
#include <Server.h>
#include <GameMessageType.h>
#include <RakNetTypes.h>
#include <helpers.h>

#include <BitStream.h>

#include <RProto.pb.h>

#include <SubState.h>

namespace RGame
{



GameState::GameState()
:_game_error(0)
,_current_state(0)
,_sub_state(0)
{
}

void GameState::Shutdown()
{
  //free all allocated memory
  if( _sub_state )
    delete _sub_state;
  _sub_state = 0;
  _current_state = 0;
  _chat_log.close();
  
  for(unsigned i = 0; i < _players.size(); ++i)
    delete _players[i];
  _players.clear();
  _ip_list.clear();
  _server_locked = false;
  _game_error = 0;
}

void GameState::SetTrafficLock(bool locked)
{
  _server_locked = locked;
  if( _server_locked )
  {
    RServer::GetServer().DisableNewConnections();
  }
  else
  {
    RServer::GetServer().EnableNewConnections();
  }
}

bool GameState::GetTrafficLock()
{
  return _server_locked;
}

GameState::~GameState()
{
  //should only ever really be called when the program closes (if then even)
  Shutdown();
}

void GameState::RunGame(double dt)
{
  game_state_type state = GetCurrentState();
  if(!state)
    return;
  state(*this,dt);
}

void GameState::RunSubState(double dt)
{  
  if( _sub_state && !_sub_state->Loaded() )
    _sub_state->Load(dt);
  
  else if( _sub_state && _sub_state->Loaded() && _sub_state->Running() )
    _sub_state->Run(dt);
  
  else if( _sub_state && !_sub_state->Running() )
  {
    RState::SubState* old_state = _sub_state;
    old_state->OnExit();
    
    if( old_state == _sub_state )
      _sub_state = 0;
    
    delete old_state;
  }
}

void GameState::SetSubState(RState::SubState* new_s_state)
{
  if( _sub_state )
  {
    delete _sub_state;
    _sub_state = 0;
  }
  
  _sub_state = new_s_state;
}

RState::SubState* GameState::GetSubState()
{
  return _sub_state;
}

bool GameState::GameComplete()
{
  if( GetCurrentState() )
    return false;
  return true;
}

void GameState::SetCurrentState(game_state_type state)
{
  _current_state = state;
}

game_state_type GameState::GetCurrentState()
{
  return _current_state;
}


void GameState::AddPlayer(PlayerInfo* player)
{
  if( !player )
    return;
  
  if( GetPlayerByName( player->_name ) )
  {
    int c = 1;
    std::stringstream counter;
    counter << player->_name << c++;
    std::string new_name = counter.str();
    while( GetPlayerByName( new_name ) )
    {
      counter.str("");
      counter << player->_name << c++;
      new_name = counter.str();
    }
    player->_name = new_name;
  }

  if( _ip_list.count( player->_ip ) )
  {
    _ip_list[ player->_ip ][ player->_name ] = player;
  }
  else
  {
    _ip_list[ player->_ip ] = t_name_list();
    _ip_list[ player->_ip ][ player->_name ] = player;
  }
}

void GameState::RemovePlayer(PlayerInfo* player)
{
  if( !player )
    return;
  
  std::map<std::string, t_name_list>::iterator fip = _ip_list.find( player->_ip );

  if(fip != _ip_list.end())
  {
    std::map<std::string, PlayerInfo*>::iterator fname = fip->second.find( player->_name );

    if(fname != fip->second.end())
      fip->second.erase( fname );

    if(fip->second.empty())
      _ip_list.erase( fip );
  }
}

void GameState::UpdatePlayerName(PlayerInfo* player, const std::string& new_name)
{
  if(!player)
    return;
  
  RemovePlayer( player );
  player->_name = new_name;
  AddPlayer( player );
}

void GameState::UpdatePlayerIP(PlayerInfo* player, const std::string& new_ip)
{
  if(!player)
    return;
  
  RemovePlayer( player );
  player->_ip = new_ip;
  AddPlayer( player );
}

bool GameState::HasPlayerName(const std::string& name)
{
  PlayerInfo* player = GetPlayerByName( name );
  return ( player != 0 );
}

bool GameState::HasPlayerIP(const std::string& ip)
{
  return (_ip_list.count( ip ) != 0);
}

std::string GameState::GetPlayerIP(const std::string& name)
{
  std::string ip;
  PlayerInfo* player = GetPlayerByName( name );
  if( player )
  {
    ip = player->_ip;
  }
  return ip;
}

PlayerInfo* GameState::GetPlayerByName(const std::string& name)
{
  std::map<std::string, t_name_list>::iterator fip = _ip_list.begin();

  for(; fip != _ip_list.end(); ++fip)
  {
    std::map<std::string, PlayerInfo*>::iterator fname = fip->second.find( name );

    if(fname != fip->second.end())
      return fname->second;
  }
  return 0;
}

PlayerInfo* GameState::GetPlayerByName( const std::string& ip, const std::string& name )
{
  std::map<std::string, t_name_list>::iterator fip = _ip_list.find( ip );

  if(fip != _ip_list.end())
  {
    std::map<std::string, PlayerInfo*>::iterator fname = fip->second.find( name );

    if(fname != fip->second.end())
      return fname->second;
  }
  return 0;
}

std::vector<PlayerInfo*> GameState::GetPlayersAtIP(const std::string& ip)
{
  std::vector<PlayerInfo*> players;

  std::map<std::string, t_name_list>::iterator fip = _ip_list.find( ip );

  if(fip != _ip_list.end())
  {
    std::map<std::string, PlayerInfo*>::iterator fname = fip->second.begin();

    for(; fname != fip->second.end(); ++fname)
    {
      players.push_back( fname->second );
    }
  }

  return players;
}

std::vector< PlayerInfo* >& GameState::GetPlayers()
{
  return _players;
}









}///end namespace RGame