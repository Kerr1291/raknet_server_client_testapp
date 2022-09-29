#pragma once

/// //////////////////
//
// Client interface for the game
//
//
//
/// //////////////////

#include <BuildSettings.h>


void RClientSetErrorState(int error_id);

enum RCLIENT_ERRORS
{
 RCLIENT_NO_ERROR = 0  /*00*/ //"No Error and No connection attempt made."
,RCLIENT_CONNECTION_FAILED_UNKNOWN      /*01*/ //,"Last connection attempt failed. Unknown reason."
,RCLIENT_CONNECTING      /*02*/ //,"Currently attempting to connect."
,RCLIENT_CONNECTION_FAILED_NO_RESPONSE      /*03*/ //,"Connection failed. No response."
,RCLIENT_CONNECTION_FAILED_PORT_IN_USE      /*04*/ //,"Connection failed. Port in use."
,RCLIENT_CONNECTION_FAILED_BAD_PASSWORD      /*05*/ //,"Connection failed. Bad password."
,RCLIENT_CONNECTION_FAILED_BANNED      /*06*/ //,"Connection failed. Client is banned."
,RCLIENT_CONNECTION_FAILED_SERVER_FULL      /*07*/ //,"Connection failed. Server is full."
,RCLIENT_CONNECTED      /*08*/ //,"Connected to server."
,RCLIENT_DISCONNECTED      /*09*/ //,"Client disconnected from server."
,RCLIENT_SERVER_SHUTDOWN      /*10*/ //,"Server shutdown."
,RCLIENT_SERVER_KICKED      /*11*/ //,"Server disconnected this client. (kicked?)"
,RCLIENT_SERVER_BANNED      /*12*/ //,"Server has banned this client."
,RCLIENT_BAD_CLIENT_ID      /*13*/ //,"Server has banned this client."
,RCLIENT_RAKNET_FAILED_START      /*14*/ //,"Server has banned this client."
};


extern "C"
{
  
RSERVER_API const char*        RCLIENT_GetErrorDesc();
  
RSERVER_API int                RCLIENT_GetErrorState();

//returns client id of new client
RSERVER_API int                RCLIENT_AllocateNewClient();

//i0-3 = ip address
//port = server port

RSERVER_API void               RCLIENT_ConnectToServer(int i0, int i1, int i2, int i3, int port, int client_port, int client_id);

RSERVER_API int                RCLIENT_GetConnectedState(int client_id);

RSERVER_API void               RCLIENT_DisconnectClient(int client_id);

RSERVER_API int                RCLIENT_GetPacket(int* raw_message_id, int* game_message_id, char** packet_data, int* bytes, int client_id);

RSERVER_API int                RCLIENT_FreeLastPacket(int client_id);

RSERVER_API int                RCLIENT_SendGamePacket(int game_message_id, char* packet_data, int bytes, int client_id);

RSERVER_API int                RCLIENT_SendRakNetPacket(int raknet_message_id, char* packet_data, int bytes, int client_id);

}