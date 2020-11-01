#ifndef GK_GLRENDEROBJECT
#define GK_GLRENDEROBJECT

#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
//#include <list> //std::list
#include "transform.h"
#include <GLFW/glfw3.h>

namespace gekRender {

struct GLShader { // Can compile any kind of shader
	GLuint m_program = 0;
	std::vector<GLuint> m_shaders;

	// MEMBER FUNCTIONS
	void buildShader(std::vector<std::string>& ShaderTexts, std::vector<GLuint>& ShaderTypes);
	void destroy();
	void bind();
	GLint getUniformLocation(std::string name);
	// UTILITY FUNCTIONS
	void checkShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage);
	GLuint createShader(const std::string& text, unsigned int type);
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class GLRenderObject {
  public:
	GLRenderObject();
	virtual ~GLRenderObject() { destruct(); }
	void destruct(); // Put a call to this in your overriding destructor
	virtual void preDraw(int meshmask,
						 void* args); // IMPLEMENT THIS METHOD IN YOUR OWN INHERITING CLASS
	virtual void draw(int meshmask,
					  void* args); // IMPLEMENT THIS METHOD IN YOUR OWN INHERITING CLASS
	// TUTORIAL
	// Anything in these vectors will have their proper GL delete commands
	// called upon destruction, in order, from 0 to .size() This makes OpenGL
	// programming WAY easier
	std::vector<GLuint> GLBuffers;
	std::vector<GLuint> GLRenderBuffers;
	std::vector<GLuint> GLTextures;
	std::vector<GLuint> GLQueries;
	std::vector<GLuint> GLSamplers;
	std::vector<GLuint> GLFrameBuffers;
	std::vector<GLuint> GLVertexArrays;
	std::vector<GLuint> GLTransformFeedbacks;
	std::vector<GLShader> GLShaders;
	Transform myTransform;
};

}; // namespace gekRender
#endif

// Reference for OpenGL https://khronos.org/registry/OpenGL-Refpages/gl4/
// Best OpenGL Tutorial Series
// https://www.youtube.com/watch?v=ftiKrP3gW3k&list=PLEETnX-uPtBXT9T-hD0Bj31DSnwio-ywh
// Best OpenGL Book: OpenGL Programming Guide by Dave Shreiner (Some editions
// list more authors) http://www.opengl-redbook.com/
