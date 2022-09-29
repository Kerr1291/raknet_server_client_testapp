#pragma once
#include <string>

namespace RGame
{



struct PlayerInfo
{  
  std::string _name;
  std::string _ip;
  
  int         _px;
  int         _py;
  
  float       _fpx;
  float       _fpy;
  
  int         _game_id;
};



}