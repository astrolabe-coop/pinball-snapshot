/***************************************************************************
                          Engine.cpp  -  description
                             -------------------
    begin                : Wed Jan 26 2000
    copyright            : (C) 2000 by Henrik Enqvist, GPL
    email                : henqvist@excite.com
 ***************************************************************************/

#include <cstdlib>
#include <cmath>
#include <cstring>

#if EM_THREADS
#include <sched.h>
#endif

#include "Private.h"
#include "Engine.h"
#include "Camera.h"
#include "Keyboard.h"
#include "AlignVisitor.h"
#include "AllegroVisitor.h"
#include "AmbientLightVisitor.h"
#include "BehaviorVisitor.h"
#include "CollisionVisitor.h"
#include "OpenGLVisitor.h"
#include "PointLightVisitor.h"
//#include "PNormalVisitor.h"
#include "SoundVisitor.h"
#include "SignalSender.h"
#include "TransformVisitor.h"
#include "Config.h"
#include "SoundUtil.h"
#include "TextureUtil.h"
#include "Profiler.h"

// TODO Remove glu
#if EM_USE_SDL
#include <GL/glu.h>
#endif

#define FPS 50 // req 100 50 33 25 20

volatile int g_iStartTime = -1;
volatile int g_iDesiredTime = -1;

volatile unsigned int g_iLoops = 0;
volatile unsigned int g_iMSeconds = 0;
volatile unsigned int g_iMSecondsFPS = 0;
volatile float g_fFps = 0;

#if EM_DEBUG
#define StartProfile(a) Profiler::getInstance()->startProfile(a)
#define StopProfile() Profiler::getInstance()->stopProfile()
#else
#define StartProfile(a)
#define StopProfile()
#endif

#if EM_THREADS
volatile bool g_bThread = true;;
SDL_mutex* g_Mutex = NULL;
SDL_Thread* g_Thread = NULL;
#endif

#if EM_DEBUG
#if EM_USE_SDL
extern "C" {

Uint32 fctCallBack(Uint32 interval) {
	g_iMSeconds += 100;
	if (g_iMSeconds == 3000) {
		g_fFps = g_fFps*0.5f + 0.5f*(float)g_iLoops*1000.0f / ((float)g_iMSeconds);
		g_iMSeconds = 0;
		g_iLoops = 0;
		cerr << g_fFps << endl;
	}
  return interval;	
}

}
#endif // EM_USE_SDL
#endif // EM_DEBUG

#if EM_USE_ALLEGRO
extern "C" {

void fctCallBack() {
	g_iMSeconds += 10;
	g_iMSecondsFPS += 10;
	if (g_iMSecondsFPS == 3000) {
		g_fFps = g_fFps*0.2f + 0.8f*(float)g_iLoops*1000.0f / ((float)g_iMSecondsFPS);
		g_iMSecondsFPS = 0;
		g_iLoops = 0;
		cerr << g_fFps << endl;
	}
}

END_OF_FUNCTION(fctCallBack)

}
#endif


Engine::Engine(int & argc, char *argv[]) {
	m_Background = NULL;

	Config * config = Config::getInstance();
	config->loadArgs(argc, argv);

	if (!config->useExternGL()) {
#if EM_USE_SDL
		SDL_Init(SDL_INIT_TIMER);
#endif
		TextureUtil::getInstance()->initGrx();
#if EM_DEBUG
#if EM_USE_SDL
		SDL_SetTimer(100, fctCallBack); // 10 ticks per sec
#endif
#if EM_USE_ALLEGRO
		LOCK_VARIABLE(g_iSeconds);
		LOCK_VARIABLE(g_fFps);
		LOCK_VARIABLE(g_iLoops);
		LOCK_FUNCTION(fctCallBack);
		install_int(fctCallBack, 10); // 100 tick per sec
#endif
#endif
	}
	
 	if (config->useSound()) {
 		SoundUtil::getInstance()->initSound();
 	} else {
 		cerr << "**************************************************" << endl;
 		cerr << "Sound turned off in init function. Sound must be" << endl;
 		cerr << "turned on and the game must be restarted to enable" << endl;
 		cerr << "playback." << endl;
 		cerr << "**************************************************" << endl;
	}

  srand((unsigned int)time((time_t *)NULL));
}

Engine::~Engine() {
	this->stopEngine();
}

void Engine::stopEngine() {
#if EM_USE_SDL
	SoundUtil::getInstance()->stopSound();
	TextureUtil::getInstance()->stopGrx();
#endif

#if EM_DEBUG
	extern float em_groups_m, em_shapes_m, em_bounds_m, em_polygons_m;

	cerr << "Collision profile ********************" << endl;
	cerr << "Group-group detections " << em_groups_m << endl;
	cerr << "Shape-shape detections " << em_shapes_m << endl;
	cerr << "Bound-bound detections " << em_bounds_m << endl;
	cerr << "Poly-poly detections " << em_polygons_m << endl;
	cerr << "Polygons *****************************" << endl;
	cerr << "Average polygons " << em_poly_m << endl;
	Profiler::getInstance()->printProfile();
	//	cerr << "Seconds " << ((float)g_iSeconds/FPS) <<" " << ((float)g_iLoops*FPS/g_iSeconds) 
	//			 << " fps" << endl;
	// cerr << "Fps " << (float)(g_iLoops)*1000 / (SDL_GetTicks()-g_iStartTime) << endl;
#endif
}

void Engine::clear() {
	SignalSender::getInstance()->clear();
	PointLightVisitor::getInstance()->clear();
	this->freeObjects();
}

void Engine::addLight(Light* l) {
	PointLightVisitor::getInstance()->add(l);
}

void Engine::setClearColor(float r, float g, float b, float a) {
	TextureUtil::getInstance()->setClearColor(r, g, b, a);
}

void Engine::clearScreen() {
	StartProfile(SWAP);
#if EM_USE_SDL
	glDepthMask(GL_TRUE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1, 1, 1);
	glLoadIdentity();
  //gluLookAt(0,0,0, 0,0,-1, 0,1,0);
#endif

#if EM_USE_ALLEGRO
	clear_zbuffer(zbuffer, 0);
	clear_bitmap(backbuffer);
#endif
	StopProfile();
}

void Engine::setLightning(float diffuse, float ambient) {
 	AmbientLightVisitor::getInstance()->setLightning(diffuse, ambient);
	// light model
}

void Engine::setBackground(EmImage * bg) {
	m_Background = bg;
}

void Engine::setEngineCamera(Group* g) {
	AlignVisitor::getInstance()->setCamera(g);
	TransformVisitor::getInstance()->setCamera(g);
}

void Engine::render() {
	EM_COUT("Engine::render()", 0);
	// Put some overall light.
	StartProfile(GLIGHT);
	AmbientLightVisitor::getInstance()->empty();
	this->accept(AmbientLightVisitor::getInstance());
	StopProfile();
	// Put some light from light sources.
	if (Config::getInstance()->useLights()) {
		StartProfile(PLIGHT);
		PointLightVisitor::getInstance()->empty();
		this->accept(PointLightVisitor::getInstance());
		StopProfile();
	}
	// Align vertices to view space.
	StartProfile(ALIGN);
	AlignVisitor::getInstance()->empty();
	this->accept(AlignVisitor::getInstance());
	StopProfile();
	// Adjust sounds.
	StartProfile(SOUND);
//	p_SoundVisitor->empty();
//	this->accept(p_SoundVisitor);
	StopProfile();
	this->clearScreen();
	// Draw screen
	StartProfile(RENDER);
#if EM_USE_SDL
	OpenGLVisitor::getInstance()->setMode(EM_GL_GCOL_TEX);
	OpenGLVisitor::getInstance()->empty();
	this->accept(OpenGLVisitor::getInstance());
	OpenGLVisitor::getInstance()->setMode(EM_GL_GCOL_TEX_TRANS);
	OpenGLVisitor::getInstance()->empty();
	this->accept(OpenGLVisitor::getInstance());
 	OpenGLVisitor::getInstance()->setMode(EM_GL_CLEAN);
 	OpenGLVisitor::getInstance()->empty();
#endif

#if EM_USE_ALLEGRO
	AllegroVisitor::getInstance()->setMode(EM_ALLEGRO_GCOL_TEX);
	AllegroVisitor::getInstance()->empty();
	this->accept(AllegroVisitor::getInstance());
	AllegroVisitor::getInstance()->setMode(EM_ALLEGRO_GCOL_TEX_TRANS);
 	AllegroVisitor::getInstance()->empty();
 	this->accept(AllegroVisitor::getInstance());
 	AllegroVisitor::getInstance()->setMode(EM_GL_CLEAN);
 	AllegroVisitor::getInstance()->empty();
#endif
	StopProfile();
}

void Engine::swap() {
	StartProfile(SWAP);
	EM_COUT("Engine::swap()", 0);
	g_iLoops++;
	// Draw to screen.
#if EM_USE_SDL
	SDL_GL_SwapBuffers();
	EM_GLERROR(" In Engine::swap ");
#endif

#if EM_USE_ALLEGRO
	blit(backbuffer, screen, 0, 0, 0, 0, Config::getInstance()->getWidth(), Config::getInstance()->getHeight());
#endif
	StopProfile();
}

void Engine::tick() {
	EM_COUT("Engine::tick()", 0);
	if (!Config::getInstance()->useExternGL()) {
		StartProfile(KEY);
		// Check keyboard.
		Keyboard::poll();
		StopProfile();
	}
	// Perform signals.
	StartProfile(SIG);
	SignalSender::getInstance()->tick();
	StopProfile();
	// Perform behaviors.
	StartProfile(BEH);
	BehaviorVisitor::getInstance()->empty();
	this->accept(BehaviorVisitor::getInstance());
	StopProfile();
	// Calculate positions
	StartProfile(TRANS);
	TransformVisitor::getInstance()->empty();
	this->accept(TransformVisitor::getInstance());
	StopProfile();
	// Detect collision.
	StartProfile(COLLISION);
	CollisionVisitor::getInstance()->empty();
	this->accept(CollisionVisitor::getInstance());
	StopProfile();
}

void Engine::delay(int ms) {
#if EM_USE_SDL
	SDL_Delay(ms);
#endif
#if EM_USE_ALLEGRO
	rest(ms);
#endif
}

/* ATTENTION! This function wraps after ~49 days */
bool Engine::limitFPS(int fps) {
	StartProfile(WAIT);
	int delay = 0;
	if (fps > 0) {
		delay = 1000/fps;
	}
#if EM_USE_SDL
	if (g_iStartTime == -1) {
		g_iDesiredTime = g_iStartTime = SDL_GetTicks();
	}
	g_iDesiredTime += delay;
	int time = SDL_GetTicks();
	int realdelay = (g_iDesiredTime - time);
	if (realdelay < -500) {     // really slow
		EM_COUT("TO SLOW", 1);
		g_iDesiredTime = time;
		return true;
	} else if (realdelay < 0) { // slow
		return false;
	} else {                    // to early, must delay
		do {
			realdelay = (g_iDesiredTime - (signed)SDL_GetTicks());
			if (realdelay > 10) {
			// The delay triggers rescheduling
				SDL_Delay(realdelay);
			}
		} while (realdelay > 0);
		return true;
	}
#endif
#if EM_USE_ALLEGRO
	if (g_iStartTime == -1) {
		g_iDesiredTime = g_iStartTime = g_iMSeconds;
	}
	g_iDesiredTime += delay;
	int realdelay = (g_iDesiredTime - g_iMSeconds);
	if (realdelay < -500) {      // really slow
		EM_COUT("TO SLOW", 1);
		g_iDesiredTime = g_iMSeconds;
		return true;
		
	} else if (realdelay < 0) {  // slow
		return false;
	} else {                     // to early, must delay
		do {
			realdelay = (g_iDesiredTime - g_iMSeconds);
			if (realdelay > 10) {
			// The delay triggers rescheduling
				rest(realdelay);
			}
		} while (realdelay > 0);
		return true;
	}
#endif
	StopProfile();
}

#if EM_THREADS
void Engine::renderThreadSafe() {
	this->pauseTickThread();

	EM_COUT("Engine::render()", 0);
	// Put some overall light.
	EM_COUT("Engine::render() glight", 0);
	AmbientLightVisitor::getInstance()->empty();
	this->accept(AmbientLightVisitor::getInstance());

	// Put some light from light sources.
	EM_COUT("Engine::render() plight", 0);
	PointLightVisitor::getInstance()->empty();
	this->accept(PointLightVisitor::getInstance());

	// Align vertices to view space.
	EM_COUT("Engine::render() align", 0);
	AlignVisitor::getInstance()->empty();
	this->accept(AlignVisitor::getInstance());

	// Adjust sounds.
	EM_COUT("Engine::render() sound", 0);
//	p_SoundVisitor->empty();
//	this->accept(p_SoundVisitor);

	this->resumeTickThread();
	this->pauseTickThread();
	// Draw screen
	EM_COUT("Engine::render() opengl", 0);
	this->clearScreen();
	OpenGLVisitor::getInstance()->setMode(EM_GL_GCOL);
	OpenGLVisitor::getInstance()->empty();
	this->accept(OpenGLVisitor::getInstance());
	OpenGLVisitor::getInstance()->setMode(EM_GL_TEX);
	OpenGLVisitor::getInstance()->empty();
	this->accept(OpenGLVisitor::getInstance());

	this->resumeTickThread();
	this->pauseTickThread();

	OpenGLVisitor::getInstance()->setMode(EM_GL_GCOL_TRANS);
	OpenGLVisitor::getInstance()->empty();
	this->accept(OpenGLVisitor::getInstance());
 	OpenGLVisitor::getInstance()->setMode(EM_GL_TEX_TRANS);
 	OpenGLVisitor::getInstance()->empty();
 	this->accept(OpenGLVisitor::getInstance());
 	OpenGLVisitor::getInstance()->setMode(EM_GL_CLEAN);
 	OpenGLVisitor::getInstance()->empty();
	

	EM_COUT("Engine::render() opengltrans", 0);
	//OpenGLTransVisitor::getInstance()->empty();
	//this->accept(OpenGLTransVisitor::getInstance());

	// The keyboard is polled here cause it is not allowed get polled from fctThread
	Keyboard::poll();

	this->resumeTickThread();
}

/* This function is not allowed to call render or event functions.
 * that's why the poll is moved to 'renderThreadSafe()' */
int fctThread(void * data) {
	Engine* engine = (Engine*) data;
	while (g_bThread) {
		//cerr << "tick" << endl;
		if (SDL_mutexP(g_Mutex) == -1) 	cerr << "Error locking mutex" << endl;

		EM_COUT("Engine::tick()", 0);
		
		// Perform signals.
		EM_COUT("Engine::tick() signal", 0);
		SignalSender::getInstance()->tick();
	
		// Perform behaviors.
		EM_COUT("Engine::tick() behavior", 0);
		BehaviorVisitor::getInstance()->empty();
		engine->accept(BehaviorVisitor::getInstance());
		
		// Calculate positions
		EM_COUT("Engine::tick() trans", 0);
		TransformVisitor::getInstance()->empty();
		engine->accept(TransformVisitor::getInstance());
		
		// Detect collision.
		EM_COUT("Engine::tick() collision", 0);
		CollisionVisitor::getInstance()->empty();
		engine->accept(CollisionVisitor::getInstance());
		
		if (SDL_mutexV(g_Mutex) == -1) 	cerr << "Error unlocking mutex" << endl;
		engine->limitFPS(100);
	}
	return 0;
}

void Engine::startTickThread() {
	g_bThread = true;
	if (g_Mutex == NULL) {
		g_Mutex = SDL_CreateMutex();
	} else {
		cerr << "Mutex already created" << endl;
	}
	if (g_Thread == NULL) {
		g_Thread = SDL_CreateThread(fctThread, this);
	} else {
		cerr << "Thread already created" << endl;
	}
}

void Engine::endTickThread() {
	int status;
	g_bThread = false;
	SDL_WaitThread(g_Thread, &status);
	SDL_DestroyMutex(g_Mutex);
	g_Thread = NULL;
	g_Mutex = NULL;
}

void Engine::pauseTickThread() {
	if (g_Mutex != NULL) {
		if (SDL_mutexP(g_Mutex) == -1) 	cerr << "Error unlocking mutex" << endl;
	}
}

void Engine::resumeTickThread() {
	if (g_Mutex != NULL) {
		if (SDL_mutexV(g_Mutex) == -1) 	cerr << "Error unlocking mutex" << endl;
	}
	sched_yield();
}
#endif
