#pragma once



namespace RakNet
{
  struct Packet;
}

namespace RState
{

class SubState
{
public:
  SubState(){}
  virtual ~SubState(){}
  
  virtual void Load(double dt){}
  virtual bool Loaded() = 0;
  
  virtual bool Running() = 0;
  virtual void Run(double dt){}
  
  virtual void OnExit(){}
};

}