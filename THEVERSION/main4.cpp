#include "gekrender.h"
using namespace gekRender; // For convenience... don't do this in your
							   // programs normally

// This allows us to easily interface with your computer!
IODevice* myDevice = new IODevice();

// Variables used for creating the window
int WIDTH = 640;
int HEIGHT = 480;

void init() {
	// Bare Minimum to open a window...
	myDevice->initGLFW();
	myDevice->pushWindowCreationHint(GLFW_RESIZABLE,
									 GLFW_FALSE);			  // The next window will be created with the hints we push
															  // on!
	myDevice->addWindow(WIDTH, HEIGHT, "Hello World Window"); // Create the
															  // window!
	myDevice->setContext(0);								  // Binds GL context of window 0 to current thread
	myDevice->initGL3W();									  // Initialize the OpenGL extension wrangler.
															  // At this point the OpenGL Context as been established and we are ready to
															  // use OpenGL Functions!
}
void initObjects() {}
void checkInput() {}
void Update() {}
void DrawScene() {}

int main() {
	init();
	std::cout << "Hello World!";

	while (!myDevice->shouldClose(0)) {

		myDevice->pollevents();
		myDevice->swapBuffers(0);
	}

	return 0;
}
