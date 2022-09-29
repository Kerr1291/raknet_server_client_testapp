#include <TestRunGame.h>

#include <GameMain.h>
#include <Server.h>
#include <GameMessageType.h>
#include <RakNetTypes.h>
#include <helpers.h>

#include <BitStream.h>

#include <RProto.pb.h>

namespace RState
{
  
const int k_player_type = 5;
const int k_player_sight_radius = 5;
const int k_player_sight_power = 5;
 
TestRunGame::TestRunGame()
:_running(true)
,_loaded(false)
{
  _t_game_id = 9000000;
}

TestRunGame::~TestRunGame()
{
  delete _fov;
}

void TestRunGame::Load(double dt)
{
  KC_MSG("Starting game...\r\n");
  
  const int k_floor_type = 0;
  const int k_wall_type = 1;
  
  ///make level
  _level.SetSize( 50,20 );
  _level.AreaFillEntType(k_floor_type,   1,1,48,18);
  _level.AreaFillRectEntType(k_wall_type,0,0,50,20);
  
  
  ///create fov calculator
  _fov = new RMap::Rfov(_level);
  
  _walls.insert( k_wall_type );
  
  ///the fov calculator knows what walls are now
  _fov->SetMask( _walls );
  
  
  
  ///add players
  std::vector< RGame::PlayerInfo* >& all_players = RGame::GetGameState().GetPlayers();
  for( unsigned i = 0; i < all_players.size(); ++i )
  {
    RMap::Cell* c = _level.GetRandomCellWithEntType(0);
    
    RType::Ent new_player;
    new_player._type_id = k_player_type;
    new_player._game_id = _t_game_id++;
    new_player._map_index = -1;
    
    all_players[i]->_px = c->x();
    all_players[i]->_py = c->y();
    
    all_players[i]->_fpx = c->x();
    all_players[i]->_fpy = c->y();
    
    all_players[i]->_game_id = new_player._game_id;
    
    c->AddEnt(new_player);
    
    _things.push_back( new_player );
    
    UpdatePlayerView(all_players[i]);
  }
  
  RServer::GetServer().SetGameCallback( rproto::msg_player_move,     
                                        TestRunGame::ProcessPlayerMove, "Process a player move request." );
  
  rproto::RMsg ready_msg;
  ready_msg.set_message_id( rproto::msg_notify_players_ready );
  RSERVER_BROADCAST_MSG( rproto::msg_notify_players_ready, ready_msg );
  
  
  _loaded = true;
}

bool TestRunGame::Loaded()
{
  return _loaded;
}

bool TestRunGame::Running()
{
  return _running;
}

void TestRunGame::Run(double dt)
{
}

void TestRunGame::OnExit()
{
  KC_MSG("Exiting game...\r\n");
  RGame::GetGameState().SetCurrentState( RGame::GameState::PrepareToCloseGame );
}


void TestRunGame::UpdatePlayerView(RGame::PlayerInfo* player)
{
  _fov->Calculate(player->_px, player->_py, k_player_sight_radius, k_player_sight_power);
  
  std::vector<RMap::Cell*> cells_seen;
  _fov->GetFOV( cells_seen );
  
  rproto::View view_msg;
  view_msg.set_radius( k_player_sight_radius );
  
  for( unsigned i = 0; i < cells_seen.size(); ++i )
  {
    rproto::Cell* cell_msg = view_msg.add_cells();
    cell_msg->set_cx( cells_seen[i]->x() );
    cell_msg->set_cy( cells_seen[i]->y() );
    
    unsigned num = cells_seen[i]->GetNumEnts();
    for( unsigned j = 0; j < num; ++j )
    {
      RType::Ent* c_ent = cells_seen[i]->GetEnt(j);
      
      rproto::Ent* ent_msg = cell_msg->add_ents();
      ent_msg->set_type_id( c_ent->_type_id );
      ent_msg->set_game_id( c_ent->_game_id );
    }
  }
  
  RSERVER_SEND_MSG( rproto::msg_fov_data, view_msg, player->_ip );
}


/// //////////////////////////////////////////////////////////////////
GAME_MESSAGE__BEGIN__CALLBACK( TestRunGame::ProcessPlayerMove, rproto::PlayerMoveCommand, rgame, rmsg )

  TestRunGame* me = RGame::GetSubStateType<TestRunGame>();
  if( me )
  { 
    std::string player_name = rmsg.name();
    RGame::PlayerInfo* player = rgame.GetPlayerByName(player_name);
    if(!player)
      return; //throw some kind of error here?
    
    float new_x = player->_fpx + rmsg.dx();
    float new_y = player->_fpy + rmsg.dy();
    
    int cx = new_x; //truncate to cells
    int cy = new_y;
    
    bool recalculate_views = false;
    
    if( cx == player->_px && cy == player->_py )
    {
      //no need to recalculate views
    }
    //we're moving to a new cell, see if it's valid
    else
    {
      //are we moving to a valid place?
      RMap::Cell* cell = me->_level.GetCell(cx,cy);
      if( !cell || cell->HasTypeInMask( me->_walls ) )
      {
        new_x = player->_fpx;
        new_y = player->_fpx;
        cx = new_x; //truncate to cells
        cy = new_y;
      }
      else
      {
        //we moved to a new cell, time to recalculate views.
        int old_cx = player->_px;
        int old_cy = player->_py;
        me->_level.MoveEnt( player->_game_id, old_cx, old_cy, cx, cy );
        recalculate_views = true;
      }
    }
    
    player->_fpx = new_x;
    player->_fpx = new_y;
    
    player->_px = cx;
    player->_py = cy;
  
    rproto::PlayerPos pos_msg;
    pos_msg.set_px( new_x );
    pos_msg.set_py( new_y );
    pos_msg.set_cx( cx );
    pos_msg.set_cy( cy );
  
    RSERVER_SEND_MSG( rproto::msg_player_pos, pos_msg, player->_ip );
      
    if( recalculate_views )
    {
      std::vector< RGame::PlayerInfo* >& all_players = RGame::GetGameState().GetPlayers();
      for( unsigned i = 0; i < all_players.size(); ++i )
        me->UpdatePlayerView(all_players[i]);
    }
  }  
  
GAME_MESSAGE__END____CALLBACK
 
 
}
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 