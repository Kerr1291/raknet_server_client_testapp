 
#pragma once

#include <SubState.h>

#include <Map.h>
#include <Types.h>
#include <vector>
#include <FOV.h>
#include <PlayerInfo.h>
 
namespace RState
{
 
class TestRunGame : public SubState
{
public:
  TestRunGame();
  ~TestRunGame();
  
  virtual void Load(double dt);
  virtual bool Loaded();
  
  virtual bool Running();
  virtual void Run(double dt);
  
  virtual void OnExit();
  
  void UpdatePlayerView(RGame::PlayerInfo* player);

private:
  
  static void  ProcessPlayerMove(RakNet::Packet* p);
  
  bool _running;
  bool _loaded;
  
  RMap::Map                _level;
  std::vector<RType::Ent>  _things;
  
  int _t_game_id;
  
  RMap::Rfov* _fov;
  std::set<unsigned> _walls;
};

}