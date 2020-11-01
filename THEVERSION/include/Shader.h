#ifndef SHADER_H
#define SHADER_H

#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
// This is a derivative work of BennyBox's original Shader loading code.
/*
Copyright 2014 TheBennyBox

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

namespace gekRender {
// The CPP file is HORRENDOUS!
class Shader {
  public:
	Shader(const std::string& Filename);
	Shader(const std::string& VS, const std::string& FS);
	Shader() {
		std::cout << "\n YOU HAVE TO INITIALIZE SHADER THE RIGHT WAY";
		std::abort();
	} // Default.
	// Shader(const Shader& other){
	// std::cout << "\n YOU HAVE TO INITIALIZE SHADER THE RIGHT WAY";
	// std::abort();
	// }
	void bind();
	void bindAttribLocation(int x, const GLchar* y) { glBindAttribLocation(m_program, x, y); }
	GLint getUniformLocation(std::string name);
	GLint getUniformBlockIndex(std::string name) { return glGetUniformBlockIndex(m_program, name.c_str()); }
	void uniformBlockBinding(GLuint index, GLuint binding_point) {
		glUniformBlockBinding(m_program, index, binding_point);
	}											   // BINDING POINT is a number between 0 and GL_MAX_UNIFORM_BUFFERS
	GLuint getHandle() const { return m_program; } // In case it is necessary...

	void setUniform1f(std::string name, GLfloat value); // 1 float
	void setUniformMatrix4fv(std::string name, GLsizei count, GLboolean transpose,
							 GLfloat* dick);			// mat4
	void setUniform1i(std::string name, GLint value);   // Integer
	void setUniform1ui(std::string name, GLuint value); // Unsigned Integer
	/* List of all OpenGL gluniform commands:
			void glUniform1f(	GLint location,
					GLfloat v0);

					void glUniform2f(	GLint location,
							GLfloat v0,
							GLfloat v1);
					 
					void glUniform3f(	GLint location,
							GLfloat v0,
							GLfloat v1,
							GLfloat v2);
			 
					void glUniform4f(	GLint location,
							GLfloat v0,
							GLfloat v1,
							GLfloat v2,
							GLfloat v3);
			 
			void glUniform1i(	GLint location,
					GLint v0);
			 
			void glUniform2i(	GLint location,
					GLint v0,
					GLint v1);
			 
			void glUniform3i(	GLint location,
					GLint v0,
					GLint v1,
					GLint v2);
			 
			void glUniform4i(	GLint location,
					GLint v0,
					GLint v1,
					GLint v2,
					GLint v3);
			 
			void glUniform1ui(	GLint location,
					GLuint v0);
			 
			void glUniform2ui(	GLint location,
					GLuint v0,
					GLuint v1);
			 
			void glUniform3ui(	GLint location,
					GLuint v0,
					GLuint v1,
					GLuint v2);
			 
			void glUniform4ui(	GLint location,
					GLuint v0,
					GLuint v1,
					GLuint v2,
					GLuint v3);
			 
			void glUniform1fv(	GLint location,
					GLsizei count,
					const GLfloat *value);
			 
			void glUniform2fv(	GLint location,
					GLsizei count,
					const GLfloat *value);
			 
			void glUniform3fv(	GLint location,
					GLsizei count,
					const GLfloat *value);
			 
			void glUniform4fv(	GLint location,
					GLsizei count,
					const GLfloat *value);
			 
			void glUniform1iv(	GLint location,
					GLsizei count,
					const GLint *value);
			 
			void glUniform2iv(	GLint location,
					GLsizei count,
					const GLint *value);
			 
			void glUniform3iv(	GLint location,
					GLsizei count,
					const GLint *value);
			 
			void glUniform4iv(	GLint location,
					GLsizei count,
					const GLint *value);
			 
			void glUniform1uiv(	GLint location,
					GLsizei count,
					const GLuint *value);
			 
			void glUniform2uiv(	GLint location,
					GLsizei count,
					const GLuint *value);
			 
			void glUniform3uiv(	GLint location,
					GLsizei count,
					const GLuint *value);
			 
			void glUniform4uiv(	GLint location,
					GLsizei count,
					const GLuint *value);
			 
			void glUniformMatrix2fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
			 
			void glUniformMatrix3fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
			 
			void glUniformMatrix4fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
			 
			void glUniformMatrix2x3fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
			 
			void glUniformMatrix3x2fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
			 
			void glUniformMatrix2x4fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
			 
			void glUniformMatrix4x2fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
			 
			void glUniformMatrix3x4fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
			 
			void glUniformMatrix4x3fv(	GLint location,
					GLsizei count,
					GLboolean transpose,
					const GLfloat *value);
	*/

	virtual ~Shader();

  protected:
  private:
	Shader(const Shader& other) {}
	void operator=(const Shader& other) {}

	// GLuint CreateShader(const std::string& text, unsigned int type);
	// void CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const
	// std::string& errorMessage); std::string LoadShader(const std::string&
	// fileName);

	enum { VERT_SHADER, FRAG_SHADER, NUM_SHADERS };

	GLuint m_program = 0; // The ID of this program
	GLuint m_shaders[NUM_SHADERS];
	bool isnull = true;
};
}; // namespace gekRender
#endif
