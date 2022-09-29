#pragma once

#include <SubState.h>

 
namespace RState
{
 
class LobbyState : public SubState
{
public:
  LobbyState();
  ~LobbyState();
  
  virtual void Load(double dt);
  virtual bool Loaded();
  
  virtual bool Running();
  virtual void Run(double dt);
  
  virtual void OnExit();
  
  
  static void  ProcessStartGame(RakNet::Packet* p);

private:
  
  bool _running;
  bool _loaded;
  
};

}