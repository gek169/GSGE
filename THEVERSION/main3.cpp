/*
(C) DMHSW 2020
* Gek's Simple Game Engine
* Main Driver Program and Demo
* 
* This program demonstrates how to create simple interactive games
* using GSGE. Q*ake-like character controller, 3d scene, AI-enabled
* characters (Admittedly a pretty terrible AI...) 
* 
* This program is organized into a single file (Not recommended for
* """actual""" projects)
* 
* This is done to make the build process easier (and easier to port should the
* standard methods not work for you)
* 
* 
*/

#include "FontRenderer.h"
#include "gekrender.h"
// My AL Utils
#include "GekAL.h"
// Engine
#include "GkEngine.h"
// Threading library
//#include "GkEnginePthreads.h"
#include "GkEngineEditor.h"
// C stuff
#include <chrono>
#include <clocale>
#include <cstdlib>

using namespace GSGE; //For convenience.
GLenum communism; //For mistakes ;o

//Program configuration settings.
int WIDTH = 800, HEIGHT = 600;
bool FULLSCREEN = false; //Use fullscreen window?
bool TESTINGLOADING = false; //Test loading a file?
bool ADDITIVE = false; //Use additive blending?
float SCALE = 1; //scale of rendered pixels to screen pixels



///Game Engine derivative
/**
 * This game engine is like many other C++ engines
 * in that it has a base class for applications.
 * 
 * GkEngine derivatives implement functions like init_game and tick_game
 * as well as displayLoadingScreen().
 * 
 * They also allow for various engine overrides such as the ability to
 * hook into the existing serialization system (Avoid writing your own)
 * and add your own .GSGE entity/object definition file lines.
 * 
 * If you believe that wrastling control from the user like this is tyrannical,
 * you do not have to use the autoexecutor, but it exists to hide a number of ugly
 * background processes such as controlling the threads and Entity Component Processors.
 * 
 * Realistically, it's what you'll probably want to use.
 * */
class GkEngine2 : public GkEngine { // You implement GkEngine by writing a derivative class
									// of it and implementing the functions.
  public:
	GkEngine2(uint window_width, uint window_height, float scale, bool resizeable, bool fullscreen, const char* title, bool _WBOIT, bool softbodies,
			  uint samples)
		: GkEngine(window_width, window_height, scale, resizeable, fullscreen, title, softbodies, _WBOIT, samples) {}
	~GkEngine2();
	void init_game() override; //Runs before first tick_game
	void tick_game() override; //main game loop
	void displayLoadingScreen(double percentage) override; //Draw the loading screen
	/**
	 * Defines a hook into the GSGE file line interpreter. Simply return "true"
	 * when you detect a line that you want to process.
	 * 
	 * GSGE files tell the engine what pieces a game entity is composed of, and how
	 * they should be treated.
	 * 
	 * In this demo we implement two line types to simply add entity components to our 
	 * objects, but you can implement the line however you like. Play with it.
	 * 
	 * The obscene number of variables are necessary to provide the full functionality of editting
	 * the base GkEngine's GSGE file loader. I believe I've exposed every variable.
	 * */
	bool GSGELineInterpreter(std::stringstream& file, std::string& line, std::vector<std::string>& tokens, bool isTemplate, GkObject* Template,
	 Transform& initTransform, GkObject*& RetVal, int& error_flag, IndexedModel& lastModel,
	 Mesh*& lastMesh, std::string& lastAnimationName, bool& RetValOwnsLastMesh, Texture*& lastTexture, CubeMap*& lastCubeMap,
	 MeshInstance*& lastMeshInstance, btRigidBody*& lastRigidBody, btCollisionShape*& lastCollisionShape,
	 std::vector<btCollisionShape*>& ShapesInOrder, std::vector<Mesh*>& MeshesInOrder, float& Scaling) override;
	// Your variables. Put your global variables here.
	// makes them easier to work with.
	CubeMap* globalSky = nullptr;				   // Note: Snagged from Resources
	//this is used by the alt control interface.
	glm::vec3 CameraMovement = glm::vec3(0, 0, 0); // Forward, Left/right, Up/down
	void setPauseState(bool pauseState);
	GkUIWindow* testWindow = nullptr;
	GkEditor* myEditor = nullptr;
	float felapsed = 0; //Variable I added for keeping track of time.
};
GkEngine2* GameEngine;
/** Q*ake-like character controller implementation
 * 
 * Well I sort of lied a bit. It feels a bit floaty... but I found that the most fun
 * so I stuck with it. I know how to make it better but I don't have the time.
 * 
 * Ultimately the FPS Controller is meant more for demonstration purposes than
 * anything else.
 * 
 * GkFPSCharacterController is derived from GkControlInterface, the generic
 * backend for all GkEngine control interfaces.
 * 
 * If you don't like the tyranny of forcefully redirecting the control callbacks into
 * classes, notice that I force you to implement the redirects yourself... You can hook
 * there as well. Additionally, you could capture other input types not implemented in 
 * GkControlInterface.
 * 
 * The FPS controller is implemented as an Entity component. it can also be used for AI
 * entities that you want to move *like* a player.
 * */

class CharacterControls : public GkFPSCharacterController {
  public:
	CharacterControls(GkObject* _body, Camera* _myCamera, glm::vec3 _RelPos, std::string configuration, btDiscreteDynamicsWorld* world)
		: GkFPSCharacterController(_body, _myCamera, _RelPos, configuration, world, false, true) {
		Forward[C_KEY] = GLFW_KEY_W;
		Backward[C_KEY] = GLFW_KEY_S;
		Left[C_KEY] = GLFW_KEY_A;
		Right[C_KEY] = GLFW_KEY_D;
		Jump[C_KEY] = GLFW_KEY_SPACE;
		Crouch[C_KEY] = GLFW_KEY_LEFT_CONTROL;
		CamLock[C_KEY] = GLFW_KEY_C;
		Run[C_KEY] = GLFW_KEY_LEFT_SHIFT;
		LookDown[C_KEY] = GLFW_KEY_DOWN;
		LookUp[C_KEY] = GLFW_KEY_UP;
		LookLeft[C_KEY] = GLFW_KEY_LEFT;
		LookRight[C_KEY] = GLFW_KEY_RIGHT;
	}
	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) override {
		GkFPSCharacterController::key_callback(window, key, scancode, action, mods);
		// Other actions
		if (key == GLFW_KEY_V && action == GLFW_PRESS) {
			switchToFlying();
		}
		if (key == GLFW_KEY_T && action == GLFW_PRESS) {
			btTransform t;
			if (GameEngine->getObject("MyDude")) {
				glm::mat4 trans = GameEngine->getObject("MyDude")->getWorldTransform();
				Transform bruh;
				bruh.setModel(trans);
				std::cout << "\nMYDUDE SITTING AT X: " << bruh.getPos().x << "\nY: " << bruh.getPos().y << "\nZ: " << bruh.getPos().z << std::endl;
			}
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
			GameEngine->setPauseState(true);
			//~ GameEngine->bullet_thread.paused = true;
			//~ for (auto*& processor : GameEngine->ECSProcessors)
				//~ if (processor)
					//~ processor->paused = true;
			//~ GameEngine->getSoundHandler()->pauseAllSound();
			GameEngine->setCursorLock(false);
			//~ GameEngine->cursorLock = false;
			GameEngine->testWindow->isHidden = false;
			GameEngine->activeControlInterface = GameEngine->testWindow;
		}
		if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
			GameEngine->Save("saves/save1/", "saves/save1/save1.gkmap");
			std::cout << "\nSaved!" << std::endl;
		}

		if (key == GLFW_KEY_M && action == GLFW_PRESS) {
			//~ GameEngine->removeObject(GameEngine->getObject("MyPetBox"));
			auto* b = GameEngine->getObject("Extrude2");
			if (b)
				b->markForDeletion();
		}
		if (key == GLFW_KEY_COMMA && action == GLFW_PRESS) {
			//~ GameEngine->removeObject(GameEngine->getObject("MyPetBox"));
			auto* b = GameEngine->getObject("Cute_Rabbit");
			if (b)
				b->markForDeletion();
		}
		if (key == GLFW_KEY_J && action == GLFW_PRESS) {
			glm::vec3 pos = GameEngine->MainCamera.pos + (5.0f * GameEngine->MainCamera.forward);
			GameEngine->addObject("EXTRUDE2.GSGE", false, "Extrude2",
								  Transform(pos, faceTowardPoint(pos, GameEngine->MainCamera.pos, glm::vec3(0, 1, 0)), glm::vec3(1, 1, 1)));
		}
		if (key == GLFW_KEY_K && action == GLFW_PRESS) {
			glm::vec3 pos = GameEngine->MainCamera.pos + (8.0f * GameEngine->MainCamera.forward);
			GameEngine->addObject("BUNNY.GSGE", true, "Cute_Rabbit",
								  Transform(pos, glm::vec3(0), glm::vec3(1, 1, 1))
								  );
		}
		if (key == GLFW_KEY_N && action == GLFW_PRESS) {
			auto* b = GameEngine->getObject("MyPetBox");
			if (b)
				b->markForDeletion();
		}
		if (key == GLFW_KEY_H && action == GLFW_PRESS) {
			glm::vec3 pos = GameEngine->MainCamera.pos + (5.0f * GameEngine->MainCamera.forward);
			GameEngine->addObject("BOX.GSGE", true, "MyPetBox",
								  Transform(pos, faceTowardPoint(pos, GameEngine->MainCamera.pos, glm::vec3(0, 1, 0)), glm::vec3(1, 1, 1)));
		}
		if (key == GLFW_KEY_B && action == GLFW_PRESS) {
			auto* b = GameEngine->getObject("MyPetMonkey");
			if (b)
				b->markForDeletion();
		}
		if (key == GLFW_KEY_G && action == GLFW_PRESS) {
			glm::vec3 pos = GameEngine->MainCamera.pos + (5.0f * GameEngine->MainCamera.forward);
			GameEngine->addObject("TEST.GSGE", true, "MyPetMonkey",
								  Transform(pos, faceTowardPoint(pos, GameEngine->MainCamera.pos, glm::vec3(0, 1, 0)), glm::vec3(1, 1, 1)));
		}

		if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
			GameEngine->shouldQuit_AutoExecutor = true;
		}
	}
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) override {
		GkFPSCharacterController::mouse_button_callback(window, button, action, mods); //redirect
		// Shoot a gun maybe
	}
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
								  glm::vec2 NormalizedCursorDelta) override {
		GkFPSCharacterController::cursor_position_callback(window, xpos, ypos, nxpos,
			nypos, CursorDelta, NormalizedCursorDelta); //redirect
	}

	void switchToFlying(); //This function has to be implemented after the definition for that
	//control interface type.
	//In a real project you could avoid this gross code by
	//setting a member variable of this class to the address of the other control interface,
	//But this is a demo, so we can safely use globals. Bad practice in a real project,
	//but not here!

	virtual void MidFrameProcess() override; //This is an entity component.
	//Specific to GkFPSController we must provide functions 
	//that lock and unlock the cursor for those keys to work.
	//In a real project, you'd retrieve a GkEngine*
	//Through the entity component system and cast it to GkEngineDerivative*
	//Because your engine derivative definition would have this function,
	//it would be accessible from here.
	void lockCursor() override {
		GameEngine->setCursorLock(true);
		//~ GameEngine->cursorLock = true;
	}

	void unLockCursor() override {
		GameEngine->setCursorLock(false);
		//~ GameEngine->cursorLock = false;
	}
	virtual ~CharacterControls();
};
CharacterControls* myCharacterController = nullptr; //Global variable.
//Globals are fine here but in a real project you wouldn't have them
//So remember that.
CharacterControls::~CharacterControls(){
	myCharacterController = nullptr; //In hindsight i'm unsure why I did this.
}
/**AI-Enabled Entity Component Demo
 * 
 * A rabbit. it hops around in random directions, at random intervals.
 * Pretty advanced stuff huh?
 * 
 * It demonstrates inter-component communication and dependency
 * 
 * Rabbit depends on AnimPlayer, specifically.
 * 
 * The actual functionality of the rabbit is pretty straight forward.
 * the hard part is understanding the API calls.
 * */
class DemoRabbit : public GkEntityComponent{
  public:
	DemoRabbit(GkObject* new_owner){
		ClassName = "DemoRabbit";
		GkObjectParent = new_owner;
		new_owner->addOwnedEntityComponent(this);
		//This little hack allows us to re-configure the entity's
		//rigid body collision flags in Bullet.
		GameEngine->getWorld()->removeRigidBody(new_owner->RigidBodies[0]);
		GameEngine->getWorld()->addRigidBody(new_owner->RigidBodies[0], COL_CHARACTERS, COL_NORMAL | COL_CHARACTERS);
		new_owner->makeCharacterController();
		state = rand()%50 + 40;
	}
	//State serialization! This class can fully serialize to file
	//Thanks to just two lines of c++!
	void readFromMaps() override{state = ints["state"];};
	void writeToMaps() override{ints["state"] = state;};
	//This function is executed during the synchronized portion of the frame,
	//when neither Bullet Physics or OpenGL is currently rendering anything.
	//You should aim to minimize what's done here. It should mostly be data transfers.
	void MidFrameProcess() override{
		if(	!GkObjectParent || 
			!((GkObject*)GkObjectParent)->RigidBodies.size() || 
			!((GkObject*)GkObjectParent)->RigidBodies[0]
		)
			return;
		auto* Daddy = ((GkObject*)GkObjectParent);
		auto* Body = Daddy->RigidBodies[0];
		GkAnimPlayer* AnimPlayer = (GkAnimPlayer*)Daddy->getEntityComponent("GkAnimPlayer");
		glm::mat3x3 SpringInfo(-1, 0, 0,   0, 0, 0,   0, 0, 0);
		glm::vec3 velbody = b2g_vec3(Body->getLinearVelocity());
		if(velbody.y < 0.1f || state < 45) SpringInfo = ((GkObject*)GkObjectParent)->applyCharacterControllerSpring(
			glm::vec3(0,0,0),	   // Position in local space relative to the body
			glm::vec3(0,-1,0), // Direction from RelPos in local space.
			1,		   // k value to use when the spring is pulling.
			0.8,		   // k value to use when the spring is pushing.
			0.98,		//Natural length of spring
			2.4, // Max Distance spring can "snap on" to things.
			GameEngine->getWorld(), 
			glm::vec3(0, 1, 0), // Set to 1,1,1 for fun!
			0.5
		);
		float dist = SpringInfo[0][0];
		float force = SpringInfo[0][1];
		float groundfriction = clampf(0.01, 0.3, SpringInfo[0][2]);
		glm::vec3 groundVel = SpringInfo[1];
		glm::vec3 groundNormal = SpringInfo[2];
		glm::vec3 relVelBody = b2g_vec3(Body->getLinearVelocity()) - groundVel;
		btVector3 FrictionImpulse = g2b_vec3(relVelBody * -groundfriction); FrictionImpulse.setY(0);
		onGround = (dist > 0);
		//Set the default "on ground" state.
		//This animation was defined in the GSGE file.
		//GSGE animations are exported from Blender using the script.
		//The script is GPL software (Blender dependent) so it can't be in this repo
		//since this is an MIT game engine.
		if(onGround) Daddy->setAnimation("DEFAULT", 0);
		if(onGround && state < 38)
		{
			
			velbody.y *= 0.5f;
			Daddy->setVel(velbody);
			//And also? Apply ground friction.
			Body->applyCentralImpulse(FrictionImpulse);
			if(AnimPlayer)
				AnimPlayer->haltAllAnimations();
			
		} 
		if(state < 1 && onGround) {
			
			
			//TODO play jump noise.
			btVector3 f(2 * jumpDirection.x, jumpForce, 2 * jumpDirection.y);
			Body->applyCentralImpulse(f);
			Transform bruh;
			bruh = (Daddy->getWorldTransform());
			//TODO: move the expensive faceTowardPoint calculation into the SeparatedProcess
			bruh.setRotQuat(
				faceTowardPoint(
					glm::vec3(0, 0, 0), 
					glm::vec3(-jumpDirection.x, 0, -jumpDirection.y), 
					glm::vec3(0,1,0)
				)
			);
			Daddy->setWorldTransform(bruh);
			//Attempt to play the jump animation of the rabbit.
			if(AnimPlayer)
				AnimPlayer->startAnimPlayback("jump", 4.0f, 39.0f/60.0f);
		}
		if(state < 1)
			state = abs(rand()%50 + 40);
		state--;
	}
	//This code is executed in the Separated portion of the frame.
	//RULES FOR THE SEPARTED PART:
	//1) No talking to Bullet or GL stuff. Reading has race conditions too, so that's a nono
	//2) No talking to other entity components. They can't be trusted, they could be running
	//on another entity component processor.
	//This part is pretty much only for calculations.
	void SeparatedProcess() override{
		//Check state to see if we need to jump.
		if(state < 1)
		{
			jumpDirection = glm::vec2(
				RandomFloat(-1,1),
				RandomFloat(-1,1)
			);
			jumpDirection = glm::normalize(jumpDirection);
			jumpForce = RandomFloat(5,7);
		} else {
			jumpForce = 0;
			jumpDirection = glm::vec2(0,0);
		}
	}
	bool onGround = false;
	int state = 60;
	glm::vec2 jumpDirection = glm::vec2(0,0);
	float jumpForce = 0;
};

/**CharacterControls is an entity component.
 * 
 * For convenience, we use the existing MidFrameProcess() function to hold all its actual
 * functionality.
 * 
 * */

void CharacterControls::MidFrameProcess() {

	// We do this instead of just saying GameEngine because
	// we want to demonstrate that the engine can still be accessed
	// even without awkwardly packing the engine class's instance declaration
	// before the entity component.

	// This is how you would access the engine normally, in a non-demo project,
	// which was organized into separate header and CPP files for all your classes.
	GkEngine* GE = (GkEngine*)(((GkEntityComponentProcessor*)myProcessor)->myEngine);
	if (GE->activeControlInterface == this) {
		tick(GE->getWorld(), GE->getSoundHandler());
	} else
		tick(GE->getWorld(), GE->getSoundHandler(), false);
}
/**GSGE Line Interpreter
 * 
 * I mentioned up above in the predeclaration that you could hook into GkEngine's
 * GSGE file parser. Well now you get to see how that actually works!
 * 
 * NOTED:
 * tkCmd is just a function I wrote to check if a string can reasonably be interpreted
 * as containing a command, and only that command, and nothing else.
 * 
 * if you just check for .find()!=std::string::npos then you get collisions with
 * any commands that are just other words slapped onto that one
 * Like "PHONG" and "PHONGINSTANCED"
 * 
 * Tokens is the result of splitting up the GSGE line with unix pipe '|' as the terminator.
 * */
bool GkEngine2::GSGELineInterpreter(std::stringstream& file, std::string& line, std::vector<std::string>& tokens, bool isTemplate, GkObject* Template,
									Transform& initTransform, GkObject*& RetVal, int& error_flag, IndexedModel& lastModel,
									Mesh*& lastMesh, std::string& lastAnimationName, bool& RetValOwnsLastMesh, Texture*& lastTexture, CubeMap*& lastCubeMap,
									MeshInstance*& lastMeshInstance, btRigidBody*& lastRigidBody, btCollisionShape*& lastCollisionShape,
									std::vector<btCollisionShape*>& ShapesInOrder, std::vector<Mesh*>& MeshesInOrder, float& Scaling) {
	if (tkCmd(tokens[0], "MAKEDEMOPLAYER")) {
		if (isTemplate)
			return false;
		if (!RetVal)
			return false;
		if (myCharacterController)
			return false; // The demo player is already set.
		if (GameEngine->ECSProcessors.size() < 1 || !GameEngine->ECSProcessors[0])
			return false;
		std::cout << "\nMAKING DEMO PLAYER!" << std::endl;
		myCharacterController = new CharacterControls(RetVal, &GameEngine->MainCamera, glm::vec3(0, 0, 0), "", GameEngine->getWorld());
		GameEngine->ECSProcessors[0]->registerComponent(myCharacterController);
		return true;
	} else if (tkCmd(tokens[0], "MAKEDEMORABBIT")) {
		if(isTemplate) return false;
		if(!RetVal) return false;
		if (GameEngine->ECSProcessors.size() < 1 || !GameEngine->ECSProcessors[0])
			return false;
		std::cout << "\nMAKING DEMO RABBIT!" << std::endl;
		GkEntityComponent* b = new DemoRabbit(RetVal);
		GameEngine->ECSProcessors[0]->registerComponent(b);
		b = new GkAnimPlayer(RetVal);
		GameEngine->ECSProcessors[0]->registerComponent(b);
		return true;
	}
	return false;
}
/**Another control interface
 * 
 * I wanted to demonstrate how one would implement their own camera manipulation system, 
 * and how GkConrolInterface worked, so I wrote one here.
 * 
 * FlyControls is sort of like noclip, or spectator cam from... other video games.
 * 
 * GkControlInterface is NOT derived from GkEntityComponent.
 * */
class FlyControls : public GkControlInterface {
  public:
	FlyControls() {}
	bool cursorLock = false;
	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) override {
		if (key == GLFW_KEY_V && action == GLFW_PRESS) {
			switchToCharacter();
		}
		if (key == GLFW_KEY_T && action == GLFW_PRESS) {
			btTransform t;
			if (GameEngine->getObject("MyDude")) {
				GameEngine->getObject("MyDude")->setPos(GameEngine->MainCamera.pos, true, true);
				glm::mat4 trans = GameEngine->getObject("MyDude")->getWorldTransform();
				Transform bruh;
				bruh.setModel(trans);
				std::cout << "\nMYDUDE SITTING AT X: " << bruh.getPos().x << "\nY: " << bruh.getPos().y << "\nZ: " << bruh.getPos().z << std::endl;
			}
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
			GameEngine->setPauseState(true);
			//~ GameEngine->getDevice()->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			GameEngine->setCursorLock(false);
			//~ GameEngine->cursorLock = false;
			GameEngine->testWindow->isHidden = false;
			GameEngine->activeControlInterface = GameEngine->testWindow;
		}
		if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
			GameEngine->Save("saves/save1/", "saves/save1/save1.gkmap");
			std::cout << "\nSaved!" << std::endl;
		}
		if(GameEngine->myEditor && key == GLFW_KEY_O && action == GLFW_PRESS){
			//~ GameEngine->getDevice()->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			GameEngine->setCursorLock(false);
			//~ GameEngine->cursorLock = false;
			GameEngine->activeControlInterface = GameEngine->myEditor;
		}
		//Some really complicated code to make shadowmapped cameralights.
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {
			// add a camera light to the scene, if there isn't one.
			// if there is one, move it to the camera's location.
			if (GameEngine->Cam_Lights.size() > 3) {
				auto& cl = *(GameEngine->Cam_Lights.back());
				cl.myCamera = GameEngine->MainCamera;
			} else {
				std::cout << "\nMaking shadowed cameralight!" << std::endl;
				GameEngine->Cam_Lights.push_back(new CameraLight());
				GameEngine->getScene()->registerCamLight(GameEngine->Cam_Lights.back());

				auto& cl = *(GameEngine->Cam_Lights.back());
				cl.isShadowed = true;
				//~ std::cout << "\nMarker 0" << std::endl;
				GameEngine->Cam_Light_Shadowmap_FBOs.push_back(new FBO(512, 512, 1, GL_RGBA32F));
				cl.myShadowMappingFBO = (void*)GameEngine->Cam_Light_Shadowmap_FBOs.back();
				//~ std::cout << "\nMarker 1" << std::endl;
				cl.Tex2Project = GameEngine->Cam_Light_Shadowmap_FBOs.back()->getTex(0);
				//~ std::cout << "\nMarker 2" << std::endl;
				cl.solidColorToggle = true;
				cl.myColor = glm::vec3(0.2, 0, 1); // Purple.
				cl.Dimensions = glm::vec2(1.0 / 512.0, 1.0 / 512.0);
				cl.myCamera = GameEngine->MainCamera;
				cl.radii = glm::vec2(0.3, 0.5);
				cl.range = 600;
				std::cout << "\nMade Camera Light!" << std::endl;
			}
		}

		if (key == GLFW_KEY_M && action == GLFW_PRESS) {
			//~ GameEngine->removeObject(GameEngine->getObject("MyPetBox"));
			auto* b = GameEngine->getObject("Extrude2");
			if (b)
				b->markForDeletion();
		}
		if (key == GLFW_KEY_COMMA && action == GLFW_PRESS) {
			//~ GameEngine->removeObject(GameEngine->getObject("MyPetBox"));
			auto* b = GameEngine->getObject("Cute_Rabbit");
			if (b)
				b->markForDeletion();
		}
		if (key == GLFW_KEY_J && action == GLFW_PRESS) {
			glm::vec3 pos = GameEngine->MainCamera.pos + (10.0f * GameEngine->MainCamera.forward);
			GameEngine->addObject("EXTRUDE2.GSGE", false, "Extrude2",
								  Transform(pos, faceTowardPoint(pos, GameEngine->MainCamera.pos, glm::vec3(0, 1, 0)), glm::vec3(1, 1, 1)));
		}
		if (key == GLFW_KEY_K && action == GLFW_PRESS) {
			glm::vec3 pos = GameEngine->MainCamera.pos + (8.0f * GameEngine->MainCamera.forward);
			GameEngine->addObject("BUNNY.GSGE", true, "Cute_Rabbit",
								  Transform(pos, glm::vec3(0), glm::vec3(1, 1, 1))
								  );
		}
		if (key == GLFW_KEY_N && action == GLFW_PRESS) {
			//~ GameEngine->removeObject(GameEngine->getObject("MyPetBox"));
			auto* b = GameEngine->getObject("MyPetBox");
			if (b)
				b->markForDeletion();
		}
		if (key == GLFW_KEY_H && action == GLFW_PRESS) {
			glm::vec3 pos = GameEngine->MainCamera.pos + (10.0f * GameEngine->MainCamera.forward);
			GameEngine->addObject("BOX.GSGE", true, "MyPetBox",
								  Transform(pos, faceTowardPoint(pos, GameEngine->MainCamera.pos, glm::vec3(0, 1, 0)), glm::vec3(1, 1, 1)));
		}
		if (key == GLFW_KEY_U && action == GLFW_PRESS) {
			std::cout << "\nSetting Position..." << std::endl;
			GameEngine->getObject("SomeNiceFloor")->setPos(GameEngine->MainCamera.pos - glm::vec3(0, 40, 0), true, true);
		}
		if (key == GLFW_KEY_B && action == GLFW_PRESS) {
			//~ GameEngine->removeObject(GameEngine->getObject("MyPetMonkey"));
			auto* b = GameEngine->getObject("MyPetMonkey");
			if (b)
				b->markForDeletion();
		}
		if (key == GLFW_KEY_G && action == GLFW_PRESS) {
			glm::vec3 pos = GameEngine->MainCamera.pos + (5.0f * GameEngine->MainCamera.forward);
			GameEngine->addObject("TEST.GSGE", true, "MyPetMonkey",
								  Transform(pos, faceTowardPoint(pos, GameEngine->MainCamera.pos, glm::vec3(0, 1, 0)), glm::vec3(1, 1, 1)));
		}
		// Camera Movement
		if (key == GLFW_KEY_W && action == GLFW_PRESS) {
			GameEngine->CameraMovement.x = 1;
			if (mods & GLFW_MOD_SHIFT)
				GameEngine->CameraMovement.x *= 5;
		} else if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
			GameEngine->CameraMovement.x = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			GameEngine->CameraMovement.x = -1;
			if (mods & GLFW_MOD_SHIFT)
				GameEngine->CameraMovement.x *= 5;
		} else if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
			GameEngine->CameraMovement.x = 0;
		}

		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			GameEngine->CameraMovement.y = 1;
			if (mods & GLFW_MOD_SHIFT)
				GameEngine->CameraMovement.y *= 5;
		} else if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
			GameEngine->CameraMovement.y = 0;
		}

		if (key == GLFW_KEY_D && action == GLFW_PRESS) {
			GameEngine->CameraMovement.y = -1;
			if (mods & GLFW_MOD_SHIFT)
				GameEngine->CameraMovement.y *= 5;
		} else if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
			GameEngine->CameraMovement.y = 0;
		}

		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			GameEngine->CameraMovement.z = 1;
			if (mods & GLFW_MOD_SHIFT)
				GameEngine->CameraMovement.z *= 3;
		} else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
			GameEngine->CameraMovement.z = 0;
		}

		if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS) {
			GameEngine->CameraMovement.z = -1;
			if (mods & GLFW_MOD_SHIFT)
				GameEngine->CameraMovement.z *= 3;
		} else if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE) {
			GameEngine->CameraMovement.z = 0;
		}

		if (key == GLFW_KEY_C && action == GLFW_PRESS) {
			cursorLock = !cursorLock;
			GameEngine->setCursorLock(cursorLock);
		}
		if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
			GameEngine->shouldQuit_AutoExecutor = true;
		}
	}

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) override {
	
	}

	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
								  glm::vec2 NormalizedCursorDelta) override {
		if (cursorLock) {
			GameEngine->MainCamera.pitch((float)(CursorDelta.y) * 0.001f);
			GameEngine->MainCamera.rotateY((float)(CursorDelta.x) * -0.001f);
			//~ GameEngine->getDevice()->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			//~ GameEngine->setCursorLock(true);
		} else {
			//~ GameEngine->setCursorLock(false);
		}
	}

	void switchToCharacter();
};
//Again we do this ugly thing because this is a single file program.
//If you had a multi-file program
//Then all functions would be moved outside class definitions
//and this wouldn't stand out so much.
//You'd include "mygame.h" and "mycharactercontroller.h" (for the
//game engine and character controller class definitions, respectively)
//and you'd have access to the types.
void FlyControls::switchToCharacter() {
	if (myCharacterController)
		GameEngine->activeControlInterface = myCharacterController;
}
//Globals not bad in demo >:/
FlyControls flyController;
//CharacterControls gets its switch function
void CharacterControls::switchToFlying() { GameEngine->activeControlInterface = &flyController; }

/**Input and Event Callbacks
 * 
 * Currently joystick support is not implemented... but GLFW3 currently does,
 * and you could implement it if you wanted to.
 * 
 * Additionally, there may be some other input device I haven't thought of
 * which you may want to provide support for.
 * 
 * I force you to implement these functions yourself specifically so that you are forced to make
 * this realization.
 * 
 * As for events?
 * You may desire to implement other event handlers.
 * So I force you to implement those too.
 * (this is as opposed to having static, global, library-defined functions)
 * */
void window_size_callback_caller(GLFWwindow* window, int width, int height) {
	GameEngine->window_size_callback(window, width, height);
	if(GameEngine->myEditor)
		GameEngine->myEditor->resize(width, height);
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
/**Callback for clicking the X button in the "q" menu
 * 
 * In the demo if you press Q you get a menu.
 * 
 * This is the code to handle the X button, which unpauses the game and stops
 * displaying the menu.
 * 
 * This is how you handle button click events in the UI system.
 * */
void TestWindowXButtonClick(GkControlInterface* me, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
		GameEngine->testWindow->isHidden = true;
		for (auto& field : GameEngine->testWindow->Fields) {
			field.isEditing = false;
		}
		if (myCharacterController)
			GameEngine->activeControlInterface = myCharacterController;
		else
			GameEngine->activeControlInterface = &flyController;
		GameEngine->setPauseState(false);
	}
}
//This is the quit button.
void TestWindowQuitButtonClick(GkControlInterface* me, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
		GameEngine->shouldQuit_AutoExecutor = true;
	}
}
/**Render Pipeline hooks
 * You have an idea for a new, amazing thing to draw. You know
 * exactly how you're going to render it.
 * 
 * The only problem is, *I* haven't thought of it. And my pipeline is
 * nearly unreadable to anyone except me (tested) so how do I make it
 * easy for you to put in your own rendering code?
 * 
 * Well the answer is a function pointer. GkScene has multiple hook function pointers
 * If they're not null, they'll be called during the rendering process
 * 
 * So you can implement your own rendering, GL, etc code at three separate stages in the pipeline!
 * */
void CustomTransparentRender(int meshmask, FBO* CurrentRenderTarget, FBO* RenderTarget_Transparent, Camera* CurrentRenderCamera, bool doFrameBufferChecks,
							 glm::vec4 backgroundColor, glm::vec2 fogRangeDescriptor) {
	GameEngine->DrawAllParticles(CurrentRenderCamera);
}
//Not as easy as it sounds. Thread management is necessary.
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
/**LOADING SCREENS
 * Write your best loading screen renderer here.
 * 
 * It'll only be drawn whenever the function is called (There are some calls to it in the
 * Save file loader, and I put some in init_game in this demo)
 * */
void GkEngine2::displayLoadingScreen(double percentage) {
	FBO::unBindRenderTarget(WIDTH, HEIGHT);
	FBO::clearTexture(0, 0, 30.0 / 255.0, 0);
	if (myFont) {
		myFont->clearScreen(0, 0, 0, 0);
		myFont->writeString(
		std::string("Loading Game...") + std::to_string((int)percentage) + std::string("%"), 
		WIDTH / 4,  //x position
		10, // y position
		32, 32, //size x and y
		glm::vec3(100, 255, 100) //color

		);
		myFont->swapBuffers();
		myFont->pushChangestoTexture();
	}
	if (myFont)
		myFont->draw(true);

	myDevice->swapBuffers(0);
}

/**Initialize your stuff
 * Runs before the first tick_game, as part of AutoExector.
 * */
void GkEngine2::init_game() {
	// setting up our callbacks.
	myDevice->setWindowFramebufferSizeCallback(0, window_size_callback_caller); // Self explanatory
	myDevice->setKeyCallBack(0, g_key_callback);
	myDevice->setCursorPositionCallback(0, g_cursor_position_callback);
	myDevice->setMouseButtonCallback(0, g_mouse_button_callback);
	myDevice->setCharCallback(0, g_char_callback);
	myDevice->setScrollCallback(0, g_scroll_callback);
	
	/**How do Entity Components work?
	 * if you gizoogle "entity components" you'll find a bunch of crap.
	 * 
	 * TL;DR it's a way to make parallelizing game code extremely generic.
	 * 
	 * You know how threading libraries like pthreads will ask you to init
	 * threads with a function pointer, right?
	 * Well entity components have two virtual functions, Mid Frame and Separated
	 * 
	 * The separated process is (theoretically, doesn't have to be) parallelized.
	 * Basically we just execute that on a thread. For every entity component.
	 * at the same time the rendering and physics simulation is happening.
	 * 
	 * Unfortunately this method of implementing ECS doesn't have some of the
	 * touted advantages (Cache grab success rate, for instance)
	 * but it's still pretty good.
	 * */
	registerECSProcessor(new GkPthreadEntityComponentProcessor()); // For Entity Components to do parallel processing.
	ECSProcessors.back()->init(this);
	//enable vsync
	
	//Here we set out wonderful rendering pipeline hook
	theScene->customRenderingAfterTransparentObjectRendering = CustomTransparentRender;
	
	/**Fonts
	 * Fonts must be in an image file.
	 * Character glyphs must be squares.
	 * There must be 8 per row.
	 * The red channel is used to define the shading of each pixel.
	 * Green and blue channels are ignored.
	 * The width and height of a character glyph
	 * are computed by taking the width of the loaded image and
	 * dividing it by 8.
	 * */
	myFont = new BMPFontRenderer("geks Bitmap ASCII font 16x16.bmp", WIDTH, HEIGHT, UI_SCALE_FACTOR, "shaders/SHOWTEX");
	//The editor! Enables us to move stuff around and delete stuff too
	myEditor = new GkEditor(this, WIDTH, HEIGHT);
	//The editor enables us to set a control interface to switch to with the 'O' key,
	//<assuming you read what I wrote up above about awkward control interface switching code>
	//this is how you'd "properly" implement it.
	myEditor->altControlInterface = &flyController; 
	
	displayLoadingScreen(1.0);
	/**Configuring the UI
	 * Every element has to be set by hand. There are a lot of them.
	 * it takes a lot of assignments (a lot of LoC)
	 * but I still think this is superior to what most Gui libraries do,
	 * where theming shit is practically baked
	 * */
	testWindow = new GkUIWindow(WIDTH, HEIGHT);
	addSubWindow(testWindow);
	testWindow->WindowBody.width = 600;
	testWindow->WindowBody.height = 400;
	testWindow->WindowBody.alpha = 128; // Partially transparent.
	testWindow->WindowBody.red = 255;
	testWindow->WindowBody.string_color = glm::vec3(255);
	testWindow->WindowBody.string = "Options";
	testWindow->WindowBody.text_offset_x = 0;
	testWindow->WindowBody.text_offset_y = 400 - 32;
	testWindow->WindowBody.x = 10;
	testWindow->WindowBody.y = 10;
	testWindow->WindowBody.charw = 32;
	testWindow->WindowBody.charh = 32;
	//~ testWindow->ClickCallBack = MakeWindowActiveWindowClick;
	testWindow->isHidden = true;
	testWindow->isInBackground = false;
	testWindow->CI_Name = "testWindow";
	//Did I mention that GkUIWindows can scroll?
	//Scrollbars are :b:loat, just use the scrollwheel, idiot
	testWindow->scrollRange = glm::ivec2(-20,20);
	// X button
	testWindow->addButton(GkButton(TextBoxInfo()));
	{
		GkButton& button = testWindow->Buttons.back();
		button.box.string = "X";
		button.box.string_color = glm::vec3(255);
		button.box.shouldScroll = false;
		button.box.width = 16 * button.box.string.length();
		button.box.height = 18;
		button.box.red = 0;
		button.box.green = 0;
		button.box.blue = 0;
		button.box.alpha = 255; // OPAQUE AS FUCK!
		button.box.charh = 16;
		button.box.charw = 16;
		button.box.text_offset_x = 0;
		button.box.text_offset_y = 3;
		button.box.x = testWindow->WindowBody.width - button.box.width;
		button.box.y = testWindow->WindowBody.height - button.box.height;
		button.ClickCallBack = TestWindowXButtonClick;
		button.CI_Name = "testWindowXbutton";
	}
	// QuitGame button.
	testWindow->addButton(GkButton(TextBoxInfo()));
	{
		GkButton& button = testWindow->Buttons.back();
		button.box.string = "Quit Demo Program";
		button.box.string_color = glm::vec3(255);
		button.box.width = 16 * button.box.string.length();
		button.box.height = 18;
		button.box.red = 0;
		button.box.green = 0;
		button.box.blue = 0;
		button.box.alpha = 255; // OPAQUE AS FUCK!
		button.box.charh = 16;
		button.box.charw = 16;
		button.box.text_offset_x = 0;
		button.box.text_offset_y = 3;
		button.box.x = 10;
		button.box.y = testWindow->WindowBody.height / 4 - button.box.height / 2;
		button.ClickCallBack = TestWindowQuitButtonClick;
		button.CI_Name = "testWindowQuitbutton";
	}
	testWindow->addField(GkEdTextField(0, testWindow->WindowBody.height / 2, 16 * 20, 16));
	{
		GkEdTextField& field = testWindow->Fields.back();
		field.red = 0;
		field.green = 100;
		field.blue = 100;
		field.alpha = 255;
		field.string = "Click here to type something";
		field.width = field.string.length() * 16;
		field.string_color = glm::vec3(255);
		field.red_cursor = 0;
		field.green_cursor = 0;
		field.blue_cursor = 0;
		field.charw = 16;
		field.charh = 16;
		field.ClickCallBack = fieldClickActiveClear;
		field.CI_Name = "testWindowEdTextField";
	}
	testWindow->setScreenSize(WIDTH, HEIGHT, UI_SCALE_FACTOR);
	displayLoadingScreen(5.0);
	// Making the btDynamicsWorld
	makeWorld(glm::vec3(0, -24, 0), false);
	bullet_thread.Construct(world, 0.016666666);
	UI_thread.Construct((void*)this);
	// Starting the threads
	bullet_thread.startThread();
	UI_thread.startThread();
	//You're going to see if(!TESTINGLOADING) a lot
	//If the engine is loading from file all this stuff is supposed to get
	//instantiated on its own, without intervention,
	//by the file loader. So we shouldn't make new ones, that would suck.
	if (!TESTINGLOADING) {
		GkObject* playerObject = getObject("MyDude");
		if (!playerObject)
			playerObject = addObject("PLAYER.GSGE", false, "MyDude", Transform(glm::vec3(3, 10, 20), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)));
		
	}
	displayLoadingScreen(10.0);
	//Particle renderers are also hard to make! They have a lot of options.
	if (!TESTINGLOADING) {
		addParticleRenderer(new ParticleRenderer());
		ParticleRenderer& testParticleRenderer = *(ParticleRenderers.back());
		testParticleRenderer.init(ParticleShader, // Internal in GkEngine
								  SafeTexture(Resources.getTexture("bubble.png", true, GL_NEAREST, GL_REPEAT, 0.0)), true);
		testParticleRenderer.finalTransparency = 0;
		testParticleRenderer.initTransparency = 5;
		//Limits on the start position when randomly generating
		testParticleRenderer.maxX = 800;
		testParticleRenderer.minX = -800;
		testParticleRenderer.maxZ = 800;
		testParticleRenderer.minZ = -800;
		testParticleRenderer.maxY = 100;
		testParticleRenderer.minY = -300;
		//Limits on the velocity when randomly generating.
		testParticleRenderer.maxVX = 0.9;
		testParticleRenderer.minVX = -0.9;
		testParticleRenderer.maxVZ = 0.9;
		testParticleRenderer.minVZ = -0.9;
		testParticleRenderer.maxVY = 0.9;
		testParticleRenderer.minVY = -0.4;

		testParticleRenderer.particleSize = 10;
		testParticleRenderer.max_age = 12;
		testParticleRenderer.Acceleration = glm::vec3(0, 0.4, 0);
	}
	displayLoadingScreen(20.0);
	if (!TESTINGLOADING) {
		Point_Lights.push_back(new PointLight(glm::vec3(300, 30, 10), glm::vec3(1, 1, 1)));
		Point_Lights.back()->range = 10000;
		Point_Lights.back()->dropoff = 2;
		theScene->registerPointLight(Point_Lights.back());

		Point_Lights.push_back(new PointLight(glm::vec3(-10, 100, 300), glm::vec3(0.5, 0.5, 1)));
		Point_Lights.back()->range = 5000;
		Point_Lights.back()->dropoff = 2;
		theScene->registerPointLight(Point_Lights.back());

		Amb_Lights.push_back(new AmbientLight());
		Amb_Lights.back()->myPos = glm::vec3(357, 0, -429);
		Amb_Lights.back()->range = 1000;
		Amb_Lights.back()->myColor = glm::vec3(0.2, 0.5, 0.2);
		theScene->registerAmbLight(Amb_Lights.back());
	}
	displayLoadingScreen(30.0);
	MainCamera.pos = (glm::vec3(0, 0, -10));
	MainCamera.forward = glm::vec3(1, 0, 1);
	std::string cubemapfilenames[6] = {
		"Cubemap/Skybox_Water10_128_right.jpg", // right
		"Cubemap/Skybox_Water10_128_left.jpg",  // left
		"Cubemap/Skybox_Water10_128_top.jpg",   // up
		"Cubemap/Skybox_Water10_128_base.jpg",  // down
		"Cubemap/Skybox_Water10_128_back.jpg",  // back
		"Cubemap/Skybox_Water10_128_front.jpg"  // front
	};
	if (!TESTINGLOADING) {
		globalSky =
			Resources.getCubeMap(cubemapfilenames[0], cubemapfilenames[1], cubemapfilenames[2], cubemapfilenames[3], cubemapfilenames[4], cubemapfilenames[5]);
		theScene->setSkyBoxCubemap(globalSky);
		addObject("RACETRACK.GSGE", false, "SomeNiceFloor", Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0.5, 0), glm::vec3(10, 10, 10)));
		addObject("TEST.GSGE", false, "BIG MONKEH", Transform(glm::vec3(0, 10, 0), glm::vec3(0, 0, 0), glm::vec3(4, 4, 4)));
	}
	displayLoadingScreen(40.0);
	//~ addObject("TERRAIN.GSGE", false, "SomeNiceFloor", Transform(glm::vec3(0, -100, 0), glm::vec3(0, 1, 0), glm::vec3(10, 10, 10)));

	std::cout << "\nFinished Initgame!" << std::endl;
	
	if (TESTINGLOADING)
		Load("saves/save1/save1.gkmap");
	if (myCharacterController)
		activeControlInterface = myCharacterController;
	else
		activeControlInterface = &flyController;
	if(myEditor)
		activeControlInterface = myEditor;
	setPauseState(true);
	displayLoadingScreen(100.0);
}




void GkEngine2::tick_game() {
	
	// GameLogic portion!
	myDevice->pollevents(); // This will call your key and mouse button callbacks.
	g_processJoysticks(); //Cannot be pushed through callbacks.
	if (!paused)
		if (activeControlInterface == &flyController) {
			MainCamera.moveForward(CameraMovement.x);
			MainCamera.moveRight(CameraMovement.y);
			MainCamera.moveUp(CameraMovement.z);
		}
	if (!paused)
		tickParticleRenderers(0.0166666);
	if (!paused)
		felapsed += 0.0166666;
	if (felapsed > 300) {
		resetTimeParticleRenderers();
		felapsed = 0;
	}
	if(myEditor)
		myEditor->tick();
	//Scroll the texture on our box. Demonstrating a GSGE feature.
	auto* b = getObject("MyPetBox");
	if(b)
		{
			for(Mesh*& m: b->SharedMeshes)
			if(m)
			{
				m->instanced_texOffset.x += 0.01;
				if(m->instanced_texOffset.x > 1)
					m->instanced_texOffset.x -= 1.0f;
				if(m->instanced_texOffset.x < -1)
					m->instanced_texOffset.x += 1.0f;
			}
		}
	if(myEditor)
		myEditor->safety_call_before_removing_marked_GkObjects();
	removeObjectsMarkedForDeletion();
	
	
}

GkEngine2::~GkEngine2() {
	if (testWindow) {
		removeSubWindow(testWindow);
		delete testWindow;
	}
}

int main(int argc, char** argv) {
	bool lastArgWasCommand = true;
	uint samples = 0;
	std::string lastArg = "";
	std::string Arg;
	for (int i = 1; i < argc; i++) {
		Arg = std::string(argv[i]);
		if (((size_t)(Arg.find("-f"))) != std::string::npos)
			FULLSCREEN = true;
		if (((size_t)(Arg.find("-nowboit"))) != std::string::npos)
			ADDITIVE = true;
		//~ if (((size_t)(lastArg.find("-msaa"))) != std::string::npos)
		//~ {
		//~ std::cout << "\nMSAA Enabled." << std::endl;
		//~ MULTISAMPLE = true;
		//~ samples = GkAtoui(argv[i]);
		//~ }
		if (((size_t)(Arg.find("-loadfile"))) != std::string::npos)
			TESTINGLOADING = true;
		if (((size_t)(lastArg.find("-w"))) != std::string::npos)
			WIDTH = GkAtoi(argv[i]);
		if (((size_t)(lastArg.find("-h"))) != std::string::npos)
			HEIGHT = GkAtoi(argv[i]);
		if (((size_t)(lastArg.find("-s"))) != std::string::npos)
			SCALE = GkAtof(argv[i]);
		lastArg = Arg;
	}
	GameEngine = new GkEngine2(WIDTH, HEIGHT, SCALE, false, FULLSCREEN, "GkEngine Test", !ADDITIVE, false, 0);
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
