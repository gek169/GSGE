/*
(C) DMHSW 2020
Starter file for GSGE projects. Recommended you work from this base.
*/
// using namespace gekRender;

#include "FontRenderer.h"
#include "gekrender.h"
// My AL Utils
#include "GekAL.h"
// Engine
#include "GkEngine.h"

// C stuff
#include <chrono>
#include <clocale>
#include <cstdlib>

using namespace GSGE;

int WIDTH = 800, HEIGHT = 600;
bool FULLSCREEN = false;
float SCALE = 1;
class GkEngine2 : public GkEngine { // You implement GkEngine by writing a derivative class
									// of it and implementing the functions.
  public:
	GkEngine2(uint window_width, uint window_height, float scale, bool resizeable, bool fullscreen, const char* title, bool _WBOIT, bool softbodies,
			  uint samples)
		: GkEngine(window_width, window_height, scale, resizeable, fullscreen, title, softbodies, _WBOIT, samples) {}
	~GkEngine2();
	void init_game() override;
	void tick_game() override;
	void displayLoadingScreen(double percentage) override;
	bool GSGELineInterpreter(std::stringstream& file, std::string& line, std::vector<std::string>& tokens, bool isTemplate, GkObject* Template,
							 Transform& initTransform, GkObject*& RetVal, int& error_flag, IndexedModel& lastModel,
							 Mesh*& lastMesh, std::string& lastAnimationName, bool& RetValOwnsLastMesh, Texture*& lastTexture, CubeMap*& lastCubeMap,
							 MeshInstance*& lastMeshInstance, btRigidBody*& lastRigidBody, btCollisionShape*& lastCollisionShape,
							 std::vector<btCollisionShape*>& ShapesInOrder, std::vector<Mesh*>& MeshesInOrder, float& Scaling) override;
	void setPauseState(bool pauseState);
	//~ Shader* ParticleShader = nullptr;
//	BulletPhysicsThreadProcessor bullet_thread;
//	GkUIRenderingThread UI_thread;
	float felapsed = 0;
};
GkEngine2* GameEngine;

bool GkEngine2::GSGELineInterpreter(std::stringstream& file, std::string& line, std::vector<std::string>& tokens, bool isTemplate, GkObject* Template,
	Transform& initTransform, GkObject*& RetVal, int& error_flag, IndexedModel& lastModel,
	Mesh*& lastMesh, std::string& lastAnimationName, bool& RetValOwnsLastMesh, Texture*& lastTexture, CubeMap*& lastCubeMap,
	MeshInstance*& lastMeshInstance, btRigidBody*& lastRigidBody, btCollisionShape*& lastCollisionShape,
	std::vector<btCollisionShape*>& ShapesInOrder, std::vector<Mesh*>& MeshesInOrder, float& Scaling) {
	
	return false;
}

class GameControls : public GkControlInterface {
  public:
	GameControls() {}
	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) override {
		
	}

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) override {
	
	}

	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
								  glm::vec2 NormalizedCursorDelta) override {
		
	}
};
GameControls GameController;

void window_size_callback_caller(GLFWwindow* window, int width, int height) {
	GameEngine->window_size_callback(window, width, height);
	WIDTH = width;
	HEIGHT = height;
}

void g_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) { GameEngine->key_callback(window, key, scancode, action, mods); }
void g_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) { GameEngine->mouse_button_callback(window, button, action, mods); }
void g_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) { GameEngine->cursor_position_callback(window, xpos, ypos); }
void g_char_callback(GLFWwindow* window, unsigned int codepoint) { GameEngine->char_callback(window, codepoint); }
void g_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { GameEngine->scroll_callback(window, xoffset, yoffset); }
void g_processJoysticks(){
	int ncount;
	const unsigned char* buttons;
	const float* axes;
	for(int i = GLFW_JOYSTICK_1; GameEngine->getDevice()->getjoyStickPresent(i) && i < 5; i++){
		axes = GameEngine->getDevice()->getJoystickAxes(i,&ncount);
		GameEngine->joystickAxes_callback(i, axes, ncount);
		buttons = GameEngine->getDevice()->getJoystickButtons(i, &ncount);
		GameEngine->joystickButtons_callback(i, buttons, ncount);
	}
}
void CustomTransparentRender(int meshmask, FBO* CurrentRenderTarget, FBO* RenderTarget_Transparent, Camera* CurrentRenderCamera, bool doFrameBufferChecks,
							 glm::vec4 backgroundColor, glm::vec2 fogRangeDescriptor) {
	GameEngine->DrawAllParticles(CurrentRenderCamera);
}
void GkEngine2::setPauseState(bool pauseState){
	if(!pauseState){
		GameEngine->paused = false;
		GameEngine->bullet_thread.paused = false;
		for (auto*& processor : GameEngine->ECSProcessors)
			if (processor)
				processor->paused = false;
		GameEngine->getSoundHandler()->resumeAllSound();
	} else {
		GameEngine->paused = true;
		GameEngine->bullet_thread.paused = true;
		for (auto*& processor : GameEngine->ECSProcessors)
			if (processor)
				processor->paused = true;
		GameEngine->getSoundHandler()->pauseAllSound();
	}
}
void GkEngine2::displayLoadingScreen(double percentage) {
	FBO::unBindRenderTarget(WIDTH, HEIGHT);
	FBO::clearTexture(0, 0, 30.0 / 255.0, 0);
	if (myFont) {
		myFont->clearScreen(0, 0, 0, 0);
		myFont->writeString(std::string("Loading Game...") + std::to_string((int)percentage) + std::string("%"), WIDTH / 4, 10, 32, 32, glm::vec3(100, 255, 100)

		);
		myFont->swapBuffers();
		myFont->pushChangestoTexture();
	}
	if (myFont)
		myFont->draw(true);

	myDevice->swapBuffers(0);
}

void GkEngine2::init_game() {

	// setting up our callbacks.
	myDevice->setWindowFramebufferSizeCallback(0, window_size_callback_caller); // Self explanatory
	myDevice->setKeyCallBack(0, g_key_callback);
	myDevice->setCursorPositionCallback(0, g_cursor_position_callback);
	myDevice->setMouseButtonCallback(0, g_mouse_button_callback);
	myDevice->setCharCallback(0, g_char_callback);
	myDevice->setScrollCallback(0, g_scroll_callback);
	//~ myDevice->setWindowTitle(0, "gek's Simple Game Engine");
	//Note: You should probably set up the physics engine and threads here...
	//See main3.cpp

	registerECSProcessor(new GkPthreadEntityComponentProcessor()); // For Entity Components to do parallel processing.
	ECSProcessors.back()->init(this);
	
	theScene->customRenderingAfterTransparentObjectRendering = CustomTransparentRender;
	myFont = new BMPFontRenderer("geks Bitmap ASCII font 16x16.bmp", WIDTH, HEIGHT, UI_SCALE_FACTOR, "shaders/SHOWTEX");
	//Configure physics engine here.
	makeWorld(glm::vec3(0, -24, 0), false);
	bullet_thread.Construct(world, 0.016666666);
	UI_thread.Construct((void*)this);
	// Starting the threads
	bullet_thread.startThread();
	UI_thread.startThread();
	
	displayLoadingScreen(40.0);

	std::cout << "\nFinished Initgame!" << std::endl;
	
	activeControlInterface = &GameController;
	setPauseState(true);
	displayLoadingScreen(100.0);
}




void GkEngine2::tick_game() {
	// GameLogic portion!
	myDevice->pollevents(); // This will call your key and mouse button callbacks.
	if (!paused)
		tickParticleRenderers(0.0166666);
	if (!paused)
		felapsed += 0.0166666;
	if (felapsed > 300) {
		resetTimeParticleRenderers();
		felapsed = 0;
	}
	removeObjectsMarkedForDeletion();
	//GAME CODE GOES HERE
	myFont->clearScreen(0, 0, 0, 0);
}

GkEngine2::~GkEngine2() {
	
}

int main(int argc, char** argv) {
	bool lastArgWasCommand = true;
	std::string lastArg = "";
	std::string Arg;
	//~ if(argc)
	//~ lastArg = std::string(argv[0]);
	for (int i = 1; i < argc; i++) {
		Arg = std::string(argv[i]);
		if (((size_t)(Arg.find("-f"))) != std::string::npos)
			FULLSCREEN = true;
		if (((size_t)(lastArg.find("-w"))) != std::string::npos)
			WIDTH = GkAtoi(argv[i]);
		if (((size_t)(lastArg.find("-h"))) != std::string::npos)
			HEIGHT = GkAtoi(argv[i]);
		if (((size_t)(lastArg.find("-s"))) != std::string::npos)
			SCALE = GkAtof(argv[i]);
		lastArg = Arg;
	}
	GameEngine = new GkEngine2(WIDTH, HEIGHT, SCALE, false, FULLSCREEN, "GkEngine Test", true, false, 0);
	GameEngine->argc = argc;
	GameEngine->argv = argv; // points to same memory
	GameEngine->AutoExecutor();
	GameEngine->bullet_thread.killThread();
	GameEngine->UI_thread.killThread();
	std::cout << "\nKilled Threads" << std::endl;
	delete GameEngine;
	GameEngine = nullptr; // I always do this.
	std::cout << "\nExecution Finished." << std::endl;
	return 0;
}
