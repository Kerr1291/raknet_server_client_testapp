#pragma once

/// //////////////////
//
// Server interface for the game
//
//
//
/// //////////////////

#include <BuildSettings.h>

extern "C"
{

RSERVER_API int               RSERVER_GetNumParams();

RSERVER_API const char*       RSERVER_GetParamDesc(int param_id);

RSERVER_API void              RSERVER_ProcessParameter(int param_id, void* data = 0);

RSERVER_API void              RSERVER_ProcessOutParameter(int param_id, void* data = 0);

RSERVER_API int               RSERVER_GetServerGamePort();

}