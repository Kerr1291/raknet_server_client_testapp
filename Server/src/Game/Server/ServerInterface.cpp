#include <ServerInterface.h>
#include <Server.h>

typedef void (*rserver_param_callback)(int param_id, void* data);
struct RSERVER_PARAM {  const char* _param_desc; rserver_param_callback _callback; };

void RSERVER_BAD_PARAM(int param_id, void* data){};
const int k_num_params = 12;

const RSERVER_PARAM server_params[k_num_params+1] =
{
/*00*/ { "Shutdown Server."                                   , SI_Shutdown_Server }
/*01*/,{ "Start Server."                                      , SI_Start_Server }
/*02*/,{ "Start Dedicated Server."                            , SI_Start_Dedicated_Server }
/*03*/,{ "Get Server Port. (int)"                             , SI_Get_Server_Port }
/*04*/,{ "Set Server Port. (int)"                             , SI_Set_Server_Port }
/*05*/,{ "Get Server Ready State (int, 1 = true)."            , SI_Get_Server_Ready }
/*06*/,{ "Check if server has error. (int, 1 = true)"         , SI_Has_Error }
/*07*/,{ "Get Server error string (const char*)."             , SI_Get_Error }
/*08*/,{ "Get Max Clients. (int)"                             , SI_Process_Parameter }
/*09*/,{ "Set Max Clients. (int)"                             , SI_Process_Parameter }
/*10*/,{ "Get Num Available Connections. (int)"               , SI_Process_Parameter }
/*11*/,{ "Get Num Connected. (int)"                           , SI_Process_Parameter }

//insert new function ids here ONLY

/*?*///,{ "Unknown Command."                                   , SI_Process_Parameter }
/*12*/,{ "Bad Parameter ID."                                  , RSERVER_BAD_PARAM }
};

extern "C"
{

RSERVER_API int               RSERVER_GetNumParams()
{
  return k_num_params;
}

RSERVER_API const char*       RSERVER_GetParamDesc(int param_id)
{
  if( param_id >= k_num_params || param_id < 0 )
    return server_params[k_num_params]._param_desc;
  return server_params[param_id]._param_desc;
}

RSERVER_API void              RSERVER_ProcessParameter(int param_id, void* data)
{
  if( param_id >= k_num_params || param_id < 0 )
    return;
  
  server_params[param_id]._callback(param_id, data);
}

RSERVER_API void              RSERVER_ProcessOutParameter(int param_id, void* data)
{
	if (param_id >= k_num_params || param_id < 0)
		return;

	server_params[param_id]._callback(param_id, data);
}

RSERVER_API int               RSERVER_GetServerGamePort()
{
  int port;
  SI_Get_Server_Port(-1,&port);
  return port;
}



}//end extern c