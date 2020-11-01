#include "TracerDemo.h"
#include "gekrender.h"
#include <cmath>
#include <ctime>
using namespace gekRender; // For convenience

IODevice* myDevice = new IODevice();
GkCPURenderer* theScene = nullptr;
Shader* showTextureShader = nullptr;

// Variables used for creating the window
int WIDTH = 1000;
int HEIGHT = 1000;

// The CPU FrameBuffer can be a different resolution than the window
int CPUWIDTH = 500;
int CPUHEIGHT = 500;
std::string Title("CPU Rendering Demo");
uint numSpheres = 10;

// Demo for the Tracer
size_t FrameIncrementedVariable = 0;
TracerWorld theWorld;
float screenDistance = 0.6;
glm::vec3 eye = glm::vec3(0, 0, -screenDistance);
glm::vec3 eyedir = glm::vec3(0, 0, 1);   // PRE-NORMALIZD
glm::vec3** PreComputedPoints = nullptr; // Pre-computed ray directions
std::vector<TraceableShape*> TraceableShapes;

void init() {
	srand(time(NULL));
	// Bare Minimum to open a window...
	myDevice->initGLFW();
	myDevice->pushWindowCreationHint(GLFW_RESIZABLE,
									 GLFW_FALSE);	  // The next window will be created with the hints we push
													   // on! In this case, we do not want the window to be
													   // resizeable, so we make it un-resizeable
	myDevice->addWindow(WIDTH, HEIGHT, Title.c_str()); // Create the window!
	//~ myDevice->setContext(0); //Binds GL context of window 0 to current
	// thread
	myDevice->initGL3W();	// Initialize the OpenGL extension wrangler.
	myDevice->setContext(0); // Binds GL context of window 0 to current thread

	theScene = new GkCPURenderer(WIDTH, HEIGHT, CPUWIDTH, CPUHEIGHT);

	// Shows Texture to Screen
	showTextureShader = new Shader("tracer_shaders/SHOWTEX");
}
void setup() {

	// Demo Tracer Code
	FrameIncrementedVariable = 0;
	// PreComputedPoints[CPUWIDTH][CPUHEIGHT];
	PreComputedPoints = (glm::vec3**)malloc(sizeof(glm::vec3*) * CPUWIDTH);
	for (uint i = 0; i < CPUWIDTH; i++)
		PreComputedPoints[i] = (glm::vec3*)malloc(sizeof(glm::vec3) * CPUHEIGHT);

	float widthum = (float)CPUWIDTH;
	float heightum = (float)CPUHEIGHT;
	for (int w_iter = 0; w_iter < CPUWIDTH; w_iter++)
		for (int h_iter = 0; h_iter < CPUHEIGHT; h_iter++) {

			PreComputedPoints[w_iter][h_iter] = glm::vec3((float)w_iter / widthum - 0.5, (float)h_iter / heightum - 0.5, 0);
			//~ if (h_iter == 3 && false)
			//~ std::cout << "PRE-COMPUTED POSITION:\nW: "<<w_iter<<"\nH:
			//"<<h_iter<<"\nis: \nX: " <<PreComputedPoints[w_iter][h_iter].x
			//<<"\nY:
			//"<<PreComputedPoints[w_iter][h_iter].y;
		}
	// Put some lights in the scene
	theWorld.TracerLights.push_back(TraceablePointLight());
	theWorld.TracerLights[0].position = glm::vec3(0, 1, 1.5);
	theWorld.TracerLights[0].color = glm::vec3(0.8, 0.8, 0.8);
	// Generate some sphere and place them some interesting places
	glm::vec3 pos1 = glm::vec3(-1, 0, 2);
	glm::vec3 pos2 = glm::vec3(1, 0, 2);
	glm::vec3 pos3 = glm::vec3(0, 1.5, 3);
	glm::vec3 color1 = glm::vec3(1, 0, 0);
	glm::vec3 color2 = glm::vec3(0, 1, 0);
	glm::vec3 color3 = glm::vec3(0.5, 0, 0.5);
	TraceableShapes.push_back(new Sphere(pos1, color1, 1));
	TraceableShapes[TraceableShapes.size() - 1]->reflectivity = 0.5;
	theWorld.TracerObjects.push_back(TraceableShapes[TraceableShapes.size() - 1]);

	TraceableShapes.push_back(new Sphere(pos2, color2, 1));
	TraceableShapes[TraceableShapes.size() - 1]->reflectivity = 0.9;
	theWorld.TracerObjects.push_back(TraceableShapes[TraceableShapes.size() - 1]);

	TraceableShapes.push_back(new Sphere(pos3, color3, 0.3));
	TraceableShapes[TraceableShapes.size() - 1]->reflectivity = 0.4;
	TraceableShapes[TraceableShapes.size() - 1]->specularDampening = 30;
	theWorld.TracerObjects.push_back(TraceableShapes[TraceableShapes.size() - 1]);
	// Generate some spheres and place them some places
	//~ if (true)
	for (int i = 0; i < numSpheres; i++) {
		glm::vec3 pos = glm::vec3(((float)(rand() % 1000) - 500.0f) / 500.0f * 10.0f, ((float)(rand() % 1000) - 500.0f) / 500.0f * 10.0f,
								  ((float)(rand() % 1000) - 500.0f) / 500.0f * 10.0f + 10.0f);
		glm::vec3 color = glm::vec3(((float)(rand() % 1000)) / 1000.0f, ((float)(rand() % 1000)) / 1000.0f, ((float)(rand() % 1000)) / 1000.0f);
		TraceableShapes.push_back(new Sphere(pos, color, 0.1f + ((float)(rand() % 1000)) / 1000.0f));
		theWorld.TracerObjects.push_back(TraceableShapes[TraceableShapes.size() - 1]);
	}
}

void DrawScene() {
	for (int w_iter = 0; w_iter < CPUWIDTH; w_iter++)
		for (int h_iter = 0; h_iter < CPUHEIGHT; h_iter++) {
			glm::vec3 RayDirection;
			RayDirection.x = PreComputedPoints[w_iter][h_iter].x - eye.x;
			RayDirection.y = PreComputedPoints[w_iter][h_iter].y - eye.y;
			RayDirection.z = PreComputedPoints[w_iter][h_iter].z - eye.z;
			glm::vec3 pixColor = theWorld.TraceRay(glm::normalize(RayDirection), eye, 3, 300.0f);
			int red = (int)(pixColor.x * 255.0f) % 256;
			int green = (int)(pixColor.y * 255.0f) % 256;
			int blue = (int)(pixColor.z * 255.0f) % 256;
			theScene->WritePixel(w_iter, h_iter, (unsigned char)(red), (unsigned char)(green), (unsigned char)(blue));
		}
}
void DrawToScreen() { theScene->draw(showTextureShader); }

void cleanUp() {
	for (auto* item : TraceableShapes)
		delete item;
	for (uint i = 0; i < CPUWIDTH; i++)
		delete PreComputedPoints[i];
	delete PreComputedPoints;
	TraceableShapes.clear();
}

std::string ver_string(int a, int b, int c) {
	std::string retval = "";
	retval = std::to_string(a) + '.' + std::to_string(b) + '.' + std::to_string(c);
	return retval;
}

int main(int argc, char** argv) {
	std::string true_cxx = "g++"; // a pretty valid assumption
	// don't you dare use MSVC to compile my baby
	// clang might be OK tho... but I won't officially support it.

	std::string true_cxx_ver = ver_string(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);

	std::string la; // last argument
	// How to parse command line arguments: simple tutorial
	for (int i = 1; i < argc; i++) {
		std::string a(argv[i]); // current argument
		if (a.find("--help") != std::string::npos || a.find("-help") != std::string::npos) {
			std::cout << "\nUsage: \n"
					  << argv[0]
					  << " --spheres <int> --cpuwidth <int> --cpuheight <int> "
						 "--width "
						 "<int> --height <int> --title <string, escape spaces please>"
					  << std::endl;
			std::cout << "\nYou may use one, all, or none of these args." << std::endl;
			std::cout << "\nthe args --help and -help produce this dialog." << std::endl;
			std::cout << "\nYou can use -v or --version to get a version printout." << std::endl;
			return 0;
		}
		if (a.find("--version") != std::string::npos || a.find("-v") != std::string::npos) { // YES I COULD JUST TEST FOR -V BUT
																							 // IT'S THE PRINCIPLE OF THE THING
			std::cout << "\nVersion: geks-Simple-Game-Engine/THEVERSION, "
						 "Compiler Version: "
					  << true_cxx << " " << true_cxx_ver << std::endl;
			return 0;
		}
		if (la.find("--spheres") != std::string::npos)
			numSpheres = GkAtoi(a.c_str());
		if (la.find("--cpuwidth") != std::string::npos)
			CPUWIDTH = GkAtoi(a.c_str());
		if (la.find("--cpuheight") != std::string::npos)
			CPUHEIGHT = GkAtoi(a.c_str());
		if (la.find("--width") != std::string::npos)
			WIDTH = GkAtoi(a.c_str());
		if (la.find("--height") != std::string::npos)
			HEIGHT = GkAtoi(a.c_str());
		if (la.find("--title") != std::string::npos)
			Title = a;
		la = a;
	}
	init();
	setup();
	while (!myDevice->shouldClose(0)) {
		myDevice->pollevents();
		myDevice->swapBuffers(0);
		DrawScene();
		DrawToScreen();
	}
	cleanUp();
	delete myDevice;
	return 0;
}
