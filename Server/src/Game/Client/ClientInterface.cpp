#include <ClientInterface.h>
#include <Client.h>
#include <GameMessageType.h>

const int k_num_errors = 15;

const char* k_client_errors[k_num_errors] =
{
/*00*/ "No Error and No connection attempt made."
/*01*/,"Last connection attempt failed. Unknown reason."
/*02*/,"Currently attempting to connect."
/*03*/,"Connection failed. No response."
/*04*/,"Connection failed. Port in use."
/*05*/,"Connection failed. Bad password."
/*06*/,"Connection failed. Client is banned."
/*07*/,"Connection failed. Server is full."
/*08*/,"Connected to server."
/*09*/,"Client disconnected from server."
/*10*/,"Server shutdown."
/*11*/,"Server disconnected this client. (kicked?)"
/*12*/,"Server has banned this client."
/*13*/,"Connect failed. Bad client id."
/*14*/,"Connect failed. RakNet failed to start."
///Only add onto this list, don't change index order
};

static int s_rclient_last_error = 0;

void RClientSetErrorState(int error_id)
{
  if( error_id < k_num_errors )
    s_rclient_last_error = error_id;
}

bool RClientIsErrorID(int error_id)
{
  switch( error_id )
  {
    case RCLIENT_NO_ERROR:   /*00*/ //"No Error and No connection attempt made."
    case RCLIENT_CONNECTING:      /*02*/ //case "Currently attempting to connect."
    case RCLIENT_CONNECTED:      /*08*/ //case "Connected to server."
    case RCLIENT_DISCONNECTED:      /*09*/ //case "Client disconnected from server."
    case RCLIENT_SERVER_SHUTDOWN:      /*10*/ //case "Server shutdown."
      return false;
    case RCLIENT_CONNECTION_FAILED_UNKNOWN:      /*01*/ //case "Last connection attempt failed. Unknown reason."
    case RCLIENT_CONNECTION_FAILED_NO_RESPONSE:      /*03*/ //case "Connection failed. No response."
    case RCLIENT_CONNECTION_FAILED_PORT_IN_USE:      /*04*/ //case "Connection failed. Port in use."
    case RCLIENT_CONNECTION_FAILED_BAD_PASSWORD:      /*05*/ //case "Connection failed. Bad password."
    case RCLIENT_CONNECTION_FAILED_BANNED:      /*06*/ //case "Connection failed. Client is banned."
    case RCLIENT_CONNECTION_FAILED_SERVER_FULL:      /*07*/ //case "Connection failed. Server is full."
    case RCLIENT_SERVER_KICKED:      /*11*/ //case "Server disconnected this client. (kicked?)"
    case RCLIENT_SERVER_BANNED:      /*12*/ //case "Server has banned this client."
    case RCLIENT_BAD_CLIENT_ID:      /*13*/ //case "Server has banned this client."
    case RCLIENT_RAKNET_FAILED_START:      /*14*/ //case "Server has banned this client."
      return true;
    default:
      return false;
  };
  return false;
}

extern "C"
{
  
RSERVER_API const char*        RCLIENT_GetErrorDesc()
{
  return k_client_errors[ s_rclient_last_error ];
}
  
RSERVER_API int                RCLIENT_GetErrorState()
{
  if( RClientIsErrorID( s_rclient_last_error ) )
    return 1;
  return 0;
}

RSERVER_API int                RCLIENT_AllocateNewClient()
{
  return CI_AllocateNewClient();
}

RSERVER_API void               RCLIENT_ConnectToServer(int i0, int i1, int i2, int i3, int port, int client_port, int client_id)
{
  CI_InitConnectToServer(i0,i1,i2,i3,port,client_port,client_id);
}

RSERVER_API void               RCLIENT_DisconnectClient(int client_id)
{
  CI_DisconnectClient(client_id);
}

RSERVER_API int                RCLIENT_GetConnectedState(int client_id)
{
  return CI_ClientIsConnected(client_id);
}

RSERVER_API int                RCLIENT_GetPacket(int* raw_message_id, int* game_message_id, char** packet_data, int* bytes, int client_id)
{
  return CI_RecievePacket(raw_message_id, game_message_id, packet_data, bytes, client_id);
}

RSERVER_API int                RCLIENT_FreeLastPacket(int client_id)
{
  return CI_FreeLastPacket(client_id);
}

RSERVER_API int                RCLIENT_SendGamePacket(int game_message_id, char* packet_data, int bytes, int client_id)
{
  int raknet_message_id = RMessage::GetRakNetIndex( RMessage::k_special_game_message_constant );
  return CI_SendPacket( raknet_message_id, game_message_id, packet_data, bytes, client_id );
}

RSERVER_API int                RCLIENT_SendRakNetPacket(int raknet_message_id, char* packet_data, int bytes, int client_id)
{
  return CI_SendPacket( raknet_message_id, -1, packet_data, bytes, client_id );
}

}