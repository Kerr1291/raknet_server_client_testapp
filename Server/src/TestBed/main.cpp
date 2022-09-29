#include <BuildSettings.h>
#ifdef LIBRARY_EXPORTS
  //disables main
#else

#include <iostream>

#include <string>
#include <algorithm>

#include <helpers.h>


#include <ServerInterface.h>
#include <ClientInterface.h>

#include <RakSleep.h>
#include <RakThread.h>

#include <ctime>

#include <RProto.pb.h>


#include <test_helper.h>

using namespace std;





std::string pname;
int client_id = 0;
bool game_ready = false;
bool in_game = true;


void RunTestClientPackets_InGame();

RAK_THREAD_DECLARATION( RunClientMessages )
{
  while( in_game )
  {
    RunTestClientPackets_InGame();
    RenderGame();
  }
  return 0;
}



bool RunTestClientPackets_Lobby()
{
  char* packet_data = 0;
  int raknet_id = -1;
  int message_id = -1;
  int bytes = 0;
  
  std::string msg;
  while( RCLIENT_GetPacket(&raknet_id, &message_id, &packet_data, &bytes, client_id) > 0 )
  {
    std::cerr<<"Recieved packet with id: "<<message_id <<"\n";
    msg.clear();
    
    if( rproto::msg_srequest == message_id )
    {
      rproto::SRequest request;
      request.ParseFromArray(packet_data, bytes);
      
      if( request.request_id() == rproto::SRequest::r_pinfo )
      {
        rproto::PlayerInfo pinfo;
        pinfo.set_name( pname );
        pinfo.set_ip( "127.0.0.1" );
        pinfo.SerializeToString( &msg );
        RCLIENT_SendGamePacket(rproto::msg_player_info, const_cast<char*>(msg.c_str()), msg.size(), client_id);
        continue;
      }
    }
    
    if( message_id == rproto::msg_chat )
    {
      std::string text;
      std::string owner;
      rproto::ChatMessage chatmsg;
      chatmsg.ParseFromArray(packet_data, bytes);
      
      text = chatmsg.content();
      owner = chatmsg.owner_name();
      std::cerr<<"Test Client:: "<<owner <<" says: "<<text <<"\n";
      continue;
    }
    
    
    if( message_id == rproto::msg_notify_players_loading )
    {
      std::cerr<<"Server is loading game. Exiting main loop to loading state...\n";
      return true;
    }
    
    //std::cerr<<"Recieved packet with data: "<<msg <<"\n";
    std::cerr<<"---\n";
  }
  RCLIENT_FreeLastPacket(client_id);
  return false;
}

void RunTestClientPackets_InGame()
{
  char* packet_data = 0;
  int raknet_id = -1;
  int message_id = -1;
  int bytes = 0;
  
  std::string msg;
  while( RCLIENT_GetPacket(&raknet_id, &message_id, &packet_data, &bytes, client_id) > 0 )
  {
    std::cerr<<"Recieved packet with id: "<<message_id <<"\n";
    msg.clear();
    
    if( rproto::msg_srequest == message_id )
    {
      rproto::SRequest request;
      request.ParseFromArray(packet_data, bytes);
      
      if( request.request_id() == rproto::SRequest::r_pinfo )
      {
        rproto::PlayerInfo pinfo;
        pinfo.set_name( pname );
        pinfo.set_ip( "127.0.0.1" );
        pinfo.SerializeToString( &msg );
        RCLIENT_SendGamePacket(rproto::msg_player_info, const_cast<char*>(msg.c_str()), msg.size(), client_id);
        continue;
      }
    }
    
    else if( message_id == rproto::msg_chat )
    {
      std::string text;
      std::string owner;
      rproto::ChatMessage chatmsg;
      chatmsg.ParseFromArray(packet_data, bytes);
      
      text = chatmsg.content();
      owner = chatmsg.owner_name();
      std::cerr<<"Test Client:: "<<owner <<" says: "<<text <<"\n";
      continue;
    }
    
    else if( message_id == rproto::msg_notify_players_ready )
    {
      std::cerr<<"Server is ready! Joining game.\n";
      game_ready = true;
      return;
    }
    
    else
    {
      RespondToMessage(message_id, packet_data, bytes);
    }
    
    //std::cerr<<"Recieved packet with data: "<<msg <<"\n";
    std::cerr<<"---\n";
  }
  RCLIENT_FreeLastPacket(client_id);
}



int main()
{
  int pid = 0;
  do
  {
    std::cout << "Server Parameters: \n";
    for(int i = 0; i < RSERVER_GetNumParams(); ++i )
    {
      const char* desc = RSERVER_GetParamDesc(i);
      std::cout << i <<" - " <<desc <<"\n";
    }
    
    std::cout << "Enter server param id (-1 to continue, -2 to quit): ";
    std::cin>>pid;
    if(pid >= 0)
    {
      int ready = 0;
      int port = 0;
      if( pid == 4 )
      {
        std::cout << "Enter server port: ";
        cin>>port;
        RSERVER_ProcessParameter(pid, &port);
      }
      else if( pid == 5 )
      {
        RSERVER_ProcessParameter(pid, &ready);
        std::cout << "Server Ready?: "<<ready <<"\n";
      }
      else
        RSERVER_ProcessParameter(pid);
    }
  }
  while(pid >= 0);

  if (pid == -2)
  {
    //kill server
    RSERVER_ProcessParameter(0);
    return 0;
  }
  
  
  std::cerr<<"Enter player name: ";
  std::cin>>pname;
  SetName( pname );
  
  int connect_port = RSERVER_GetServerGamePort();
  
  
  client_id = RCLIENT_AllocateNewClient();
  
  SetInternalID( client_id );
  
  RCLIENT_ConnectToServer(127,0,0,1, connect_port, connect_port+1, client_id);
  
  std::cerr <<"Connecting to "<<"127.0.0.1" <<" : "<<connect_port <<" .";
  std::cerr.flush();
  int ticks = 0;
  int connected_state = 0;
  while( ticks < 10 )
  {
    std::cerr <<".";
    std::cerr.flush();
    connected_state = RCLIENT_GetConnectedState(client_id);
    if( connected_state )
      break;
    
    RakSleep(1000);
    ticks++;
  }
    
  connected_state = RCLIENT_GetConnectedState(client_id);

  if( connected_state )
    std::cerr <<"\nConnection successful.\n";
  else
  {
    std::cerr <<"\nConnection failed after 10 seconds.\n";
    
    if( RCLIENT_GetErrorState() )
    {
      std::cerr << "Error: " <<RCLIENT_GetErrorDesc() <<"\n";
      RSERVER_ProcessParameter(0);  
      return 0;
    }
  }
  
  bool exit_lobby = false;
  
  ///lobby state
  char input[256];
  std::string in_msg;
  while( in_msg != "bye" )
  {
    exit_lobby = RunTestClientPackets_Lobby();
    
    if( exit_lobby )
    {
      std::cerr<<"Exiting lobby...\n";
      break;
    }
    
    std::cerr<<"==Available Commands=\n";
    
    std::cerr<<"Say \"Start Game\" to have the server load the game.\n";
    
    std::cerr<<"===\n";
    
    cin.getline(input,256);
    in_msg = std::string(input);
    
    if( in_msg.empty() )
      continue;
    
    std::string msg;
    
    if( in_msg == "Start Game" )
    {
      std::cerr<<"Doing start game request!\n";
      rproto::RMsg start_game;
      start_game.set_message_id( rproto::msg_lobby_start_game );
      
      start_game.SerializeToString( &msg );
      
      RCLIENT_SendGamePacket(rproto::msg_lobby_start_game, const_cast<char*>(msg.c_str()), msg.size(), client_id);
      exit_lobby = true;
      break;
    }
    
    rproto::ChatMessage chatty;
    chatty.set_owner_ip("127.0.0.1");
    chatty.set_owner_name(pname);
    chatty.set_msg_type(rproto::ChatMessage::chat_normal);
    chatty.set_content(in_msg);
    
    chatty.SerializeToString( &msg );
          
    RCLIENT_SendGamePacket(rproto::msg_chat, const_cast<char*>(msg.c_str()), msg.size(), client_id);
    
    //send message
    
    RakSleep(500);
  }
  
  
  std::cerr<<"waiting for game to load...\n";
  
  ///server is now loading
  while( !game_ready )
  {
    RunTestClientPackets_InGame();
    RakSleep(500);
  }
  
  std::cerr<<"starting game!!!!...\n";
  
  InitTestRender();
  SetQuitBool( &in_game );
  
  RakNet::RakThread::Create(&RunClientMessages, 0);
  
  ///now playing game
  while( in_game )
  {
    RunPlayer();
  }
  
  EndTestRender();
  
  //kill client
  RCLIENT_DisconnectClient( client_id );
  
  //kill server
  RSERVER_ProcessParameter(0);  
  
  return 0;
}


#endif

