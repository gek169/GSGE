#include "GLRenderObject.h"

namespace gekRender {

void GLShader::buildShader(std::vector<std::string>& ShaderTexts, std::vector<GLuint>& ShaderTypes) {
	if (m_program != 0)
		destroy();
	m_program = glCreateProgram();
	for (size_t i = 0; i < ShaderTexts.size() && i < ShaderTypes.size(); i++) {
		m_shaders.push_back(createShader(ShaderTexts[i], ShaderTypes[i]));
		glAttachShader(m_program, m_shaders[i]);
	}
	glLinkProgram(m_program);
	checkShaderError(m_program, GL_LINK_STATUS, true, "Error linking shader program");

	glValidateProgram(m_program);
	checkShaderError(m_program, GL_VALIDATE_STATUS, true, "Invalid shader program");
}
void GLShader::destroy() {								// Very important that this is NOT a destructor
	for (unsigned int i = 0; i < m_shaders.size(); i++) // Doesn't need to be size_t
	{
		glDetachShader(m_program, m_shaders[i]);
		glDeleteShader(m_shaders[i]);
	}
	glDeleteProgram(m_program);
	m_program = 0;
	m_shaders.clear();
}
void GLShader::bind() { glUseProgram(m_program); }
GLint GLShader::getUniformLocation(std::string name) { return glGetUniformLocation(m_program, name.c_str()); }
void GLShader::checkShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage) {
	GLint success = 0;
	GLchar error[1024] = {0};

	if (isProgram)
		glGetProgramiv(shader, flag, &success);
	else
		glGetShaderiv(shader, flag, &success);

	if (success == GL_FALSE) {
		if (isProgram)
			glGetProgramInfoLog(shader, sizeof(error), NULL, error);
		else
			glGetShaderInfoLog(shader, sizeof(error), NULL, error);

		std::cerr << errorMessage << ": '" << error << "'" << std::endl;
	}
}
GLuint GLShader::createShader(const std::string& text, unsigned int type) {
	GLuint shader = glCreateShader(type);

	if (shader == 0)
		std::cerr << "Error compiling shader type " << type << std::endl;

	const GLchar* p[1];
	GLint lengths[1];

	p[0] = text.c_str();
	lengths[0] = text.length();

	glShaderSource(shader, 1, p, lengths);
	glCompileShader(shader);

	checkShaderError(shader, GL_COMPILE_STATUS, false, "Error compiling shader!");

	return shader;
}

void GLRenderObject::destruct() { // put a call to this in your overriding
								  // destructors
	if (GLBuffers.size() > 0) {
		glDeleteBuffers(GLBuffers.size(), &(GLBuffers[0]));
		GLBuffers.clear();
	}
	if (GLRenderBuffers.size() > 0) {
		glDeleteRenderbuffers(GLRenderBuffers.size(), &(GLRenderBuffers[0]));
		GLRenderBuffers.clear();
	}
	if (GLTextures.size() > 0) {
		glDeleteTextures(GLTextures.size(), &(GLTextures[0]));
		GLTextures.clear();
	}
	if (GLQueries.size() > 0) {
		glDeleteQueries(GLQueries.size(), &(GLQueries[0]));
		GLQueries.clear();
	}
	if (GLSamplers.size() > 0) {
		glDeleteSamplers(GLSamplers.size(), &(GLSamplers[0]));
		GLSamplers.clear();
	}
	if (GLFrameBuffers.size() > 0) {
		glDeleteFramebuffers(GLFrameBuffers.size(), &(GLFrameBuffers[0]));
		GLFrameBuffers.clear();
	}
	if (GLVertexArrays.size() > 0) {
		glDeleteVertexArrays(GLVertexArrays.size(), &(GLVertexArrays[0]));
		GLVertexArrays.clear();
	}
	if (GLTransformFeedbacks.size() > 0) {
		glDeleteTransformFeedbacks(GLTransformFeedbacks.size(), &(GLTransformFeedbacks[0]));
		GLTransformFeedbacks.clear();
	}
	if (GLShaders.size() > 0) {
		for (size_t i = 0; i < GLShaders.size(); i++)
			GLShaders[i].destroy();
		GLShaders.clear();
	}
}

}; // namespace gekRender
