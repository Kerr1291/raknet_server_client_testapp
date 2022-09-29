#pragma once
#include <map>
#include <vector>
#include <fstream>
#include <string>

#include <PlayerInfo.h>

namespace RakNet
{
  struct Packet;
}

namespace RState
{
  class SubState;
}

namespace RGame
{
  class GameState;
  typedef void(*game_state_type)(GameState&,double);
  
  
  GameState& GetGameState();

  const char* GetGameError();
  bool        HasGameError();
  
  void SendGameNotice(const std::string& dest_ip, const std::string& msg, int type=1); //1=normal, 2=error
  void SendGameNoticeAll(const std::string& msg, int type=1); //1=normal, 2=error
  
  PlayerInfo* CreateNewPlayer(const std::string& name, const std::string& ip);
  
  //////////////////////////////////////////////////////////////////////////////////////
  //
  class GameState
  {
    friend void        EndGameNow();
    friend int         InitGame();
    friend const char* GetGameError();
    friend bool        HasGameError();
    friend GameState&  GetGameState();
    friend bool        NetworkTrafficLocked();
  private:
    GameState();
    void Shutdown();
    
    //if set to true then no packets or callbacks will be processed!!!
    void SetTrafficLock(bool locked);
    bool GetTrafficLock();
    
  public:
    ~GameState();
    
    void RunGame(double dt);
    void RunSubState(double dt);
    void SetSubState(RState::SubState* new_s_state);
    RState::SubState* GetSubState();
    
    bool GameComplete();
    
    void SetCurrentState(game_state_type state);
    game_state_type GetCurrentState();
    
    void AddPlayer(PlayerInfo* player);
    void RemovePlayer(PlayerInfo* player);
    
    void UpdatePlayerName(PlayerInfo* player, const std::string& new_name);
    void UpdatePlayerIP(PlayerInfo* player, const std::string& new_ip);
    
    bool HasPlayerName(const std::string& name);
    bool HasPlayerIP(const std::string& ip);
    
    std::string GetPlayerIP(const std::string& name);
    
    PlayerInfo* GetPlayerByName(const std::string& name);
    PlayerInfo* GetPlayerByName( const std::string& ip, const std::string& name);
    std::vector<PlayerInfo*> GetPlayersAtIP( const std::string& ip );

    std::vector< PlayerInfo* >& GetPlayers();
    
  ///Members  
  private:
    
    //if this is non-zero, crash the game
    int _game_error;
    game_state_type _current_state;
    
    //if true raknet will suspend the processing of all incomming network traffic
    //set this to true while doing transitions between server states
    bool _server_locked;
    
    //log all chat here
    std::fstream    _chat_log;

    typedef std::map< std::string, PlayerInfo* > t_name_list;
    
    //sets of connected players, sorted by ip and then name
    std::map<std::string, t_name_list> _ip_list;
    
    //set of connected players
    std::vector< PlayerInfo* > _players;
    
    RState::SubState* _sub_state;
    
  public:
  /// /// /// Game States /// /// ///
    
  ///Basic startup and shutdown
    static void FirstInitState(GameState& me, double dt);
    static void ShutdownGameState(GameState& me, double dt);
    
  ///Test state
    static void TempMainState(GameState& me, double dt);
    
  ///States other than running a game
    static void LobbyMainState(GameState& me, double dt);
    
  ///States for setting up a game
    static void PrepareToLoadGame(GameState& me, double dt);
    static void LoadGame_Server(GameState& me, double dt);
    static void LoadGame_Clients(GameState& me, double dt);
    static void PrepareToRunGame(GameState& me, double dt);
    static void RunGameMain(GameState& me, double dt);
    static void PrepareToCloseGame(GameState& me, double dt);
    static void CloseGame_Clients(GameState& me, double dt);
    static void CloseGame_Server(GameState& me, double dt);
    
  /// /// /// Network Callbacks /// /// ///
    static void  ProcessChatMessage(RakNet::Packet* p);
    static void  ProcessShutdownMessage(RakNet::Packet* p);
    
    static void  ProcessPlayerInfoMessage(RakNet::Packet* p);
    //static void  ProcessPlayerQuit( RakNet::Packet* p );
  };
  //
  //////////////////////////////////////////////////////////////////////////////////////
  
  ///Used by substates to make a clean way to get their type
  template <typename T>
  T* GetSubStateType()
  {
    return dynamic_cast<T*>(GetGameState().GetSubState());
  }

  ///Server.cpp calls these functions from a thread
  int InitGame();
  int GameMain(double dt);
  bool NetworkTrafficLocked();
}