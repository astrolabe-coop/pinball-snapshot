/***************************************************************************
                          Pinball.cpp  -  description
                             -------------------
    begin                : Thu Mar 9 2000
    copyright            : (C) 2000 by Henrik Enqvist
    email                : henqvist@excite.com
***************************************************************************/

#include <fstream>
#include <string>
#include <strstream>

#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <cstdlib>
#include <ctime>

#ifdef _MSC_VER //!+rzr : maybe we can use a portable IO lib such as physicfs
#include <io.h> //find
#include <direct.h> //getcwd
#else
#include <dirent.h>
#endif 

#include "Private.h" // macro flags defined here

#ifdef HAVE_UNISTD_H 
#include <unistd.h> // not in msvc
#endif //!-rzr


#include "Pinball.h"
#include "Keyboard.h"
#include "Menu.h"
#include "Engine.h"
#include "Camera.h"
#include "Light.h"
#include "BigSphere.h"
#include "Cylinder.h"
#include "BounceBehavior.h"
#include "KeyBehavior.h"
#include "Grid.h"
#include "KeyRotBehavior.h"
#include "Polygon.h"
#include "TextureUtil.h"
#include "SoundUtil.h"
#include "Score.h"
#include "CollisionBounds.h"
#include "StateMachine.h"
#include "EyeBehavior.h"
#include "Config.h"
#include "EmFont.h"
#include "Loader.h"
#include "BallGroup.h"
#include "Table.h"

#if EM_USE_SDL
#include <SDL.h>
#endif

void get_config(void);

/****************************************************************************
 * Menus
 ***************************************************************************/

MenuChoose* menusnd = NULL;
MenuChoose* menumusic = NULL;
MenuChoose* menubright = NULL;
MenuChoose* menuscreen = NULL;
MenuChoose* menusize = NULL;
MenuChoose* menuview = NULL;
MenuChoose* menufilter = NULL;
MenuChoose* menufps = NULL;
MenuChoose* menufire = NULL;

/****************************************************************************
 * Define some menu classes
 ***************************************************************************/

/** Menu item for changing a key binding. */
class MyMenuKey : public MenuFct {
public:
  MyMenuKey(const char * name, int (*fct)(void), Engine *e) : MenuFct(name, fct, e) {};
protected:
  int perform () {
    p_Engine->clearScreen();
    p_EmFont->printRowCenter("press a key", 10);
    p_Engine->swap();
    EMKey key = Keyboard::waitForKey();
    string name(m_Name);
    Config::getInstance()->setKey(name, key);
    return EM_MENU_NOP;
  }
  const char * getText() {
    ostrstream stm;
    stm.clear();
    string name(m_Name);
    string key(Config::getInstance()->getKeyCommonName(Config::getInstance()->getKey(name)));
    name = name + ":";
    while (name.size() < 12) {
      name = name + " ";
    }
    while (key.size() < 12) {
      key = " " + key;
    }
    stm << name << key << '\0';
    string text = stm.str();
    return text.c_str();
  }    
};


/** The apply meny item. */	
class MyMenuApply : public MenuFct {
public:
  MyMenuApply(const char * name, int (*fct)(void), Engine *e) :	MenuFct(name, fct, e) {};
protected:
  int perform () {
    Config* config = Config::getInstance();
    TextureUtil * textureutil = TextureUtil::getInstance();
    // sound
    switch (menusnd->getCurrent()) {
    case 0: config->setSound(0); break;
    case 1: config->setSound(1); break;
    case 2: config->setSound(2); break;
    case 3: config->setSound(3); break;
    case 4: config->setSound(4); break;
    case 5: config->setSound(5); break;
    case 6: config->setSound(6); break;
    case 7: config->setSound(7); break;
    case 8: config->setSound(8); break;
    }
    switch (menumusic->getCurrent()) {
    case 0: config->setMusic(0); break;
    case 1: config->setMusic(1); break;
    case 2: config->setMusic(2); break;
    case 3: config->setMusic(3); break;
    case 4: config->setMusic(4); break;
    case 5: config->setMusic(5); break;
    case 6: config->setMusic(6); break;
    case 7: config->setMusic(7); break;
    case 8: config->setMusic(8); break;
    }
    SoundUtil::getInstance()->applyConfigVolume();
    // fullscreen
    if (menuscreen->getCurrent() == 0) {
      if (config->useFullScreen() == false) {
#if EM_USE_SDL
	SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
#endif
      }
      config->setFullScreen(true);
    } else {
      if (config->useFullScreen() == true) {
#if EM_USE_SDL
	SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
#endif
      }
      config->setFullScreen(false);
    }
    // brightness
    switch (menubright->getCurrent()) {
    case 0: menubright->getEngine()->setLightning(0, 0.1f); 
      Config::getInstance()->setBrightness(0); break;
    case 1: menubright->getEngine()->setLightning(0, 0.2f);
      Config::getInstance()->setBrightness(0.2f); break;
    case 2: menubright->getEngine()->setLightning(0, 0.4f);
      Config::getInstance()->setBrightness(0.4f); break;
    default: menubright->getEngine()->setLightning(0, 0.2f);
      Config::getInstance()->setBrightness(0.2f); break;
    }
    // screen size
    int w, h;
    switch (menusize->getCurrent()) {
    case 0: w = 320; h = 240; break;
    case 1: w = 400; h = 300; break;
    case 2: w = 512; h = 384; break;
    case 3: w = 640; h = 480; break;
    case 4: w = 800; h = 600; break;
    case 5: w = 1024; h = 768; break;
    default: w = 640; h = 480;
    }
    if (config->getWidth() != w) {
#ifdef WIN32  //!+-rzr 
      //cout<<("Workround bug (for WIN32) + macosx etc");
      textureutil->freeTextures();
#endif
#if EM_USE_SDL
      SDL_SetVideoMode(w, h, config->getBpp(), 
		       SDL_OPENGL | (config->useFullScreen() ? SDL_FULLSCREEN : 0));

#endif // SDL
      textureutil->resizeView(w, h);
#ifdef WIN32
      string filename = Config::getInstance()->getDataDir() + string("/font_34.png");
      //cout<<filename<<endl;
      EmFont::getInstance()->loadFont(filename.c_str());
#endif //!-rzr
    }
    config->setSize(w, h);
		
    switch (menuview->getCurrent()) {
    case 0: config->setView(0); break;
    case 1: config->setView(1); break;
    default: config->setView(2);
    }
    // texture filter
    if (menufilter->getCurrent() == 0) {
      config->setGLFilter(EM_LINEAR);
    } else if (menufilter->getCurrent() == 1) {
      config->setGLFilter(EM_NEAREST);
    } else {
      config->setGLFilter(-1);
    }
    // fps
    if (menufps->getCurrent() == 0) {
      config->setShowFPS(false);
    } else {
      config->setShowFPS(true);
    }
    // fire
    if (menufire->getCurrent() == 0) {
      config->setFire(false);
      for (int a=0; a<MAX_BALL; ++a) {
	BallGroup * bg = Table::getInstance()->getBall(a);
	if (bg != NULL) {
	  Behavior * beh = bg->getBehavior();
	  EmAssert(beh != NULL, "MyMenuApply::perform behavior NULL");
	  EmAssert(beh->getType() == PBL_TYPE_BOUNCEBEH, 
		   "MyMenuApply::perform behavior not bouncebehavior");
	  ((BounceBehavior*)beh)->setFire(false);
	}
      }
    } else {
      config->setFire(true);
      for (int a=0; a<MAX_BALL; ++a) {
	BallGroup * bg = Table::getInstance()->getBall(a);
	if (bg != NULL) {
	  Behavior * beh = bg->getBehavior();
	  EmAssert(beh != NULL, "MyMenuApply::perform behavior NULL");
	  EmAssert(beh->getType() == PBL_TYPE_BOUNCEBEH, 
		   "MyMenuApply::perform behavior not bouncebehavior");
	  ((BounceBehavior*)beh)->setFire(true);
	}
      }
    }
    get_config();
    return EM_MENU_NOP;
  }
};

/** The back button. */
class MyMenuCancel : public MenuFct {
public:
  MyMenuCancel(const char * name, int (*fct)(void), Engine *e) : MenuFct(name, fct, e) {};
protected:
  int perform () {
    get_config();
    return EM_MENU_BACK;
  }
};

/** Button for loading a table. */
class MyMenuLoad : public MenuFct {
public:
  MyMenuLoad(const char * name, int (*fct)(void), Engine *e) : MenuFct(name, fct, e) {
		
  };
protected:
  int perform () {
    p_Engine->clearScreen();
    p_EmFont->printRowCenter("LOADING", 10);
    p_Engine->swap();

    if (Table::getInstance()->loadLevel(p_Engine, m_Name) == 0) {
      p_Engine->clearScreen();
      p_EmFont->printRowCenter("OK", 10);
      p_Engine->swap();
      p_Engine->delay(500);
    } else {
      p_Engine->clearScreen();
      p_EmFont->printRowCenter("ERROR", 10);
      p_Engine->swap();
      p_Engine->delay(1000);
    }
    return EM_MENU_BACK;
  }
};


/****************************************************************************
 * Menu functions
 ***************************************************************************/

/** Update the current meny with the configuration. */
void get_config(void) {
  // sound
  switch (Config::getInstance()->getSound()) {
  case 0: menusnd->setCurrent(0); break;
  case 1: menusnd->setCurrent(1); break;
  case 2: menusnd->setCurrent(2); break;
  case 3: menusnd->setCurrent(3); break;
  case 4: menusnd->setCurrent(4); break;
  case 5: menusnd->setCurrent(5); break;
  case 6: menusnd->setCurrent(6); break;
  case 7: menusnd->setCurrent(7); break;
  case 8: menusnd->setCurrent(8); break;
  }
  switch (Config::getInstance()->getMusic()) {
  case 0: menumusic->setCurrent(0); break;
  case 1: menumusic->setCurrent(1); break;
  case 2: menumusic->setCurrent(2); break;
  case 3: menumusic->setCurrent(3); break;
  case 4: menumusic->setCurrent(4); break;
  case 5: menumusic->setCurrent(5); break;
  case 6: menumusic->setCurrent(6); break;
  case 7: menumusic->setCurrent(7); break;
  case 8: menumusic->setCurrent(8); break;
  }
  // fullscreen
  if (Config::getInstance()->useFullScreen()) {
    menuscreen->setCurrent(0);
  } else {
    menuscreen->setCurrent(1);
  }
  // brightness
  if (Config::getInstance()->getBrightness() < 0.19f) {
    menubright->setCurrent(0);
  } else 	if (Config::getInstance()->getBrightness() > 0.21f) {
    menubright->setCurrent(2);
  } else {
    menubright->setCurrent(1);
  }
  // screen size
  if (Config::getInstance()->getWidth() == 320) {
    menusize->setCurrent(0);
  } else 	if (Config::getInstance()->getWidth() == 400) {
    menusize->setCurrent(1);
  } else 	if (Config::getInstance()->getWidth() == 512) {
    menusize->setCurrent(2);
  } else 	if (Config::getInstance()->getWidth() == 640) {
    menusize->setCurrent(3);
  } else 	if (Config::getInstance()->getWidth() == 800) {
    menusize->setCurrent(4);
  } else 	if (Config::getInstance()->getWidth() == 1024) {
    menusize->setCurrent(5);
  } else {
    menusize->setCurrent(0);
  }
  // view mode
  switch(Config::getInstance()->getView()) {
  case 0: menuview->setCurrent(0); break;
  case 1: menuview->setCurrent(1); break;
  default: menuview->setCurrent(2);
  }
  // texture filter
  if (Config::getInstance()->getGLFilter() == EM_LINEAR) {
    menufilter->setCurrent(0);
  } else if (Config::getInstance()->getGLFilter() == EM_NEAREST) {
    menufilter->setCurrent(1);
  } else {
    menufilter->setCurrent(2);
  }
  // show fps
  if (Config::getInstance()->getShowFPS()) {
    menufps->setCurrent(1);
  } else {
    menufps->setCurrent(0);
  }
  // fire
  if (Config::getInstance()->getFire()) {
    menufire->setCurrent(1);
  } else {
    menufire->setCurrent(0);
  }
}

/* Create some menus */
MenuItem* createMenus(Engine * engine) {
  // Create the meny
  MenuSub* menu = new MenuSub("menu", engine);

  MenuSub* menuresume = new MenuSub("play", engine);
  menuresume->setAction(EM_MENU_RESUME);
  menu->addMenuItem(menuresume);

  MenuSub* menuload = new MenuSub("load table", engine);
  menu->addMenuItem(menuload);

  MenuSub* menucfg = new MenuSub("config", engine);
  menu->addMenuItem(menucfg);

  MenuSub* menuexit = new MenuSub("exit", engine);
  menuexit->setAction(EM_MENU_EXIT);
  menu->addMenuItem(menuexit);

  MenuSub* menugfx = new MenuSub("graphics", engine);
  menucfg->addMenuItem(menugfx);

  MenuSub* menuaudio = new MenuSub("audio", engine);
  menucfg->addMenuItem(menuaudio);

  MenuSub* menukey = new MenuSub("keyboard", engine);
  menucfg->addMenuItem(menukey);

  string filename = string(Config::getInstance()->getDataSubDir()) + "/splash.png";
  EmTexture * tex = TextureUtil::getInstance()->loadTexture(filename.c_str());
  if (tex != NULL) {
		menu->setBackground(tex);
		menuload->setBackground(tex);
		menucfg->setBackground(tex);
		menugfx->setBackground(tex);
		menuaudio->setBackground(tex);
		menukey->setBackground(tex);
	} else {
		cerr << "Error loading data/splash.png" << endl;
	}

  // create one entry for each directory
  // TODO scrolling text if to many tables
#ifdef _MSC_VER //!+rzr : thanx to ramlaid ;)
  struct _finddata_t dirFile;
  long hFile=0;
  char cwd[256];
  if (getcwd(cwd,256) != NULL)  {
    chdir(Config::getInstance()->getDataDir());
    if( (hFile = _findfirst( "*", &dirFile )) != NULL ) {
      do {
        if ((dirFile.attrib & _A_SUBDIR) != 0) 	{
          if (strcmp(".", dirFile.name) != 0 
              && strcmp("..", dirFile.name) != 0) {
            MenuFct* menufct = new MyMenuLoad(dirFile.name, NULL, engine);
            menuload->addMenuItem(menufct);
          }
        }
      }
      while( _findnext( hFile, &dirFile ) == 0 );
      chdir(cwd);
      _findclose( hFile );
    }
  }
#else
  DIR * datadir = opendir(Config::getInstance()->getDataDir());
  char cwd[256];
  if (datadir != NULL && getcwd(cwd, 256) != NULL) {
    struct dirent * entry;
    struct stat statbuf;
    //cerr<<  Config::getInstance()->getDataDir() <<endl; //!+rzr
    chdir(Config::getInstance()->getDataDir());
    while ((entry = readdir(datadir)) != NULL) {
      lstat(entry->d_name, &statbuf);
      if (S_ISDIR(statbuf.st_mode) &&
	  strcmp(".", entry->d_name) != 0 &&
	  strcmp("..", entry->d_name) != 0) {
	MenuFct* menufct = new MyMenuLoad(entry->d_name, NULL, engine);
	menuload->addMenuItem(menufct);
      }
    }
    chdir(cwd);
    closedir(datadir);
  }
#endif //!+rzr
  menuview = new MenuChoose(engine);
  menuview->addText(  "view:        standard");
  menuview->addText(  "view:             top");
  menuview->addText(  "view:          locked");
  menugfx->addMenuItem(menuview);

  menuscreen = new MenuChoose(engine);
  menuscreen->addText("screen:    fullscreen");
  menuscreen->addText("screen:      windowed");
  menugfx->addMenuItem(menuscreen);

  menubright = new MenuChoose(engine);
  menubright->addText("brightness:       low");
  menubright->addText("brightness:    medium");
  menubright->addText("brightness:      high");
  menugfx->addMenuItem(menubright);

  menusize = new MenuChoose(engine);
  menusize->addText(  "screen size:  340x240");
  menusize->addText(  "screen size:  400x300");
  menusize->addText(  "screen size:  512x384");
  menusize->addText(  "screen size:  640x480");
  menusize->addText(  "screen size:  800x600");
  menusize->addText(  "screen size: 1024x768");
  menugfx->addMenuItem(menusize);

  menufilter = new MenuChoose(engine);
  menufilter->addText("texture:       linear");
  menufilter->addText("texture:      nearest");
  menufilter->addText("texture:         none");
  menugfx->addMenuItem(menufilter);

  menufps = new MenuChoose(engine);
  menufps->addText(   "show fps:          no");
  menufps->addText(   "show fps:         yes");
  menugfx->addMenuItem(menufps);

  menufire = new MenuChoose(engine);
  menufire->addText(  "fire effect:       no");
  menufire->addText(  "fire effect:      yes");
  menugfx->addMenuItem(menufire);

  //   menumaxfps = new MenuChoose(engine);
  //   menumaxfps->addText("max fps            50");
  //   menumaxfps->addText("max fps           100");
  //   menugfx->addMenuItem(menumaxfps);

  menusnd = new MenuChoose(engine);
  menusnd->addText(   "sound:            off");
  menusnd->addText(   "sound:       =.......");
  menusnd->addText(   "sound:       ==......");
  menusnd->addText(   "sound:       ===.....");
  menusnd->addText(   "sound:       ====....");
  menusnd->addText(   "sound:       =====...");
  menusnd->addText(   "sound:       ======..");
  menusnd->addText(   "sound:       =======.");
  menusnd->addText(   "sound:       ========");
  menuaudio->addMenuItem(menusnd);

  menumusic = new MenuChoose(engine);
  menumusic->addText( "music:            off");
  menumusic->addText( "music:       =.......");
  menumusic->addText( "music:       ==......");
  menumusic->addText( "music:       ===.....");
  menumusic->addText( "music:       ====....");
  menumusic->addText( "music:       =====...");
  menumusic->addText( "music:       ======..");
  menumusic->addText( "music:       =======.");
  menumusic->addText( "music:       ========");
  menuaudio->addMenuItem(menumusic);

  MyMenuKey * menuleftflip = new MyMenuKey("leftflip", NULL, engine);
  menukey->addMenuItem(menuleftflip);
  MyMenuKey * menurightflip = new MyMenuKey("rightflip", NULL, engine);
  menukey->addMenuItem(menurightflip);
  MyMenuKey * menuleftnudge = new MyMenuKey("leftnudge", NULL, engine);
  menukey->addMenuItem(menuleftnudge);
  MyMenuKey * menurightnudge = new MyMenuKey("rightnudge", NULL, engine);
  menukey->addMenuItem(menurightnudge);
  MyMenuKey * menubottomnudge = new MyMenuKey("bottomnudge", NULL, engine);
  menukey->addMenuItem(menubottomnudge);
  MyMenuKey * menulaunch = new MyMenuKey("launch", NULL, engine);
  menukey->addMenuItem(menulaunch);
  MyMenuKey * menureset = new MyMenuKey("reset", NULL, engine);
  menukey->addMenuItem(menureset);

  MenuFct* menuapply = new MyMenuApply("apply", NULL, engine);
  menuaudio->addMenuItem(menuapply);
  //menucfg->addMenuItem(menuapply);
  menugfx->addMenuItem(menuapply);
  //menukey->addMenuItem(menuapply);

  MenuFct* menucancel = new MyMenuCancel("back", NULL, engine);
  menucfg->addMenuItem(menucancel);
  menugfx->addMenuItem(menucancel);
  menuaudio->addMenuItem(menucancel);
  menuload->addMenuItem(menucancel);
  menukey->addMenuItem(menucancel);

  get_config();
  return menu;
}


/** Main */
int main(int argc, char *argv[]) {
  //cerr<<"+ Pinball::main"<<endl;
  try {
    // Create a engine and parse emilia arguments
    Config::getInstance()->loadConfig();
    Engine * engine = new Engine(argc, argv);
    
    float direct, ambient;
    if (Config::getInstance()->useLights()) {
      direct = 0.0f;
    } else {
      direct = 0.5f;
    }
    if (Config::getInstance()->getBrightness() < 0.19f) {
      ambient = 0.1f;
    } else if (Config::getInstance()->getBrightness() > 0.21f) {
      ambient = 0.4f;
    } else {
      ambient = 0.2f;
    }
    engine->setLightning(direct, ambient);
    
#if EM_USE_SDL
    string filename = Config::getInstance()->getDataDir() + string("/font_34.png");
#endif
#if EM_USE_ALLEGRO
    string filename = Config::getInstance()->getDataDir() + string("/font_35.pcx");
#endif
    
    EmFont::getInstance()->loadFont(filename.c_str());
    
    // Add a score board and a menu.
    MenuItem* menu = createMenus(engine);
    
    // Draw to the screen.
    int all = 0;
    engine->resetTick();
    while (!Keyboard::isKeyDown(SDLK_INSERT)) {
#if EM_DEBUG
      if (Keyboard::isKeyDown(SDLK_p)) {
	Keyboard::waitForKey();
	Keyboard::clear();
	engine->resetTick();
      }
#endif
      if (Keyboard::isKeyDown(SDLK_ESCAPE) || all == 0) {
	SoundUtil::getInstance()->pauseMusic();
	if (menu->perform() == EM_MENU_EXIT) {
	  break;
	}
	engine->resetTick();
	SoundUtil::getInstance()->resumeMusic();
      }
      
      if (Keyboard::isKeyDown(SDLK_r)) {
	SendSignal(PBL_SIG_RESET_ALL, 0, engine, NULL);
      }
      
      if (engine->nextTickFPS(200)) {
	engine->tick(); 
      } else {
	engine->render();
	if (Table::getInstance()->getScore() != NULL) {
	  Table::getInstance()->getScore()->draw();
	}
	if (engine->getGroup(0) == NULL) {
	  EmFont::getInstance()->printRowCenter("no table loaded", 6);
	  EmFont::getInstance()->printRowCenter("press esc", 8);
	}
	engine->swap();
      }
      all++;
      //engine->limitFPS(100);
    }
    
    Config::getInstance()->saveConfig();
    
    delete(engine);
  } catch (string str) {
    cerr << "EXCEPTION: " << str << endl;
  }
  return 0;
}

#if EM_USE_ALLEGRO
END_OF_MAIN();
#endif
