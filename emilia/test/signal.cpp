/***************************************************************************
 Simple test. A cube should be visible in the middle of the screen.
 The arrow keys rotates the cube.
 ***************************************************************************/

#include "Engine.h"
#include "Camera.h"
#include "Cube.h"
#include "Keyboard.h"
#include "Behavior.h"
#include "Shape3D.h"
#include "StateMachine.h"

class SignalTest : public Behavior {
public:
	void StdOnSignal();
	void StdOnCollision() {};
	void StdOnTick() {};
};

void SignalTest::StdOnSignal() {
	OnSignal(1) {
		cerr << "Got signal 1" << endl;
		if (p_Parent != NULL) {
			Shape3D* shape = p_Parent->getShape3D(0);
			if (shape != NULL) shape->setColor(1,1,1,1);
		}
	}
	OnSignal(2) {
		cerr << "Got signal 2" << endl;
		if (p_Parent != NULL) {
			Shape3D* shape = p_Parent->getShape3D(0);
			if (shape != NULL) shape->setColor(1,0,1,1);
		}
	}
}

/** Main */
int main(int argc, char *argv[]) {
	cerr << "Simple emilia test." << endl;

	// Create the engine.
	Engine* engine = new Engine(argc, argv);
	engine->setLightning(0.5f, 0.1f);

	// Add a camera. Move a bit.
	Camera* camera = new Camera();
	Group* groupCamera = new Group();
	engine->add(groupCamera);
	groupCamera->setCamera(camera);
	groupCamera->setTranslation(0, 0, 3);
	engine->setEngineCamera(groupCamera);
	// Add a cube.
	Cube* cube = new Cube(1.0, 1.0, 1.0, 0.0, 1.0);
	Group* groupCube = new Group();
	engine->add(groupCube);
	groupCube->addShape3D(cube);
	
	// Add a behavior to the cube
	SignalTest* beh = new SignalTest();
	groupCube->addBehavior(beh);
		
	while (!Keyboard::isKeyDown(SDLK_ESCAPE)) {
		if (Keyboard::isKeyDown(SDLK_1)) {
			// multicast a signal
			SendSignal(1, 0, engine, NULL);
		}
		if (Keyboard::isKeyDown(SDLK_2)) {
			// multicast a signal
			SendSignal(2, 0, engine, NULL);
		}
		engine->tick();
		engine->render();
		engine->swap();
		SDL_Delay(50);
	}
	delete(engine);
	return 0;
}