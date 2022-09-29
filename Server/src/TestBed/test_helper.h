#pragma once

#include <BuildSettings.h>

#ifdef LIBRARY_EXPORTS
#else

#include <string>

void SetInternalID(int id);
void SetName(const std::string& name);
void SetPlayer(int x, int y);
void ClearMsg();
void SetMsg(const std::string& msg);
void SetMapData(char* data, int w, int h);
void SetQuitBool( bool* quitbool );
void InitTestRender();
void RenderGame();
void RunPlayer();
void EndTestRender();

void RespondToMessage(int message, char* data, int bytes);

void KeyMessages(int key);


#endif