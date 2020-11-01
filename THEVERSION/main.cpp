/*
(C) DMHSW 2018
GkScene Demo Program 1 main.cpp
*/

#include "FontRenderer.h"
#include "Global_Variables.h" //theScene and FileResourceManager
#include "gekrender.h"
#include "resource_manager.h"
// My AL Utils
#include "GekAL.h"
//#define GLFW_DLL // Do we need this? //No.
//(C) DMHSW 2018 All Rights Reserved

unsigned int WIDTH = 1920;
unsigned int HEIGHT = 1080;
// Constants for the water demo
const unsigned int FLUID_WIDTH = 512;
const unsigned int FLUID_HEIGHT = 512;
unsigned char* Fluid_Target = nullptr;
unsigned char* Caustic_Target = nullptr;
// Fluid Demo Stuff
float u[FLUID_WIDTH][FLUID_HEIGHT];
float v[FLUID_WIDTH][FLUID_HEIGHT];
float u_new[FLUID_WIDTH][FLUID_HEIGHT];
unsigned char d_map[FLUID_WIDTH * FLUID_HEIGHT * 4];

unsigned char get_d(int _x, int _y, int component) {
	int x = _x;
	int y = _y;
	if (x >= (int)FLUID_WIDTH) {
		x = FLUID_WIDTH - 1;
	}
	if (x < 0) {
		x = 0;
	}
	if (y >= (int)FLUID_HEIGHT) {
		y = FLUID_HEIGHT - 1;
	}
	if (y < 0) {
		y = 0;
	}
	return d_map[(x + y * FLUID_HEIGHT) * 4 + component];
}

float get_u(int _x, int _y) {
	int x = _x;
	int y = _y;
	if (x >= (int)FLUID_WIDTH) {
		x = FLUID_WIDTH - 1;
	}
	if (x < 0) {
		x = 0;
	}
	if (y >= (int)FLUID_HEIGHT) {
		y = FLUID_HEIGHT - 1;
	}
	if (y < 0) {
		y = 0;
	}
	return u[x][y];
}
unsigned char float_to_char(float input) {
	float x = input;
	if (input > 1) {
		x = 1;
	} else if (input < 0) {
		x = 0;
	}
	return 255 * x;
}

// UI relative scale to monitor size
float UI_SCALE_FACTOR = 0.5f;
bool useFBO = false;	   // Should the next object spawned use the test FBO texture?
bool holdingShift = false; // used for sprinting
double progtime = 0.0;
bool shouldQuit = false;
bool creepilyfacetowardthecamera = true;
bool deregisterOnButtonPress = true;
// std::vector<RenderableMesh*> RendMeshArray; //TO BE DEPRECATED
std::vector<gekRender::PointLight*> PointLightsWithoutShadows;
std::vector<gekRender::AmbientLight*> Amb_Lights;
std::vector<gekRender::CameraLight*> Cam_Lights;

gekRender::Mesh* InstancedMesh = nullptr;
gekRender::Mesh* Generated_Sphere = nullptr;
std::vector<gekRender::MeshInstance*> ProgramMeshInstances; // Instances of meshes.
std::vector<gekRender::FBO*> FBOArray;						// FBOs, not including ones used by GkScene
gekRender::MeshInstance TerrainInstance;					// The terrain meshinstance. To be registered to
gekRender::MeshInstance LetterTester;						// Meshinstance for testing all
																// letters
gekRender::MeshInstance Generated_Sphere_Tester;			// Meshinstance for testing sphere generation

gekRender::Shader* MainShad = nullptr;		  // Final Pass Shader
gekRender::Shader* SkyboxShad = nullptr;	  // Skybox shader
gekRender::Shader* DisplayTexture = nullptr;  // Displays a texture to the screen
gekRender::Shader* WBOITCompShader = nullptr; // Composites the WBOIT initial pass onto the opaque framebuffer
gekRender::Shader* MainshaderShadows = nullptr;

gekRender::Camera* SceneRenderCamera = nullptr;		// SceneRender camera
gekRender::Camera RenderTargetCamera;				// The render target camera
gekRender::Camera RenderTargetCameraShadowMapping;  // render target camera for shadowmapping
gekRender::Camera RenderTargetCameraShadowMapping2; // render target camera for shadowmapping
gekRender::CubeMap* SkyboxTex = nullptr;			// Skybox texture
gekRender::CubeMap* SkyboxTwo = nullptr;			// Second skybox texture, for testing per-mesh cubemaps

gekRender::BMPFontRenderer* myFont = nullptr; // my super special font!
gekRender::Mesh* DeleteMeshTest = nullptr;

// Necessary for water surface rendering
gekRender::Mesh* WaterSurfaceMesh = nullptr;
gekRender::MeshInstance WaterSurfaceInstance;
gekRender::Texture* WaterSurfaceTexture = nullptr;
gekRender::Texture* WaterCausticsTexture = nullptr;

// Struct used for image writing
struct Image_Data {
	unsigned char* data_ptr = nullptr;
	int width = 0;
	int height = 0;
	int num_components = 0;
	Image_Data() { data_ptr = nullptr; }
	Image_Data(std::string filename) { loadImage(filename); }
	void unload() {
		if (data_ptr != nullptr)
			free(data_ptr);
	}
	~Image_Data() {
		// DO NOT DO ANYTHING!!!
	}
	Image_Data(const Image_Data& other) {
		data_ptr = other.data_ptr;
		width = other.width;
		height = other.height;
		num_components = other.num_components;
	}
	void loadImage(std::string filename) { // Attempt to load file
		unload();
		data_ptr = nullptr;
		int temp_width;
		int temp_height;
		int temp_components;
		int temp_something = 4; // no idea
		data_ptr = gekRender::Texture::stbi_load_passthrough((char*)filename.c_str(), &temp_width, &temp_height, &temp_components, temp_something);
		if (data_ptr != nullptr) {
			width = temp_width;
			height = temp_height;
			num_components = temp_components;
		} else {
			std::cout << "\nERROR!!! PROBELM LOADING " << filename << " AS IMAGE" << std::endl;
			std::abort();
		}
	}
};

// Array of these structs
std::vector<Image_Data> Images;
//~ bvh::Bvh Character_Running_Anim;

// OpenAL Variables
//~ ALCdevice *OpenALDevice = 0;
//~ ALCcontext *OpenALContext = 0;
//~ ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
// For the actual audio we're going to play
ALuint audiosource1 = 0;
ALuint audiobuffer1 = 0;

/*
User Input Variables and Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
bool cameralock;		  // Should the mouse be locked and enable first person mouse
						  // control?
int oldkeystates[51];	 // nuff said
double oldmousexy[2];	 // Old mouse position
double currentmousexy[2]; // Why query the current mouse position multiple
						  // times?

// KEYBOARD CALLBACK FUNCTIONS~~~~~~~~~~~~~
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		return;
}

// MOUSE BUTTON CALLBACK FUNCTIONS~~~~~~~~~~~~~~~~
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) { return; }

// Callback for custom Mainshader rendering and uniform binding
void MainshaderUniformFunctionDemo(int meshmask, gekRender::FBO* CurrentRenderTarget, gekRender::FBO* RenderTarget_Transparent,
								   gekRender::Camera* CurrentRenderCamera, bool doFrameBufferChecks, glm::vec4 backgroundColor,
								   glm::vec2 fogRangeDescriptor) {
	// std::cout<<"The Uniform Binding Function works, replace the code here
	// with your uniform binds!!!"<<std::endl; Put your own code here for
	// binding uniform variables... Keep in mind you have to get their locations
	// and save them yourself!
}

void CustomRenderingFunction(int meshmask, gekRender::FBO* CurrentRenderTarget, gekRender::FBO* RenderTarget_Transparent,
							 gekRender::Camera* CurrentRenderCamera, bool doFrameBufferChecks, glm::vec4 backgroundColor, glm::vec2 fogRangeDescriptor) {
	// std::cout<<"The Custom Rendering Function works, replace the code here
	// with your rendering code."<<std::endl; Put your custom rendering code in
	// here. Make sure you reset the GL state settings (e.g.
	// glEnable(GL_DEPTH_TEST)) if you change them at any point. You can bind
	// shaders as you please. This function takes place between rendering the
	// skybox and rendering the objects in the scene, before MainShader is
	// bound.
}
// A stupid way to do key checking since there are a buttload of state calls but
// eh
void checkKeys() {
	using namespace gekRender;
	static int state;
	state = myDevice->getKey(0, GLFW_KEY_W);
	if (state == GLFW_PRESS) {
		if (!holdingShift)
			SceneRenderCamera->moveForward(0.20);
		else
			SceneRenderCamera->moveForward(10.0);
	}
	oldkeystates[0] = state;
	state = myDevice->getKey(0, GLFW_KEY_A);
	if (state == GLFW_PRESS)
		SceneRenderCamera->moveRight(0.20);
	oldkeystates[1] = state;
	state = myDevice->getKey(0, GLFW_KEY_S);
	if (state == GLFW_PRESS) {
		if (!holdingShift)
			SceneRenderCamera->moveForward(-0.20);
		else
			SceneRenderCamera->moveForward(-10.0);
	}
	oldkeystates[2] = state;
	state = myDevice->getKey(0, GLFW_KEY_D);
	if (state == GLFW_PRESS)
		SceneRenderCamera->moveRight(-0.20);
	oldkeystates[3] = state;
	state = myDevice->getKey(0, GLFW_KEY_C);
	if (state == GLFW_PRESS && oldkeystates[4] != GLFW_PRESS) {
		cameralock = !cameralock;
		std::cout << "\nToggled Cameralock";
	}
	oldkeystates[4] = state;
	state = myDevice->getKey(0, GLFW_KEY_J); // Spawn a mesh
	if (state == GLFW_PRESS && oldkeystates[5] != GLFW_PRESS)
		for (int i = 0; i < 1000; i++) {
			// Test out the Meshinstance registration feature
			ProgramMeshInstances.push_back(
				new MeshInstance((rand() % 100 > 50) ? 1 : 0, // Texture, demo of inline if. It's either 1 or 0.
								 Transform(SceneRenderCamera->pos + glm::vec3(rand() % 200 - 100, rand() % 200 - 100, rand() % 200 - 100),
										   glm::vec3(rand() % 200 - 100, rand() % 200 - 100, rand() % 200 - 100), glm::vec3(1, 1, 1))));
			// Randomly decide whether or not to use Cubemap Reflections
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->EnableCubemapReflections =
				(rand() % 100 > 25) ? 1 : 0; // Note this does nothing because instancing is used...
			// ProgramMeshInstances[ProgramMeshInstances.size()-1]->EnableCubemapReflections
			// = 1;
			// Randomly decide which cubemap to use.
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->cubeMap = 0;

			// Random Specular Dampening
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->myPhong.specdamp = rand() % 128 + 1.0;

			// Random Specular Reflectivity
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->myPhong.specreflectivity = rand() % 100 / 100.0;

			// Use specr to determine diffusivity
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->myPhong.diffusivity =
				1 - ProgramMeshInstances[ProgramMeshInstances.size() - 1]->myPhong.specreflectivity;

			// finally register
			InstancedMesh->registerInstance(ProgramMeshInstances[ProgramMeshInstances.size() - 1]);
		}
	oldkeystates[5] = state;
	state = myDevice->getKey(0, GLFW_KEY_K);
	if (state == GLFW_PRESS && oldkeystates[6] != GLFW_PRESS)
		for (int i = 0; i < 1000; i++) {
			if (ProgramMeshInstances.size() > 0 && InstancedMesh->numInstances() > 0) // if there are any to deregister
			{
				int haveDeregistered = 0;
				int i = ProgramMeshInstances.size() - 1;
				while (haveDeregistered < 1 && i > -1) // While we might be able to deregister
				{
					bool donedeed = InstancedMesh->deregisterInstance(ProgramMeshInstances[i]);
					if (donedeed) {
						delete ProgramMeshInstances[i];
						ProgramMeshInstances.erase(ProgramMeshInstances.begin() + i);
						haveDeregistered++;
					}
					i--; // we're going through the vector backwards
				}
				if (haveDeregistered < 1)
					std::cout << "\n ERROR! Unable to delete";
			}
		}
	oldkeystates[6] = state;
	state = myDevice->getKey(0, GLFW_KEY_H);
	if (state == GLFW_PRESS && oldkeystates[7] != GLFW_PRESS) {
		// std::cout << "\n BEGUN DELETION PROCESS~~~ There are" <<
		// PointLightsWithoutShadows.size() << " point lights right now";
		if (PointLightsWithoutShadows.size() > 0) {
			theScene->deregisterPointLight(PointLightsWithoutShadows[PointLightsWithoutShadows.size() - 1]);
			delete PointLightsWithoutShadows[PointLightsWithoutShadows.size() - 1];
			PointLightsWithoutShadows.erase(PointLightsWithoutShadows.begin() + PointLightsWithoutShadows.size() - 1);
		}
		std::cout << "\n FINISHED DELETION PROCESS~~~ There are" << PointLightsWithoutShadows.size() << " point lights right now";
	}
	oldkeystates[7] = state;
	state = myDevice->getKey(0, GLFW_KEY_G);
	if (state == GLFW_PRESS && oldkeystates[8] != GLFW_PRESS) {
		// Make a light
		float tempr = (rand() % 600) / 900.0 + 0.33333333333333333333;
		float tempg = (rand() % 600) / 900.0 + 0.33333333333333333333;
		float tempb = (rand() % 600) / 900.0 + 0.33333333333333333333;
		// float tempr = 1;
		// float tempg = 1;
		// float tempb = 1;
		PointLightsWithoutShadows.push_back(new PointLight(SceneRenderCamera->pos, glm::vec3(tempr, tempg, tempb)));
		PointLightsWithoutShadows[PointLightsWithoutShadows.size() - 1]->range = rand() % 100 + 100;
		PointLightsWithoutShadows[PointLightsWithoutShadows.size() - 1]->dropoff = 1;
		PointLightsWithoutShadows[PointLightsWithoutShadows.size() - 1]->setPos(SceneRenderCamera->pos);
		theScene->registerPointLight(PointLightsWithoutShadows[PointLightsWithoutShadows.size() - 1]);
	}
	oldkeystates[8] = state;
	state = myDevice->getKey(0, GLFW_KEY_LEFT_SHIFT);
	if (state == GLFW_PRESS) // We are testing for holding...
		holdingShift = true;
	else
		holdingShift = false;
	oldkeystates[9] = state;
	state = myDevice->getKey(0, GLFW_KEY_B);
	if (state == GLFW_PRESS && oldkeystates[10] != GLFW_PRESS) {
		std::cout << "\nCamera is located at X: " << SceneRenderCamera->pos.x << " Y: " << SceneRenderCamera->pos.y << " Z: " << SceneRenderCamera->pos.z;

		std::cout << "\n We have " << ProgramMeshInstances.size() << " Mesh Instances being rendered every frame.";

		std::cout << "\n We have " << PointLightsWithoutShadows.size() << " Point Lights being rendered every frame.";

		std::cout << "\nMAIN: " << ProgramMeshInstances.size() << " Instanced Model Instances: " << InstancedMesh->numInstances()
				  << " Sphere Model Instances: " << FileResourceManager->loadMesh("sphere_test.obj", false, true)->numInstances();

		std::cout << "\n WIDTH: " << WIDTH << "\n HEIGHT: " << HEIGHT;

		RenderTargetCamera = *SceneRenderCamera;
		RenderTargetCamera.buildPerspective(70, 1, 1, 5000);
		RenderTargetCamera.moveRight((float)(rand() % 1000 - 500.0f) / 500.0f);
		RenderTargetCamera.moveForward((float)(rand() % 1000 - 500.0f) / 500.0f);
		RenderTargetCameraShadowMapping = Camera(SceneRenderCamera->pos + glm::vec3(0, 20, 0), // World Pos
												 70.0f,										   // FOV
												 1.0f,										   // Aspect
												 1.0f,										   // Znear
												 500.0f,									   // Zfar
												 glm::vec3(0.0f, -1.0f, 0.0f),				   // forward
												 glm::vec3(0.0f, 0.0f, 1.0f));				   // Up;
		//~ RenderTargetCameraShadowMapping = *SceneRenderCamera;
		std::cout << "\n The Rendertarget Camera has been moved to the "
					 "Camera's location";
		// Test FBO Information
		// FBOArray[0]->printcolorattachments();
		//~ creepilyfacetowardthecamera = !creepilyfacetowardthecamera;
		if (Cam_Lights.size() > 0)
			Cam_Lights[0]->myCamera = RenderTargetCameraShadowMapping;
	}
	oldkeystates[10] = state;
	state = myDevice->getKey(0, GLFW_KEY_N);
	if (state == GLFW_PRESS && oldkeystates[11] != GLFW_PRESS) {
		useFBO = !useFBO;
	}
	oldkeystates[11] = state;
	state = myDevice->getKey(0, GLFW_KEY_Y);
	if (state == GLFW_PRESS && oldkeystates[12] != GLFW_PRESS) {
		if (PointLightsWithoutShadows.size() > 0)
			PointLightsWithoutShadows[0]->setPos(SceneRenderCamera->pos);
	}
	oldkeystates[12] = state;
	state = myDevice->getKey(0, GLFW_KEY_ESCAPE);
	if (state == GLFW_PRESS && oldkeystates[13] != GLFW_PRESS) {
		shouldQuit = true;
	}
	oldkeystates[13] = state;
	state = myDevice->getKey(0, GLFW_KEY_E);
	if (state == GLFW_PRESS && oldkeystates[14] != GLFW_PRESS)
		for (int i = 0; i < 25; i++) {
			// Test out the Meshinstance registration feature
			ProgramMeshInstances.push_back(
				new MeshInstance((rand() % 100 > 50) ? 1 : 0, // Texture, demo of inline if. It's either 1 or 0.
								 Transform(SceneRenderCamera->pos + glm::vec3(rand() % 100 - 50, rand() % 100 - 50, rand() % 100 - 50),
										   glm::vec3(rand() % 100 - 50, rand() % 100 - 50, rand() % 100 - 50), glm::vec3(1, 1, 1))));
			// Randomly decide whether or not to use Cubemap Reflections
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->EnableCubemapDiffusion = 0;
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->EnableCubemapReflections = (rand() % 100 > 25) ? 1 : 0;
			//~
			// ProgramMeshInstances[ProgramMeshInstances.size()-1]->EnableCubemapReflections
			//= 0;

			// Randomly decide which cubemap to use.
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->cubeMap = 0;

			// Random Specular Dampening
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->myPhong.specdamp = 30;

			// Random Emissivity
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->myPhong.emissivity = 0;

			// Random Specular Reflectivity
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->myPhong.specreflectivity = 0.3;

			// Random diffusivity
			//~
			// ProgramMeshInstances[ProgramMeshInstances.size()-1]->myPhong.diffusivity
			//=
			// 1-ProgramMeshInstances[ProgramMeshInstances.size()-1]->myPhong.specreflectivity;
			////looks better ~
			// ProgramMeshInstances[ProgramMeshInstances.size()-1]->myPhong.diffusivity
			//= rand()%100/100.0;
			ProgramMeshInstances[ProgramMeshInstances.size() - 1]->myPhong.diffusivity = 1.0;

			FileResourceManager->loadMesh("sphere_test.obj", false, true)->registerInstance(ProgramMeshInstances[ProgramMeshInstances.size() - 1]);

			// Grab the bones from the mesh class and then create the UBO
			//~
			// FileResourceManager->loadMesh("sphere_test.obj",false,true)->PassBoneInfoToMeshInstance(*ProgramMeshInstances[ProgramMeshInstances.size()-1]);

			//~
			// ProgramMeshInstances[ProgramMeshInstances.size()-1]->BuildBoneUBO(true,
			// true);
			// std::cout << "\n CREATED A " <<name;
		}
	oldkeystates[14] = state;
	state = myDevice->getKey(0, GLFW_KEY_Q);
	if (state == GLFW_PRESS && oldkeystates[15] != GLFW_PRESS)
		for (int i = 0; i < 10; i++) {
			if (ProgramMeshInstances.size() > 0 &&
				FileResourceManager->loadMesh("sphere_test.obj", false, true)->numInstances() > 0) // if there are any to deregister
			{
				int haveDeregistered = 0;
				int i = ProgramMeshInstances.size() - 1;
				while (haveDeregistered < 1 && i > -1) // While we might be able to deregister
				{
					bool donedeed = FileResourceManager->loadMesh("sphere_test.obj", false, true)->deregisterInstance(ProgramMeshInstances[i]);
					if (donedeed) {
						delete ProgramMeshInstances[i];
						ProgramMeshInstances.erase(ProgramMeshInstances.begin() + i);
						haveDeregistered++;
					}
					i--;
				}
				if (haveDeregistered < 1)
					std::cout << "\n ERROR! Unable to delete";
			}
		}
	oldkeystates[15] = state;
	state = myDevice->getKey(0, GLFW_KEY_I);
	if (state == GLFW_PRESS && oldkeystates[16] != GLFW_PRESS) {
		if (deregisterOnButtonPress)
			theScene->fastAndSafeDeregistration(nullptr, FileResourceManager->loadTexture("AMIGA.PNG", true).getHandle());
		else {
			FileResourceManager->loadMesh("sphere_test.obj", false, true)->pushTexture(FileResourceManager->loadTexture("AMIGA.PNG", true));
		}
		deregisterOnButtonPress = !deregisterOnButtonPress;
	}
	oldkeystates[16] = state;
	state = myDevice->getKey(0, GLFW_KEY_SPACE);
	if (state == GLFW_PRESS && oldkeystates[17] != GLFW_PRESS) {
		alSourcePlay(audiosource1);
	}
	oldkeystates[17] = state;
	state = myDevice->getKey(0, GLFW_KEY_V);
	if (state == GLFW_PRESS && oldkeystates[18] != GLFW_PRESS) {
		// Move the second cameralight to be the same as the camera
		if (Cam_Lights.size() > 1) {
			Cam_Lights[1]->myCamera = *SceneRenderCamera;
			Cam_Lights[1]->myCamera.buildPerspective(70, 1, 1, 5000);
		}
	}
	oldkeystates[18] = state;
	state = myDevice->getKey(0, GLFW_KEY_T);
	if (state == GLFW_PRESS && oldkeystates[19] != GLFW_PRESS) {
		std::cout << "\nLook ahead of you" << std::endl;
		glm::vec3 forward = SceneRenderCamera->forward;
		forward.x *= 10;
		forward.y *= 10;
		forward.z *= 10;
		Generated_Sphere_Tester.myTransform.setPos(SceneRenderCamera->pos + (forward));
	}
	oldkeystates[19] = state;

	// Mouse Keys. Yes, I know this is shitty and inefficient
	state = myDevice->getMouseButton(0, GLFW_MOUSE_BUTTON_LEFT);
	if (state == GLFW_PRESS && (oldkeystates[49] != GLFW_PRESS)) {
		double mousepos[2];
		myDevice->getCursorPosition(0, &mousepos[0], &mousepos[1]);
		std::cout << "\n X: " << (mousepos[0] / WIDTH);
		std::cout << "\n Y: " << (mousepos[1] / HEIGHT);

		std::cout << "\nsizeof int pointer (proves this is 64 bit if its 8):" << sizeof(int*);
		std::cout << "\nsizeof long long int: " << sizeof(long long int);
		InstancedMesh->optimizeCacheMemoryUsage();
		FileResourceManager->loadMesh("sphere_test.obj", false, true)->optimizeCacheMemoryUsage();
		if (DeleteMeshTest) {
			theScene->deregisterMesh(DeleteMeshTest);
			delete DeleteMeshTest;
			DeleteMeshTest = nullptr;
		} else {
			std::cout << "\nMaking Mesh";
			DeleteMeshTest = new gekRender::Mesh("SPHERE_TEST.OBJ", false, true, true);
			std::cout << "\n" << DeleteMeshTest->getVAOHandle();
			theScene->registerMesh(DeleteMeshTest);
			DeleteMeshTest->registerInstance(&LetterTester);
			DeleteMeshTest->pushTexture(FileResourceManager->loadTexture("AMIGA.PNG", true));
		}
	}
	oldkeystates[49] = state;
	state = myDevice->getMouseButton(0, GLFW_MOUSE_BUTTON_RIGHT);
	if (state == GLFW_PRESS && oldkeystates[50] != GLFW_PRESS) {
		// DEMO: Exporting models
		IndexedModel Boxy = createBox(1.0f, 4.0f, 3.0f);
		std::string OBJExportedBoxy = Boxy.exportToString(true);
		std::cout << "\n BEGIN PRINTOUT:\n";
		std::cout << OBJExportedBoxy;
	}
	oldkeystates[50] = state;
}

void window_size_callback(GLFWwindow* window, int width, int height) {
	std::cout << "\n\nWindow resized!!!" << std::endl;
	using namespace gekRender;
	if (height == 0 || width == 0)
		return;
	if (theScene)
		theScene->resizeSceneViewport(width, height, 1.0);
	WIDTH = width;
	HEIGHT = height;
	if (SceneRenderCamera != nullptr)
		SceneRenderCamera->buildPerspective(70, ((float)WIDTH) / ((float)HEIGHT), 1, 1000);

	if (myFont != nullptr)
		myFont->resize(width, height, UI_SCALE_FACTOR);
}

void character_callback(GLFWwindow* window,
						unsigned int codepoint) // Used for string input
{}

// Sets up the window, opengl context, openal context, and generally everything
// you need for games
void init() {
	// Why didn't I just put this at the top of the file???
	using namespace gekRender;
	// Creates the GLFW context. This pretty much has to be the first code we
	// run
	myDevice->initGLFW();
	// GLFW requires you to "push" properties of the next window to be created
	// before making it
	myDevice->pushWindowCreationHint(GLFW_RESIZABLE, GLFW_TRUE);
	// myDevice->pushWindowCreationHint(GLFW_RESIZABLE, GLFW_FALSE);
	// Set Version to OpenGL 3.3
	myDevice->pushWindowCreationHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	myDevice->pushWindowCreationHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//~ //Enable Anti-Aliasing
	//~ myDevice->pushWindowCreationHint(GLFW_SAMPLES, 4);
	// FINALLY add the window
	myDevice->addWindow(WIDTH, HEIGHT, "GkScene API on gekRender + OpenAL, Demo Program 1");
	// myDevice->addFullScreenWindow(WIDTH, HEIGHT, "GkScene API Alpha- Demo
	// Program 1");

	myDevice->setWindowSizeCallback(0,
									window_size_callback); // Self explanatory
	myDevice->setContext(0);							   // Binds GL context of window 0 to current thread
	myDevice->initGL3W();								   // Initialize the OpenGL extension wrangler.
	myDevice->setKeyCallBack(0, key_callback);			   // Set a callback function.

	// Default Enabled OpenGL Options
	glEnable(GL_CULL_FACE);  // Enable culling faces
	glEnable(GL_DEPTH_TEST); // test fragment depth when rendering
	glCullFace(GL_BACK);	 // cull faces with clockwise winding
	cameralock = false;
	for (int i = 0; i < 51; i++)
		oldkeystates[i] = 0;
	for (int i = 0; i < 2; i++)
		oldmousexy[i] = 0;

	for (int i = 0; i < 2; i++)
		currentmousexy[i] = 0;
	theScene = new GkScene(WIDTH, HEIGHT, 1.0);
	// Initialize OpenAL
	myDevice->fastInitOpenAL();
}

// Load the resources from file for the demo.
void LoadResources() {
	using namespace gekRender;
	// it puts .vs and .fs after the string.
	MainShad = new Shader("shaders/FORWARD_MAINSHADER_UBO");
	MainshaderShadows = new Shader("shaders/FORWARD_MAINSHADER_SHADOWS");
	DisplayTexture = new Shader("shaders/SHOWTEX");
	SkyboxShad = new Shader("shaders/Skybox");
	WBOITCompShader = new Shader("shaders/WBOIT_COMPOSITION_SHADER");
	theScene->setSkyboxShader(SkyboxShad);
	theScene->setMainShader(MainShad);
	theScene->ShadowOpaqueMainShader = MainshaderShadows;
	theScene->ShowTextureShader = DisplayTexture;		  // Add a setter later
	theScene->setWBOITCompositionShader(WBOITCompShader); // Has a setter, and it's been a long time, I should
														  // write a setter for the ShowTextureShader
	FBOArray.push_back(new FBO(640, 480, 1,
							   GL_RGBA32F)); // 0, FBO for rendering shadowmaps for Cameralight 0
	FBOArray.push_back(new FBO(640, 480, 1,
							   GL_RGBA32F));			// 1, FBO for rendering shadowmaps for Cameralight 1
	FBOArray.push_back(new FBO(640, 480, 1, GL_RGBA8)); // Honest Render-to-target
	FBOArray.push_back(new FBO(640, 480, 2, GL_RGBA16F,
							   FBOArray[2]->getDepthBufferHandle())); // 3, Honest render-to-target,
																	  // transparent
	//~ theScene->registerCustomFBO(FBOArray[0]);						  // I was planning to use this to do reflections in the
																	  //~ // future, so I added a custom FBO registration feature.

	std::string cubemapfilenames[6] = {
		"Cubemap/Skybox_Water10_128_right.jpg", // right
		"Cubemap/Skybox_Water10_128_left.jpg",  // left
		"Cubemap/Skybox_Water10_128_top.jpg",   // up
		"Cubemap/Skybox_Water10_128_base.jpg",  // down
		"Cubemap/Skybox_Water10_128_back.jpg",  // back
		"Cubemap/Skybox_Water10_128_front.jpg"  // front
	};
	SkyboxTex = new CubeMap(cubemapfilenames[0], cubemapfilenames[1], cubemapfilenames[2], cubemapfilenames[3], cubemapfilenames[4], cubemapfilenames[5]);
	std::cout << "\nMARKER 0" << std::endl;
	std::string secondCubeMapFileNames[6] = {"Cubemap/2desert_RIGHT.bmp", "Cubemap/2desert_LEFT.bmp", "Cubemap/2desert_UP.bmp",
											 "Cubemap/2desert_DOWN.bmp",  "Cubemap/2desert_BACK.bmp", "Cubemap/2desert_FRONT.bmp"};
	SkyboxTwo = new CubeMap(secondCubeMapFileNames[0], secondCubeMapFileNames[1], secondCubeMapFileNames[2], secondCubeMapFileNames[3],
							secondCubeMapFileNames[4], secondCubeMapFileNames[5]);
	theScene->setSkyBoxCubemap(SkyboxTex);
	std::cout << "\nMARKER 0.5" << std::endl;
	//~ FileResourceManager->loadMesh("sphere_test.obj", false, true);
	//~ FileResourceManager->loadMesh("FIRST_TERRAIN_WITH_CAVES.OBJ", false, true);
	InstancedMesh = new Mesh("Cube_Test_Low_Poly.obj", true, false,
							 true); // Instanced, Static, Asset
	std::cout << "\nMARKER 1" << std::endl;
	IndexedModel GenSphereModel = createSphere(2, 10, 10, glm::vec3(1, 0, 0));
	Generated_Sphere = new Mesh(GenSphereModel, false, true, true);
	theScene->registerMesh(Generated_Sphere);
	Generated_Sphere->registerInstance(&Generated_Sphere_Tester);

	FileResourceManager->loadTexture("AMIGA.PNG", true);
	FileResourceManager->loadTexture("ART.JPG", false);

	// MESH REGISTRATION. This is how you tell the scene what meshes to draw
	// every frame
	theScene->registerMesh(FileResourceManager->loadMesh("sphere_test.obj", false, true));
	// theScene->registerMesh(FileResourceManager->loadMesh("Cube_Test_Low_Poly.obj",false,true));
	theScene->registerMesh(FileResourceManager->loadMesh("FIRST_TERRAIN_WITH_CAVES.OBJ", false, true));
	// register the instanced
	theScene->registerMesh(InstancedMesh);

	// NotPointedTextureTest = Texture("AMIGA.PNG", true);

	// Sphere_test Available Textures.
	FileResourceManager->loadMesh("sphere_test.obj", false, true)->pushTexture(FileResourceManager->loadTexture("AMIGA.PNG", true)); // 0
	// FileResourceManager->loadMesh("sphere_test.obj",false,true)->pushTexture(FileResourceManager->loadTexture("ART.JPG",false));
	// //1
	FileResourceManager->loadMesh("sphere_test.obj", false, true)->pushTexture(FileResourceManager->loadTexture("CLOUDS.JPG", false)); // 1
	std::cout << "\nMARKER 2" << std::endl;
	// Render the RenderTarget as a texture on the instanced cube
	InstancedMesh->pushTexture(FBOArray[2]->getTex(0));
	InstancedMesh->pushCubeMap(SkyboxTex);

	// Sphere_test Cubemaps
	FileResourceManager->loadMesh("sphere_test.obj", false, true)->pushCubeMap(SkyboxTwo); // 0
	FileResourceManager->loadMesh("sphere_test.obj", false, true)->pushCubeMap(SkyboxTex); // 1

	std::cout << "\nMARKER 3" << std::endl;
	myFont = new BMPFontRenderer("geks Bitmap ASCII font 16x16.bmp", WIDTH, HEIGHT, UI_SCALE_FACTOR,
								 "shaders/SHOWTEX"); // Load font for testing
	// Images for testing the font
	Images.push_back(Image_Data("sprite.png"));

	// Water Rendering~ Making the mesh
	IndexedModel WaterModel;
	// We want a single quad with X dimension 100 and Z dimension 100, normals
	// facing into positive Y and visible only from the top. The positions are
	// as follows, x,z -50,50 TOP LEFT -50,-50 BOTTOM LEFT 50, -50 BOTTOM RIGHT
	// 50, 50 TOP RIGHT

	WaterModel.positions.push_back(glm::vec3(50, 0, 50));   // TRIANGLE 1, TOP RIGHT CORNER
	WaterModel.positions.push_back(glm::vec3(-50, 0, -50)); // TRIANGLE 1, BOTTOM LEFT CORNER
	WaterModel.positions.push_back(glm::vec3(-50, 0, 50));  // TRIANGLE 1, TOP LEFT CORNER

	WaterModel.positions.push_back(glm::vec3(50, 0, 50));   // TRIANGLE 2, TOP RIGHT CORNER
	WaterModel.positions.push_back(glm::vec3(50, 0, -50));  // TRIANGLE 2, BOTTOM RIGHT CORNER
	WaterModel.positions.push_back(glm::vec3(-50, 0, -50)); // TRIANGLE 2, BOTTOM LEFT CORNER

	// Push on normals, pointing straight up
	WaterModel.normals.push_back(glm::vec3(0, 1, 0));
	WaterModel.normals.push_back(glm::vec3(0, 1, 0));
	WaterModel.normals.push_back(glm::vec3(0, 1, 0));

	WaterModel.normals.push_back(glm::vec3(0, 1, 0));
	WaterModel.normals.push_back(glm::vec3(0, 1, 0));
	WaterModel.normals.push_back(glm::vec3(0, 1, 0));
	// Texture coordinates
	WaterModel.texCoords.push_back(glm::vec2(1, 1)); // TRIANGLE 1, TOP RIGHT CORNER
	WaterModel.texCoords.push_back(glm::vec2(0, 0)); // TRIANGLE 1, BOTTOM LEFT CORNER
	WaterModel.texCoords.push_back(glm::vec2(1, 0)); // TRIANGLE 1, TOP LEFT
													 // CORNER

	WaterModel.texCoords.push_back(glm::vec2(1, 1)); // TRIANGLE 2, TOP RIGHT CORNER
	WaterModel.texCoords.push_back(glm::vec2(0, 1)); // TRIANGLE 2, BOTTOM RIGHT CORNER
	WaterModel.texCoords.push_back(glm::vec2(0, 0)); // TRIANGLE 2, BOTTOM LEFT CORNER

	// Colors, we don't need to use it, but I make it blue for good measure
	WaterModel.colors.push_back(glm::vec3(0.3, 0.3, 1));
	WaterModel.colors.push_back(glm::vec3(0.3, 0.3, 1));
	WaterModel.colors.push_back(glm::vec3(0.3, 0.3, 1));

	WaterModel.colors.push_back(glm::vec3(0.3, 0.3, 1));
	WaterModel.colors.push_back(glm::vec3(0.3, 0.3, 1));
	WaterModel.colors.push_back(glm::vec3(0.3, 0.3, 1));

	// Indices. We need 6, and in order. We could have been more efficient by
	// not repeating vertex info but eh what can ya do
	for (unsigned int i = 0; i < 6; i++)
		WaterModel.indices.push_back(i);

	// Make the mesh
	WaterSurfaceMesh = new Mesh(WaterModel, false, true, true);
	theScene->registerMesh(WaterSurfaceMesh);
	WaterSurfaceMesh->registerInstance(&WaterSurfaceInstance);
	WaterSurfaceInstance.myTransform = Transform(glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(10, 10, 10));
	WaterSurfaceInstance.EnableCubemapReflections = true;
	WaterSurfaceInstance.myPhong.diffusivity = 0.5;
	WaterSurfaceInstance.myPhong.specreflectivity = 0.8;
	WaterSurfaceInstance.myPhong.emissivity = 0.0; // emissivity
	WaterSurfaceInstance.myPhong.specdamp = 48;
	WaterSurfaceMesh->setFlags(GK_RENDER | GK_TEXTURED);
	// Make the water surface texture
	unsigned char* Temp_Datum =
		(unsigned char*)malloc(sizeof(unsigned char) * FLUID_WIDTH * FLUID_HEIGHT * 4); // There are 4 components, i.e. 4 bytes per pixel
	// Color it faint blue, half visible
	for (int i = 0; i < FLUID_WIDTH * FLUID_HEIGHT; i++) {
		Temp_Datum[i * 4] = 0;
		Temp_Datum[i * 4 + 1] = 0;
		Temp_Datum[i * 4 + 2] = 128;
		Temp_Datum[i * 4 + 3] = 128;
	}
	WaterSurfaceTexture = new Texture(FLUID_WIDTH, FLUID_HEIGHT, 4, Temp_Datum);
	WaterCausticsTexture = new Texture(FLUID_WIDTH, FLUID_HEIGHT, 4, Temp_Datum);
	WaterSurfaceTexture->setTransparency(true);
	free(Temp_Datum);
	WaterSurfaceMesh->pushTexture(SafeTexture(WaterSurfaceTexture));
	WaterSurfaceMesh->pushCubeMap(SkyboxTex);
	Fluid_Target = WaterSurfaceTexture->getDataPointerNotConst();
	Caustic_Target = WaterCausticsTexture->getDataPointerNotConst();
	// Prep the fluid simulation
	for (int i = 0; i < FLUID_WIDTH; i++) {
		for (int j = 0; j < FLUID_HEIGHT; j++) {

			u[i][j] = 0.0;
			u_new[i][j] = u[i][j];
			v[i][j] = 0.0;
		}
	}
	// Custom rendering pipeline callbacks for your rendering needs
	theScene->customMainShaderBinds = &MainshaderUniformFunctionDemo;
	theScene->customRenderingAfterSkyboxBeforeMainShader = &CustomRenderingFunction; // Draw to your heart's content!

	// See GekAL.h and file resource manager for how this is done
	audiobuffer1 = FileResourceManager->loadSound("SOUNDS/FM HOUSE.WAV");
	std::cout << "\nMARKER 4" << std::endl;
}

void initObjects() {			   // Default existing objects
	using namespace gekRender; // sue me
	std::cout << "\nBeginning of initObjects" << std::endl;
	// Notice: We are using a fixed instance to draw this
	FileResourceManager->loadMesh("FIRST_TERRAIN_WITH_CAVES.OBJ", false, true)->registerInstance(&TerrainInstance);
	FileResourceManager->loadMesh("FIRST_TERRAIN_WITH_CAVES.OBJ", false, true)
		->pushTexture(FileResourceManager->loadTexture("CLOUDS.JPG", false)); // Note: we are Loading a texture here...
																			  // should probably move to LoadResources

	//                                      position          Euler Rotation
	//                                      Scale
	TerrainInstance.myTransform = Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(20, 20, 20));
	TerrainInstance.myPhong.specreflectivity = 0; // Not shiny at all. Chalk.
	TerrainInstance.myPhong.diffusivity = 1;	  // How much does diffuse light affect the drawing
	TerrainInstance.mymeshmask = 4;				  // DIVISIBLE BY: 4, 2, 1 so if you call the rendering function with
												  // any other number it will not render this mesh (Negative numbers
												  // not withstanding)

	Generated_Sphere_Tester.myTransform = Transform(glm::vec3(40, 29, 90), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
	//~~~~~~~~~~~~~~~~~~~~~~~~~
	// Lights...
	PointLightsWithoutShadows.push_back(new PointLight(glm::vec3(10, 20, 10), glm::vec3(1, 1, 1))); // 0 LIGHT
	PointLightsWithoutShadows[0]->range = 500;														// range is NOT squared
	PointLightsWithoutShadows[0]->dropoff = 1;
	theScene->registerPointLight(PointLightsWithoutShadows[0]);

	//~ Dir_Lights.push_back(new DirectionalLight()); // 0
	//~ if (Dir_Lights.size() > 0) {
		//~ Dir_Lights[0]->myDirection = glm::vec3(0, -1, 0);
		//~ Dir_Lights[0]->myColor = glm::vec3(0.3, 0.3, 0.3);
		//~ theScene->registerDirLight(Dir_Lights[0]);
	//~ }

	// register an Ambient Light
	Amb_Lights.push_back(new AmbientLight()); // 0
	if (Amb_Lights.size() > 0)				  // if we successfully made one
	{
		Amb_Lights[0]->myColor = glm::vec3(0.2, 0.2, 0.2);
		Amb_Lights[0]->myPos = glm::vec3(0, 50, 0);
		Amb_Lights[0]->range = 5000 * 5000; // Note: This creates a harsh cutoff. Fix later?
		theScene->registerAmbLight(Amb_Lights[0]);
		//~ Amb_Lights[0]->sphere1 = glm::vec4(0, 0, 0, 300 * 300);
	}
	// the SceneRender camera
	SceneRenderCamera = new Camera(glm::vec3(0, 50, -10),		   // World Pos
								   70.0f,						   // FOV
								   ((float)WIDTH / (float)HEIGHT), // Aspect
								   1.0f,						   // Znear
								   1000.0f,						   // Zfar
								   glm::vec3(0.0f, 0.0f, 1.0f),	// forward
								   glm::vec3(0.0f, 1.0f, 0.0f));   // Up

	RenderTargetCameraShadowMapping = Camera(glm::vec3(0, 50, -10),		   // World Pos
											 70.0f,						   // FOV
											 1.0f,						   // Aspect
											 1.0f,						   // Znear
											 5000.0f,					   // Zfar
											 glm::vec3(0.0f, -1.0f, 0.0f), // forward
											 glm::vec3(0.0f, 0.0f, 1.0f)); // Up
	// RenderTargetCameraShadowMapping = *SceneRenderCamera;
	RenderTargetCameraShadowMapping2 = RenderTargetCameraShadowMapping;
	RenderTargetCamera = *SceneRenderCamera;
	// Set the main render camera for the scene. You MUST set the main camera
	// otherwise the main rendering function will NOT run.
	theScene->setSceneCamera(SceneRenderCamera);
	Cam_Lights.push_back(new CameraLight());
	// std::cout << "\nRight before creating camera lights" << std::endl;
	//~ Cam_Lights[0]->Tex2Project =
	// FileResourceManager->loadTexture("ART.JPG",false);
	Cam_Lights[0]->Tex2Project = FBOArray[0]->getTex(0);
	Cam_Lights[0]->Dimensions = glm::vec2(1 / 640.0f, 1 / 480.0f); // Dimensions of the shadowmap
	Cam_Lights[0]->isShadowed = true;							   // Are we using a Shadowmap?
	Cam_Lights[0]->solidColorToggle = 1.0;						   // Are we projecting the camera's myColor?
	Cam_Lights[0]->myColor = glm::vec3(1, 0, 0);				   // Color of this light
	Cam_Lights[0]->range = 600;									   // Range of this light
	Cam_Lights[0]->radii = glm::vec2(0.3, 0.5);					   // Radii for spot
	theScene->registerCamLight(Cam_Lights[0]);
	Cam_Lights[0]->myCamera = RenderTargetCameraShadowMapping; // set this cameralight's camera.

	// Testing a second one
	Cam_Lights.push_back(new CameraLight());
	Cam_Lights[1]->Tex2Project = FBOArray[1]->getTex(0);
	Cam_Lights[1]->Dimensions = glm::vec2(1 / 640.0f, 1 / 480.0f); // Dimensions of the shadowmap, inverse
	Cam_Lights[1]->isShadowed = true;
	Cam_Lights[1]->solidColorToggle = 1.0;
	Cam_Lights[1]->myColor = glm::vec3(0.5, 0.5, 1);
	Cam_Lights[1]->range = 600;
	Cam_Lights[1]->radii = glm::vec2(0.4, 0.5);
	theScene->registerCamLight(Cam_Lights[1]);
	Cam_Lights[1]->myCamera = Camera(glm::vec3(100, 300, 200),	 // World Pos
									 70.0f,						   // FOV
									 1,							   // Aspect
									 1.0f,						   // Znear
									 5000.0f,					   // Zfar
									 glm::vec3(0.0f, -1.0f, 0.0f), // forward
									 glm::vec3(1.0f, 0.0f, 0.0f)); // Up
	// Caustics Projector
	Cam_Lights.push_back(new CameraLight());
	Cam_Lights[2]->Tex2Project = SafeTexture(WaterCausticsTexture);
	Cam_Lights[2]->isShadowed = false;
	Cam_Lights[2]->solidColorToggle = 0.0;
	Cam_Lights[2]->myColor = glm::vec3(1.0, 1.0, 1.0);
	Cam_Lights[2]->range = -3000; // Negative means it uses the shader's lightposition variable as
								  // direction, and it passes myCamera->forward as lightposition. This is Proper for
								  // Orthogonal Projectors (Who use worldspace coords)
	Cam_Lights[2]->radii = glm::vec2(2, 2);
	theScene->registerCamLight(Cam_Lights[2]);
	Cam_Lights[2]->myCamera = Camera(glm::vec3(0, -5, 0),		   // World Pos
									 70.0f,						   // FOV
									 1,							   // Aspect
									 1.0f,						   // Znear
									 300.0f,					   // Zfar
									 glm::vec3(0.0f, -1.0f, 0.0f), // forward
									 glm::vec3(1.0f, 0.0f, 0.0f)); // Up
	Cam_Lights[2]->myCamera.buildOrthogonal(-500, 500, -500, 500, 1, 300);

	// Cam_Lights.push_back(new CameraLight());
	// std::cout << "\nRight after creating camera lights" << std::endl;

	// OpenAL Source Generation
	alGenSources(1, &audiosource1);
	alSourcef(audiosource1, AL_GAIN, 10);
	alSourcef(audiosource1, AL_PITCH, 1);
	// Params
	alSource3f(audiosource1, AL_POSITION, 0, 0, 0);
	alSource3f(audiosource1, AL_VELOCITY, 0, 0, 0);
	alSourcei(audiosource1, AL_LOOPING, AL_FALSE);
	// What sound?
	alSourcei(audiosource1, AL_BUFFER, audiobuffer1);
	std::cout << "\nEnd of initObjects" << std::endl;
}

int main() {
	using namespace gekRender; // bad practice but I don't care
	init();						   // Sets up all the OpenGL setup and stuff
	LoadResources();			   // Load models and shaders and stuff
	initObjects();				   // Create the initial objects we place into the scene

	// STACK-ALLOCATED OBJECTS
	// Most things in gekRender have to be Malloced, but some classes can be
	// made on the stack. For the most protection against error, I recommend
	// using malloced versions, but stack-allocated versons may be better for
	// beginners
	Mesh BadDeer = Mesh("Bad Deer.obj", false, true, false);
	Mesh BlobMan = Mesh("Blob_Man.obj", false, true, false);
	Texture NotPointedTextureTest = Texture("AMIGA.PNG", true);
	Texture NotPointedTextureTest2 = Texture("ART.JPG", false);
	BadDeer.pushTexture(SafeTexture(&NotPointedTextureTest));  // 0
	BlobMan.pushTexture(SafeTexture(&NotPointedTextureTest2)); // 0
	BadDeer.pushCubeMap(SkyboxTex);							   // 0
	BlobMan.pushCubeMap(SkyboxTex);							   // 0
	theScene->registerMesh(&BadDeer);
	theScene->registerMesh(&BlobMan);
	MeshInstance MyVeryBadDeer = MeshInstance();
	MeshInstance MyBlobbyMan = MeshInstance();
	MyVeryBadDeer.tex = 0;
	MyVeryBadDeer.EnableCubemapReflections = 1;
	MyVeryBadDeer.myPhong.diffusivity = 0.6;
	MyVeryBadDeer.myPhong.specreflectivity = 0.2;
	MyVeryBadDeer.myPhong.specdamp = 128;
	MyVeryBadDeer.myPhong.emissivity = 0;
	MyVeryBadDeer.myTransform.setScale(glm::vec3(10, 10, 10));
	MyVeryBadDeer.myTransform.setPos(glm::vec3(0, 20, 50));
	// For blob man
	MyBlobbyMan.tex = 0;
	MyBlobbyMan.EnableCubemapReflections = 1;
	MyBlobbyMan.myPhong.diffusivity = 1.0;
	MyBlobbyMan.myPhong.specreflectivity = 0.05;
	MyBlobbyMan.myPhong.specdamp = 128;
	MyBlobbyMan.myPhong.emissivity = 0;
	MyBlobbyMan.myTransform.setScale(glm::vec3(10, 10, 10));
	MyBlobbyMan.myTransform.setPos(glm::vec3(0, 20, 100));
	BadDeer.registerInstance(&MyVeryBadDeer); // demonstrating that you can use meshes on the stack
	BlobMan.registerInstance(&MyBlobbyMan);
	float ordinarycounter = 0.0f; // Used to achieve the trigonometry wave effects
	float dropcounter = 0.0f;
	int numdrops = 0;
	int UI_WIDTH = UI_SCALE_FACTOR * WIDTH;
	int UI_HEIGHT = UI_SCALE_FACTOR * HEIGHT;
	std::vector<glm::vec4> Sprite_Draw_Pos; // posx, posy, scale, 1/speed
	if (true)
		for (int i = 0; i < 10; i++)
			Sprite_Draw_Pos.push_back(glm::vec4(rand() % UI_WIDTH, rand() % UI_HEIGHT, (float)(rand() % 1000) / (float)500, rand() % 40 + 5));

	while (!myDevice->shouldClose(0) && !shouldQuit) // Main game loop.
	{
		// COLLECT INFORMATION ABOUT THE SYSTEM
		progtime = IODevice::getTime();
		ordinarycounter += 0.1f;
		if (ordinarycounter > 1000.0f) {
			ordinarycounter = 0.0f;
		}
		dropcounter += 0.0166666666f;
		if (dropcounter > 1000) {
			dropcounter = 0.0;
			numdrops = 0;
		}

		// INPUT DETECTION
		// We would re-route all information from input detection directly into
		// other classes. E.g. we might have a player class and we would pass it
		// the distance that the player moved their mouse. That would calculate
		// what the new camera orientation would be.
		checkKeys();

		// GAME CODE | Demonstrates using transforms to set the rotation and
		// position of an object in the scene, as well as properties and
		// changing shape.
		{
			// Changing shape demo
			// /*
			//~ IndexedModel tempshape = createBox(1, 1, 1);
			//~ for (size_t i = 0; i < tempshape.colors.size(); i++){
			//~ tempshape.colors[i] = glm::vec3((rand()%100)/100.0,
			//(rand()%100)/100.0, (rand()%100)/100.0);
			//~ }
			//~ InstancedMesh->reShapeMesh(createBox(3, 3, 7));
			//~ NotPointedTextureTest.reInitFromDataPointer(true);
			// */
			// LetterTester.myTransform.SetRot(glm::vec3(sinf(ordinarycounter/10.0)
			// * 5,sinf(ordinarycounter/10.0)*3,sinf(ordinarycounter/11.2)*2));
			// UI Rendering Demo
			//~ myFont->clearScreen(0,0,0,0.5);
			//~ for(int i = 0; i < 100; i++)
			//~ myFont->writeRectangle(rand()%(int)(WIDTH * UI_SCALE_FACTOR),
			// rand()%(int)(HEIGHT * UI_SCALE_FACTOR), ~ rand()%(int)(WIDTH *
			// UI_SCALE_FACTOR), rand()%(int)(HEIGHT * UI_SCALE_FACTOR), ~
			// rand()%256, rand()%256, rand()%256, rand()%128);
			myFont->clearScreen(0, 0, 0, 0);
			// myFont->writeCircle(rand()%WIDTH, rand()%HEIGHT, 10, rand()%255,
			// rand()%255, rand()%255, 50); myFont->writeCharacter('#',
			// rand()%WIDTH, rand()%HEIGHT, 32, 32, glm::vec3(128,128,255));
			myFont->writeString(std::string("Rendering Engine Main Demo"), 0, (int)(((float)HEIGHT * UI_SCALE_FACTOR) - 32), 32, 32, glm::vec3(100, 255, 100));
			//~ myFont->writeImage(
			//~ 0, 0,
			//~ myFont->getBMPFont(), myFont->getBMPFontWidth(),
			// myFont->getBMPFontHeight(), myFont->getBMPFontNumComponents(), ~
			// 0, 127, 0, 271, ~ 128, 272, ~ false, false);
			for (size_t i = 0; i < Sprite_Draw_Pos.size(); i++) {
				myFont->writeImage(Sprite_Draw_Pos[i].x + Sprite_Draw_Pos[i].z * 64 * sinf(ordinarycounter / Sprite_Draw_Pos[i].w),
								   Sprite_Draw_Pos[i].y + Sprite_Draw_Pos[i].z * 64 * cosf(ordinarycounter / Sprite_Draw_Pos[i].w), Images[0].data_ptr,
								   Images[0].width, Images[0].height, Images[0].num_components, 0, 255, 0, 255,
								   Sprite_Draw_Pos[i].z * 64 * abs(sinf(ordinarycounter / (Sprite_Draw_Pos[i].w + 21.0))),
								   Sprite_Draw_Pos[i].z * 64 * abs(sinf(ordinarycounter / (Sprite_Draw_Pos[i].w + 30.0))), false, false);
			}
			// Do and Draw Fluid Simulation
			// But first, randomly create drops
			if (numdrops < (int)dropcounter) {
				u[rand() % FLUID_WIDTH][rand() % FLUID_HEIGHT] += 15.0;
				numdrops++;
			}
			for (int i = 0; i < FLUID_WIDTH; i++)
				for (int j = 0; j < FLUID_HEIGHT; j++) {
					v[i][j] += (get_u(i - 1, j) + 
                                get_u(i + 1, j) + 
                                get_u(i, j - 1) + 
                                get_u(i, j + 1)) * 0.25 
                                - get_u(i, j);
					v[i][j] *= 0.999;
					u_new[i][j] += v[i][j];
					// myFont->writePixel(i, j, float_to_char((5 +
					// u_new[i][j])/10.0), float_to_char((5 +
					// u_new[i][j])/10.0), 255, 128);
					unsigned char r = float_to_char((5 + u_new[i][j]) / 10.0);
					unsigned char g = float_to_char((5 + u_new[i][j]) / 10.0);
					Fluid_Target[(i + j * FLUID_WIDTH) * 4 + 0] = 128 - float_to_char((u_new[i][j]) * 10.0) / 2;
					Fluid_Target[(i + j * FLUID_WIDTH) * 4 + 1] = 128 - float_to_char((u_new[i][j]) * 10.0) / 2;
					Fluid_Target[(i + j * FLUID_WIDTH) * 4 + 2] = 255;
					Fluid_Target[(i + j * FLUID_WIDTH) * 4 + 3] = 128;
					// Caustics are generated by taking the derivative of the
					// heightmap, aka sobel filter. It's a crude estimation.
					float dx = fabsf(get_u(i - 1, j) - u[i][j]);
					float dy = fabsf(get_u(i, j - 1) - u[i][j]);
					float dx_1 = fabsf(get_u(i + 1, j) - u[i][j]);
					float dy_1 = fabsf(get_u(i, j + 1) - u[i][j]);
					dx += dx_1;
					dy += dy_1;
					dx /= 2.0;
					dy /= 2.0;
					float steepness = sqrtf(dx * dx + dy * dy);

					// Write Caustics
					d_map[(i + j * FLUID_WIDTH) * 4 + 0] = float_to_char(steepness * 2);
					d_map[(i + j * FLUID_WIDTH) * 4 + 1] = float_to_char(steepness * 2);
					d_map[(i + j * FLUID_WIDTH) * 4 + 2] = float_to_char(steepness * 2);
					d_map[(i + j * FLUID_WIDTH) * 4 + 3] = 0;
				}
			// Update U
			for (int i = 0; i < FLUID_WIDTH; i++)
				for (int j = 0; j < FLUID_HEIGHT; j++) {
					u[i][j] = u_new[i][j];
					// blur the caustic target
					unsigned char r_u, r_l, r_d, r_r, r_o;
					float r, g, b;
					r_u = get_d(i, j - 1, 0);
					r_d = get_d(i, j + 1, 0);
					r_l = get_d(i - 1, j, 0);
					r_r = get_d(i + 1, j, 0);
					// average all 5 together
					r = (float)r_o + (float)r_u + (float)r_d + (float)r_l + (float)r_r;
					r /= 5.0;
					// write caustics
					Caustic_Target[(i + j * FLUID_WIDTH) * 4] = r;
					Caustic_Target[(i + j * FLUID_WIDTH) * 4 + 1] = r;
					Caustic_Target[(i + j * FLUID_WIDTH) * 4 + 2] = r;
					Caustic_Target[(i + j * FLUID_WIDTH) * 4 + 3] = 128;
				}
			// Reinit the fluid surface texture
			WaterSurfaceTexture->reinitFromDataPointer(false, true);
			WaterCausticsTexture->reinitFromDataPointer(false, true);

			for (size_t communists_killed = 0; communists_killed < ProgramMeshInstances.size() && communists_killed < 1000; communists_killed++) {
				// faceTowardPoint(glm::vec3 pos, glm::vec3 target, glm::vec3
				// up)
				//~ if (!creepilyfacetowardthecamera)
				//~
				// ProgramMeshInstances[communists_killed]->myTransform.SetRot(glm::vec3(sinf(ordinarycounter/10.0)
				//*
				// 5,sinf(ordinarycounter/10.0)*3,sinf(ordinarycounter/11.2)*2));
				//~ if (creepilyfacetowardthecamera)
				//~
				// ProgramMeshInstances[communists_killed]->myTransform.SetRotQuat(
				//~ gekRender::faceTowardPoint(
				//~
				// ProgramMeshInstances[communists_killed]->myTransform.getPos(),
				//~ SceneRenderCamera->pos,
				//~ glm::vec3(0,1,0)
				//~ )
				//~ );

				//*/
				// ProgramMeshInstances[communists_killed]->myPhong.specreflectivity
				// = (1+sinf(ordinarycounter/100.0f))/2.0;
			}
			// Camera Positioning
			if (cameralock) {
				myDevice->getCursorPosition(0, &currentmousexy[0], &currentmousexy[1]);
				SceneRenderCamera->pitch((float)(currentmousexy[1] - oldmousexy[1]) * 0.001f);
				SceneRenderCamera->rotateY((float)(currentmousexy[0] - oldmousexy[0]) * -0.001f);
				myDevice->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				myDevice->getCursorPosition(0, &oldmousexy[0], &oldmousexy[1]);

			} else {
				myDevice->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
			myDevice->getCursorPosition(0, &oldmousexy[0], &oldmousexy[1]);
		}
		// Eof Game Code
		syncCameraStateToALListener(SceneRenderCamera);
		//~ FBO::unBindRenderTarget(WIDTH, HEIGHT);
		//~ FBO::clearTexture(0, 0, 0, 0);
		// Render shadowmaps.
		theScene->drawShadowPipeline(1, FBOArray[0], &Cam_Lights[0]->myCamera, false);
		theScene->drawShadowPipeline(1, FBOArray[1], &Cam_Lights[1]->myCamera, false);
		// Render to rendertarget
		theScene->drawPipeline(1, FBOArray[2], FBOArray[3], &RenderTargetCamera, false, glm::vec4(0, 0, 0, 0), glm::vec2(800, 1000));
		// Render to screen.
		theScene->drawPipeline(1, nullptr, nullptr, nullptr, false, glm::vec4(0, 0, 0, 0), glm::vec2(800, 1000));
		myFont->pushChangestoTexture();
		myFont->draw(true);
		myFont->swapBuffers();
		myDevice->pollevents();
		myDevice->swapBuffers(0);
	} // EOF game loop

	// We should delete stuff... if we want to be good programmers :<
	std::cout << "\n BEGINNING DELETION...";
	if (WaterSurfaceMesh != nullptr)
		delete WaterSurfaceMesh;
	if (WaterSurfaceTexture != nullptr)
		delete WaterSurfaceTexture;
	if (WaterCausticsTexture != nullptr)
		delete WaterCausticsTexture;

	for (size_t i = 0; i < Images.size(); i++) {
		Images[i].unload();
	}
	delete InstancedMesh; // TODO: Push this to 147 //Later: what the fuck???
	delete FileResourceManager;
	if (MainShad)
		delete MainShad;
	if (MainshaderShadows)
		delete MainshaderShadows; // Shadowmap renderer
	if (SkyboxShad)
		delete SkyboxShad; // Skybox shader, passed to the scene class.
	if (DisplayTexture)
		delete DisplayTexture;
	if (WBOITCompShader) {
		delete WBOITCompShader;
	}
	if (SceneRenderCamera) {
		delete SceneRenderCamera;
	}
	// delete DisplayTexture; //Displays a texture to the screen
	delete SkyboxTex;
	delete SkyboxTwo;
	theScene->deregisterMesh(&BadDeer);
	// myFont->deregisterFontFromScene(theScene);
	delete theScene;
	if (myFont)
		delete myFont;

	std::cout << "\n DELETING MESHES...";
	while (ProgramMeshInstances.size() > 0) {
		delete ProgramMeshInstances[ProgramMeshInstances.size() - 1]; // Note: I didn't check for null here.
																	  // Bad practice?
		ProgramMeshInstances.erase(ProgramMeshInstances.end() - 1);
	}
	if (PointLightsWithoutShadows.size() > 0)
		for (size_t i = 0; i < PointLightsWithoutShadows.size(); i++) {
			if (PointLightsWithoutShadows[i]) // Returns true if it's not null.
											  // if it's not null, we want to
											  // delete it.
				delete PointLightsWithoutShadows[i];
		}
	if (Amb_Lights.size() > 0)
		for (size_t i = 0; i < Amb_Lights.size(); i++) {
			if (Amb_Lights[i])
				delete Amb_Lights[i];
		}
	if (Cam_Lights.size() > 0) // TODO: Push this to 147
		for (size_t i = 0; i < Cam_Lights.size(); i++) {
			if (Cam_Lights[i])
				delete Cam_Lights[i];
		}
	std::cout << "\n\n DELETED ALL LIGHTS!";

	if (FBOArray.size() > 0) {
		for (size_t i = 0; i < FBOArray.size(); i++) {
			std::cout << "\n\n DELETING FBO " << i;
			if (FBOArray[i]) // Returns true if it's not null. if it's not null,
							 // we want to delete it.
				delete FBOArray[i];
		}
	}
	//~ if(OpenALDevice)
	//~ alcCloseDevice(OpenALDevice);
	std::cout << "\n\n DELETED THE FBOS";
	myDevice->removeWindow(0);
	std::cout << "\n DELETED THE WINDOW";
	delete myDevice;
	std::cout << "\n Peace!" << std::endl;
}
