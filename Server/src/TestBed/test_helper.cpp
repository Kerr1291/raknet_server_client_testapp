 
#include <test_helper.h>

#ifdef LIBRARY_EXPORTS
#else


#include <iostream>

#ifdef _WIN32
const int stdscr = 0;
void mvprintw( int a, int b, const char* c ) {}
void initscr() {}
void noecho() {}
void nonl() {}
void refresh() {}
void curs_set( bool b ) {}
void keypad( int k, bool b ) {}
void clear() {}
const int KEY_UP = 0;
const int KEY_DOWN = 0;
const int KEY_LEFT = 0;
const int KEY_RIGHT = 0;
int getch() {}
void endwin() {}
#else

#include <ncurses.h>
#include <curses.h>
#include <term.h>
#include <unistd.h>

#endif

#include <memory.h>
#include <string>


#include <algorithm>

#include <helpers.h>


#include <ServerInterface.h>
#include <ClientInterface.h>

#include <RakSleep.h>
#include <RakThread.h>

#include <ctime>

#include <RProto.pb.h>

bool locked_map = false;
char* global_data = 0;

int px = 0;
int py = 0;

int w = 0;
int h = 0;

int test_client_id = 0;

std::string test_name;
std::string msg;

bool* quitbool = 0;

int offsetx = 10;
int offsety = 10;

int minx = 0;
int miny = 0;

void renderwindow()
{
  if( locked_map )
    return;
  
  if(!global_data)
    return;
  
  for(int j = 0; j < h; ++j)
  {
    if(!global_data)
      return;
    
    for(int i = 0; i < w; ++i)
    {
      if(!global_data)
        return;
      
      if( global_data[j*w + i] == 2 )
        mvprintw(offsety+j, offsetx+i, "."); 
      else if( global_data[j*w + i] == 1 )
        mvprintw(offsety+j, offsetx+i, "#"); 
      else
      {
        mvprintw(offsety+j, offsetx+i, " "); 
      }
    }
  }
}


void SetQuitBool( bool* qb )
{
  quitbool = qb;
}

int clear_delay = 300;
int delay = 0;

void SetInternalID(int id)
{
  test_client_id = id;
}

void SetName(const std::string& name)
{
  test_name = name;
}

void SetPlayer(int x, int y)
{
  px = x;
  py = y;
}

void SetMsg(const std::string& m)
{
  msg = m;
  delay = 0;
}

void ClearMsg()
{
  msg.clear();
}

void SetMapData(char* data, int w, int h)
{
  locked_map = true;
  
  if( global_data )
  {
    std::cerr<<"deleting old data\n";
    char* old = global_data;
    global_data = 0;
    delete old;
  }
  
  char* nd = new char[w*h];
  memcpy( nd, data, w*h );
  
  global_data = nd;

  locked_map = false;
}

void InitTestRender()
{
  initscr();
  noecho();
  nonl();
  refresh();
  curs_set(false);
  keypad(stdscr,true);
}


void RenderGame()
{
  clear(); 
  renderwindow();
  mvprintw(offsety+py-miny, offsetx+px-minx, "@"); 
  if(!msg.empty())
  {
    mvprintw(h + 2,5,msg.c_str());
    delay++;
    if( delay > clear_delay )
      ClearMsg();
  }
  refresh();
  RakSleep(33);
}


void RunPlayer()
{
  int ch = getch();
  KeyMessages(ch);
}


void EndTestRender()
{
  delete global_data;
  endwin();
}


void RespondToMessage(int message, char* data, int bytes)
{
  std::string msg_to_send;
  
  if( message == rproto::msg_fov_data )
  {
    rproto::View view_msg;
    view_msg.ParseFromArray(data, bytes);
    
    int radius = view_msg.radius();
    
    w = 2 * radius+1;
    h = 2 * radius+1;
    
    std::cerr<<"size = "<<w*h <<"\n";
    
    char* ndata = new char[ w*h ];
    memset( ndata, 0, w*h );
    
    minx = 999999;
    miny = 999999;
    
    for( int i = 0; i < view_msg.cells_size(); ++i )
    {
      const rproto::Cell& c = view_msg.cells(i);
      if( c.cx() < minx )
        minx = c.cx();
      if( c.cy() < miny )
        miny = c.cy();
    }
    
    for( int i = 0; i < view_msg.cells_size(); ++i )
    {
      const rproto::Cell& c = view_msg.cells(i);
      int real_x = c.cx() - minx;
      int real_y = c.cy() - miny;
      
      std::cerr<<"realx = "<<real_x;
      std::cerr<<"  realy = "<<real_y <<"\n";
      
      bool has_wall = false;
      bool has_floor = false;
      for( int j = 0; j < c.ents_size(); ++j )
      {        
        if( c.ents(j).type_id() == 1 )
          has_wall = true;
        
        if( c.ents(j).type_id() == 0 )
          has_floor = true;
      }
      
      if( has_wall )
        ndata[real_y*w + real_x] = 1;
      else if( has_floor )
        ndata[real_y*w + real_x] = 2;
      else
      {
        //ndata[real_y*w + real_x] = 5;
      }
    }
    
    SetMapData(ndata,w,h);
    
    std::cerr<<"cleaning up temp data\n";
    delete ndata;
  }
  
  if( message == rproto::msg_player_pos )
  {
    rproto::PlayerPos pos;
    pos.ParseFromArray(data, bytes);
    px = pos.cx();
    py = pos.cy();
    
    std::stringstream where;
    where <<"player at: "<<px <<" , " <<py <<"\n";
    SetMsg( where.str() );
  }
}

void KeyMessages(int key)
{
  std::string msg_to_send;
  
  int oldx = px;
  int oldy = py;
  
  if(key == KEY_UP)
    oldy--;
  else
  if(key == KEY_DOWN)
    oldy++;
  else
  if(key == KEY_RIGHT)
    oldx++;
  else
  if(key == KEY_LEFT)
    oldx--;
  
  if(key == 'q')
  {
    *quitbool = false;
  }
  else
  {
    float dx = oldx - px;
    float dy = oldy - px;
    
    //dx *= 0.5f;
    //dy *= 0.5f;
    
    rproto::PlayerMoveCommand pmove;
    pmove.set_dx( dx );
    pmove.set_dy( dy );
    pmove.set_name( test_name );
    pmove.SerializeToString( &msg_to_send );
    RCLIENT_SendGamePacket(rproto::msg_player_move, const_cast<char*>(msg_to_send.c_str()), msg_to_send.size(), test_client_id);
  }
}


#endif




















