/*
(C) DMHSW 2018
GkScene Demo Program 2 main2.cpp
*/

// using namespace gekRender;

#include "gekrender.h"
// My AL Utils
#include "GekAL.h"
unsigned int WIDTH = 640;
unsigned int HEIGHT = 480;

// Global Variables
gekRender::IODevice* myDevice = new gekRender::IODevice();
gekRender::GkScene* theScene; // Pointer to the scene

// Keyboard Related
int oldkeystates[51];	 // nuff said
double oldmousexy[2];	 // Old mouse position
double currentmousexy[2]; // Why query the current mouse position multiple
						  // times?

// Rendering Related
gekRender::Shader* MainShad = nullptr;		  // Final Pass Shader
gekRender::Shader* SkyboxShad = nullptr;	  // Skybox shader
gekRender::Shader* DisplayTexture = nullptr;  // Displays a texture to the screen
gekRender::Shader* WBOITCompShader = nullptr; // Composites the WBOIT initial pass onto the opaque framebuffer

gekRender::Camera* SceneRenderCamera = nullptr; // SceneRender camera

// OUR OBJECTS
ALuint audiosource1 = 0;
ALuint audiobuffer1 = 0;
gekRender::Mesh* CubeMesh = nullptr;
gekRender::Mesh* SphereMesh = nullptr;
gekRender::CubeMap* SkyboxTex = nullptr;
gekRender::MeshInstance myCube = gekRender::MeshInstance();
gekRender::MeshInstance mySphere = gekRender::MeshInstance();
gekRender::Texture* CloudsJpeg = nullptr;
gekRender::DirectionalLight theSun = gekRender::DirectionalLight();
gekRender::PointLight thePoint = gekRender::PointLight(glm::vec3(10, 20, 10), glm::vec3(1, 1, 1));
gekRender::Texture* AmigaPng = nullptr;

// Controls the spin of the cube
float ordinarycounter = 0.0f;

// Callbacks
void window_size_callback(GLFWwindow* window, int width, int height) {
	using namespace gekRender;
	if (height == 0 || width == 0)
		return;
	if (theScene)
		theScene->resizeSceneViewport(width, height, 1);
	WIDTH = width;
	HEIGHT = height;
	if (SceneRenderCamera != nullptr)
		SceneRenderCamera->buildPerspective(70, ((float)WIDTH) / ((float)HEIGHT), 1, 1000);
	std::cout << "\nNEW SIZE: " << WIDTH << " x " << HEIGHT;
}

void init() {
	// Why didn't I just put this at the top of the file???
	using namespace gekRender;
	// Creates the GLFW context. This pretty much has to be the first code we
	// run
	myDevice->initGLFW();
	myDevice->pushWindowCreationHint(GLFW_RESIZABLE, GLFW_TRUE);
	// Set Version to OpenGL 3.3
	myDevice->pushWindowCreationHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	myDevice->pushWindowCreationHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	myDevice->addWindow(WIDTH, HEIGHT, "GkScene API on gekRender- Demo Program 2");
	// myDevice->addFullScreenWindow(WIDTH, HEIGHT, "GkScene API- Demo Program
	// 2");

	myDevice->setWindowSizeCallback(0,
									window_size_callback); // Self explanatory
	myDevice->setContext(0);							   // Binds GL context of window 0 to current thread
	myDevice->initGL3W();								   // Initialize the OpenGL extension wrangler.
	// myDevice->setKeyCallBack(0, key_callback); //Set a callback function.

	glEnable(GL_CULL_FACE);  // Enable culling faces
	glEnable(GL_DEPTH_TEST); // test fragment depth when rendering
	glDepthMask(GL_TRUE);
	glCullFace(GL_BACK); // cull faces with clockwise winding

	// Standard error check code
	// GLenum communism = glGetError();
	// if (communism != GL_NO_ERROR)
	// {
	// std::cout<<"\n OpenGL reports an ERROR!";
	// if (communism == GL_INVALID_ENUM)
	// std::cout<<"\n Invalid enum.";
	// if (communism == GL_INVALID_OPERATION)
	// std::cout<<"\n Invalid operation.";
	// if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
	// std::cout <<"\n Invalid Framebuffer Operation.";
	// if (communism == GL_OUT_OF_MEMORY)
	// {
	// std::cout <<"\n Out of memory. You've really messed up. How could you do
	// this?!?!"; std::abort();
	// }
	// }
	// User Input Variables being set to default. Hacky but works.
	for (int i = 0; i < 51; i++)
		oldkeystates[i] = 0;
	for (int i = 0; i < 2; i++)
		oldmousexy[i] = 0;

	for (int i = 0; i < 2; i++)
		currentmousexy[i] = 0;
	theScene = new GkScene(WIDTH, HEIGHT, 1);

	// OpenAL Stuff
	//~ OpenALDevice = alcOpenDevice(NULL);
	//~ if (OpenALDevice)
	//~ {
	//~ std::cout << "\nUsing Device: " << alcgetString(OpenALDevice,
	// ALC_DEVICE_SPECIFIER) << "\n"; ~ OpenALContext =
	// alcCreateContext(OpenALDevice, 0); ~
	// if(alcMakeContextCurrent(OpenALContext)) ~ { ~ std::cout<<"\nSuccessfully
	// Made Context!!!";
	//~ }
	//~ }
	//~ algetError();
	myDevice->fastInitOpenAL();
}

void loadResources() {
	using namespace gekRender;
	// it puts .vs and .fs after the string. The EXE Is the starting folder, but
	// I could probably change that if I tried.
	MainShad = new Shader("shaders/FORWARD_MAINSHADER_UBO");
	DisplayTexture = new Shader("shaders/SHOWTEX");
	SkyboxShad = new Shader("shaders/Skybox");
	WBOITCompShader = new Shader("shaders/WBOIT_COMPOSITION_SHADER");
	theScene->setSkyboxShader(SkyboxShad);
	theScene->setMainShader(MainShad);
	theScene->ShowTextureShader = DisplayTexture; // Add a setter later
	// theScene->setWBOITCompositionShader(WBOITCompShader); //Has a setter, and
	// it's been a long time, I should write a setter for the ShowTextureShader

	// Load the cubemap we're going to use
	std::string cubemapfilenames[6] = {
		"Cubemap/Skybox_Water10_128_right.jpg", // right
		"Cubemap/Skybox_Water10_128_left.jpg",  // left
		"Cubemap/Skybox_Water10_128_top.jpg",   // up
		"Cubemap/Skybox_Water10_128_base.jpg",  // down
		"Cubemap/Skybox_Water10_128_back.jpg",  // back
		"Cubemap/Skybox_Water10_128_front.jpg"  // front
	};

	// Load the textures for the cube
	SkyboxTex = new CubeMap(cubemapfilenames[0], cubemapfilenames[1], cubemapfilenames[2], cubemapfilenames[3], cubemapfilenames[4], cubemapfilenames[5]);
	// And set the skybox
	theScene->setSkyBoxCubemap(SkyboxTex);
	CloudsJpeg = new Texture("CLOUDS.JPG", false);
	// Load the cube mesh and install the right textures into it
	CubeMesh = new Mesh("Cube_Test_Low_Poly.obj", false, false,
						true);						// Instanced, Static, Asset
	CubeMesh->pushTexture(SafeTexture(CloudsJpeg)); // 0
	CubeMesh->pushCubeMap(SkyboxTex);				// 0
	theScene->registerMesh(CubeMesh);
	// loadWAVintoALBuffer(const char* fn);
}

void initGame() {
	using namespace gekRender;
	SceneRenderCamera = new Camera(glm::vec3(0, 1, -10),		 // World Pos
								   70.0f,						 // FOV
								   (float)WIDTH / HEIGHT,		 // Aspect
								   0.01f,						 // Znear
								   1000.0f,						 // Zfar
								   glm::vec3(0.0f, 0.0f, 1.0f),  // forward
								   glm::vec3(0.0f, 1.0f, 0.0f)); // Up
	theScene->setSceneCamera(SceneRenderCamera);
	// Load lights and register them
	theScene->registerPointLight(&thePoint);
	theScene->registerDirLight(&theSun);

	// Set up our cube instance
	myCube.tex = 0;
	myCube.EnableCubemapReflections = 1;
	myCube.myPhong = Phong_Material(0.1, 0.9, 0.5, 25, 0.0);
	myCube.myTransform = Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
	CubeMesh->registerInstance(&myCube);
}

void checkKeys() {}

void everyFrame() {
	using namespace gekRender;
	// The counter which controls the program
	ordinarycounter += 0.1f;
	if (ordinarycounter > 5000.0f) {
		ordinarycounter = 0.0f;
	}

	// Spin the cube
	myCube.myTransform.setRot(glm::vec3(sinf(ordinarycounter / 10.0) * 5, sinf(ordinarycounter / 10.0) * 3, sinf(ordinarycounter / 11.2) * 2));
	myCube.myPhong = Phong_Material(0.1, 0.9, (sinf(ordinarycounter / 100) + 1) / 2.0f, 25, 0.0);
}

void Draw() {
	theScene->drawPipeline(1, nullptr, nullptr, nullptr, false, glm::vec4(0, 0, 0.1, 0), glm::vec2(800, 1000));
	myDevice->pollevents();
	myDevice->swapBuffers(0);
}

void cleanUp() {
	if (MainShad)
		delete MainShad;
	if (DisplayTexture)
		delete DisplayTexture;
	if (SkyboxShad)
		delete SkyboxShad;
	if (WBOITCompShader)
		delete WBOITCompShader;
	if (SceneRenderCamera)
		delete SceneRenderCamera;

	if (SkyboxTex)
		delete SkyboxTex;

	if (CubeMesh)
		delete CubeMesh;
	if (SphereMesh)
		delete SphereMesh;

	if (CloudsJpeg)
		delete CloudsJpeg;
	if (AmigaPng)
		delete AmigaPng;

	if (theScene)
		delete theScene;
	if (myDevice)
		delete myDevice;
}

int main() {
	init();
	loadResources();
	initGame();
	while (!myDevice->shouldClose(0)) // Main game loop.
	{
		checkKeys();
		everyFrame();
		Draw();
	}

	cleanUp();
	return 0;
}
