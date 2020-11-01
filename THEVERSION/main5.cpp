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
															  // on! In this case, we do not want the window to be
															  // resizeable, so we make it un-resizeable
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
	myDevice->setContext(0);
	std::cout << "Hello World!" << std::endl;
	GLenum communism = glGetError();

	// Error Check code. Paste where you need it.

	// communism = glGetError(); //Ensure there are no errors listed before we
	// start. if (communism != GL_NO_ERROR) //if communism has made an error
	// (which is pretty typical)
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
	// std::cout <<"\n Out of memory. You've really done it now. I'm so angry,
	// i'm going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
	// std::abort();
	// }
	// }
	// Triangle Handles
	GLuint TriangleVAO = 0;
	GLuint TriangleVBOs[2] = {0, 0};
	// CPU-side Triangle Data
	GLfloat TrianglePositionData[12] = {-0.90f, -0.90f, 0.5, 1, // NOTE: Making W 0 is BAD JUJU to opengl for
																// some reason
										0.90f, -0.90f, 0.5, 1, -0.0f, 0.90f, 0.5, 1};
	GLfloat TriangleColorData[9] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f};
	// I'm using the gekRender Shader class for convenience. You can use
	// your own OpenGL Code to load the shader and compile it, but I prefer not
	// to.
	Shader* TriangleShader = new Shader("tri_shaders/triangles");
	TriangleShader->bind(); // Bind it to work with it...

	// Generate the Triangle's VAO- Typically (including in gekRender's Mesh
	// Class) a Vertex Array Object represents one "Object". VBOs are typically
	// tied to the one VAO as individual or multiple Vertex Attributes.
	glGenVertexArrays(1,		   // How many to generate
					  &TriangleVAO // Where to write the handle
	);							   // Go pick a spot in memory, give it a name, and put it in the VAOs
								   // array. Do that NumVAOs times.

	// Bind the VAO so we can work with it.
	glBindVertexArray(TriangleVAO);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	// Generate some VBOs to use
	glGenBuffers(2, TriangleVBOs);

	// Let's set up VBO 1: Positions
	glBindBuffer(GL_ARRAY_BUFFER, TriangleVBOs[0]);													   // GL_ARRAY_BUFFER is a type
	glBufferData(GL_ARRAY_BUFFER, sizeof(TrianglePositionData), TrianglePositionData, GL_STATIC_DRAW); // Data Upload
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,
						  0); // Tells the VAO how to get its 0'th vertex
							  // attribute out of this VBO.
	// Let's set up VBO 2: Colors
	glBindBuffer(GL_ARRAY_BUFFER, TriangleVBOs[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TriangleColorData), TriangleColorData,
				 GL_STATIC_DRAW); // Data Upload
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
						  0); // Tells the VAO how to get its 1st vertex
							  // attribute out of this VBO.

	communism = glGetError();	 // Ensure there are no errors listed before we
								  // start.
	if (communism != GL_NO_ERROR) // if communism has made an error (which is pretty typical)
	{
		std::cout << "\n OpenGL reports an ERROR!";
		if (communism == GL_INVALID_ENUM)
			std::cout << "\n Invalid enum.";
		if (communism == GL_INVALID_OPERATION)
			std::cout << "\n Invalid operation.";
		if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
			std::cout << "\n Invalid Framebuffer Operation.";
		if (communism == GL_OUT_OF_MEMORY) {
			std::cout << "\n Out of memory. You've really done it now. I'm so angry, "
						 "i'm "
						 "going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
			std::abort();
		}
	}

	// Always ALWAYS unbind the vertex array
	glBindVertexArray(0);
	// We don't actually NEED these but for 99.9% of cases you are going to use
	// them
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	while (!myDevice->shouldClose(0)) {
		// FBO::unBindRenderTarget(WIDTH,HEIGHT);
		FBO::clearTexture(0, 0, 0.2, 1);
		// glClearBufferfv(GL_COLOR, 0, black); // What color is the background?
		// Bind the Shader for the Triangle Render
		TriangleShader->bind();
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		// Draw the Triangle
		glBindVertexArray(TriangleVAO);   // Work with our vertex array again
		glDrawArrays(GL_TRIANGLES, 0, 3); // DRAW. 3 vertices.
		// ERROR CHECK
		{
			communism = glGetError();	 // Ensure there are no errors listed
										  // before we start.
			if (communism != GL_NO_ERROR) // if communism has made an error
										  // (which is pretty typical)
			{
				std::cout << "\n OpenGL reports an ERROR!";
				if (communism == GL_INVALID_ENUM)
					std::cout << "\n Invalid enum.";
				if (communism == GL_INVALID_OPERATION)
					std::cout << "\n Invalid operation.";
				if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
					std::cout << "\n Invalid Framebuffer Operation.";
				if (communism == GL_OUT_OF_MEMORY) {
					std::cout << "\n Out of memory. You've really done it now. "
								 "I'm so angry, "
								 "i'm going to close the program. ARE YOU "
								 "HAPPY NOW, DAVE?!?!";
					std::abort();
				}
			}
		}
		myDevice->pollevents();
		myDevice->swapBuffers(0);
	}

	delete TriangleShader;
	delete myDevice;
	return 0;
}
