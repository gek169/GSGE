//#include "gekRender.h"
#include "FBO.h"
namespace gekRender {

// bool FBO::get_was_instantiated_with_Depth_Buffer() const {return
// was_instantiated_with_Depth_Buffer;} unsigned int FBO::getWidth() const
// {return width;} unsigned int FBO::getHeight() const {return height;} GLenum
// FBO::getmyInternalDataType() const {return myInternalDataType;} unsigned int
// getNumColorAttachments() const {return numcolorattachments;}
FBO::FBO(const FBO& Other) {
	if (Other.get_was_instantiated_with_Depth_Buffer())
		FBO(Other.getWidth(), Other.getHeight(), Other.getNumColorAttachments(), Other.getmyInternalDataType(), Other.getDepthBufferHandle());
	else
		FBO(Other.getWidth(), Other.getHeight(), Other.getNumColorAttachments(), Other.getmyInternalDataType());
}
FBO::FBO(int width, int height, unsigned int howmanycolorattachments, GLenum InternalDataType) {
	if (m_RenderTex) {
		// std::cerr << "\n ERROR! It is INVALID to call the constructor of an
		// already-instantiated OpenGL Object!\n It is the convention of this
		// library for assets and opengl objects to be non-reinstantiatable." <<
		// "\n\n at the current time that includes these classes:" <<
		// "\n Mesh" <<
		// "\n FBO" <<
		// "\n Texture" <<
		// "\n Shader" <<
		// "\n Cubemap";
		// std::abort();
		// int* p = nullptr;
		// *p = 0;
		glDeleteTextures(numcolorattachments, &m_RenderTex[0]);
		glDeleteFramebuffers(1, &m_FBO);
		if (!was_instantiated_with_Depth_Buffer)
			glDeleteRenderbuffers(1, &m_DepthBuffer);
		m_DepthBuffer = 0;
		was_instantiated_with_Depth_Buffer = false; // Default Value
		if (m_RenderTex != nullptr)
			free(m_RenderTex);
		m_RenderTex = nullptr;
	}
	myInternalDataType = InternalDataType;
	was_instantiated_with_Depth_Buffer = false;
	this->width = width;
	this->height = height;
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	// SAFETY CATCH! Limiting how many color attachments you can have.
	if (howmanycolorattachments < MAX_COLOR_ATTACHMENTS && howmanycolorattachments > 0) {
		numcolorattachments = howmanycolorattachments; // How many color attachments will we
													   // allocate?
	} else if (howmanycolorattachments > MAX_COLOR_ATTACHMENTS) {
		numcolorattachments = MAX_COLOR_ATTACHMENTS;
	} else {
		numcolorattachments = 1; // 0 is invalid
	}
	if (m_RenderTex != nullptr)
		free(m_RenderTex);
	m_RenderTex = (GLuint*)malloc(sizeof(GLuint) * numcolorattachments);
	glGenTextures(numcolorattachments, &m_RenderTex[0]);
	// Allocate the color attachments
	for (unsigned int i = 0; i < numcolorattachments; i++) {
		// "Bind" the newly created texture : all future texture functions will
		// modify this texture
		glBindTexture(GL_TEXTURE_2D, m_RenderTex[i]);

		glTexImage2D(GL_TEXTURE_2D, 0, InternalDataType, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
					 0); // Create the GPU memory space allocation

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	glGenRenderbuffers(1, &m_DepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

	// Bind the texture targets to the FBO object
	for (unsigned int i = 0; i < numcolorattachments; i++) {
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_RenderTex[i], 0);
		DrawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	glDrawBuffers(numcolorattachments, DrawBuffers);

	GLenum communism = glGetError();
	if (communism != GL_NO_ERROR) {
		std::cout << "\n OpenGL reports an ERROR while creating FBOs.";
		if (communism == GL_INVALID_ENUM)
			std::cout << "\n Invalid enum.";
		if (communism == GL_INVALID_OPERATION)
			std::cout << "\n Invalid operation.";
		if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
			std::cout << "\n Invalid Framebuffer Operation.";
		if (communism == GL_OUT_OF_MEMORY) {
			std::cout << "\n Out of memory. You've really messed up. How could you "
						 "do this?!?!";
			std::abort();
		}
	}
}
FBO::FBO(int width, int height, unsigned int howmanycolorattachments, GLenum InternalDataType, GLuint mylittledepthbuffer) {
	if (m_RenderTex) {
		// std::cerr << "\n ERROR! It is INVALID to call the constructor of an
		// already-instantiated OpenGL Object!\n It is the convention of this
		// library for assets and opengl objects to be non-reinstantiatable." <<
		// "\n\n at the current time that includes these classes:" <<
		// "\n Mesh" <<
		// "\n FBO" <<
		// "\n Texture" <<
		// "\n Shader" <<
		// "\n Cubemap";
		// std::abort();
		// int* p = nullptr;
		// *p = 0;
		glDeleteTextures(numcolorattachments, &m_RenderTex[0]);
		glDeleteFramebuffers(1, &m_FBO);
		if (!was_instantiated_with_Depth_Buffer)
			glDeleteRenderbuffers(1, &m_DepthBuffer);
		m_DepthBuffer = 0;
		was_instantiated_with_Depth_Buffer = false; // Default Value
		if (m_RenderTex != nullptr)
			free(m_RenderTex);
		m_RenderTex = nullptr;
	}
	myInternalDataType = InternalDataType;
	was_instantiated_with_Depth_Buffer = true;
	this->width = width;
	this->height = height;
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	// Limiting you to MAX_COLOR_ATTACHMENTS.
	if (howmanycolorattachments < MAX_COLOR_ATTACHMENTS && howmanycolorattachments > 0) {
		numcolorattachments = howmanycolorattachments; // How many color attachments will we
													   // allocate?
	} else {
		numcolorattachments = 1;
	}

	if (m_RenderTex != nullptr)
		free(m_RenderTex);
	m_RenderTex = (GLuint*)malloc(sizeof(GLuint) * numcolorattachments); // NEEDS TO BE MODIFIED FOR MULTIPLE!
	// generate the color attachments
	glGenTextures(numcolorattachments, &m_RenderTex[0]); // NEEDS TO BE FOR'D
	// Allocate the color attachments
	for (unsigned int i = 0; i < numcolorattachments; i++) {
		// "Bind" the newly created texture : all future texture functions will
		// modify this texture
		glBindTexture(GL_TEXTURE_2D, m_RenderTex[i]);

		glTexImage2D(GL_TEXTURE_2D, 0, InternalDataType, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	// glGenRenderbuffers(1, &m_DepthBuffer);
	m_DepthBuffer = mylittledepthbuffer;
	glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
	// glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width,
	// height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

	for (unsigned int i = 0; i < numcolorattachments; i++) {
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_RenderTex[i], 0);
		DrawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	// Set the list of draw buffers.
	// = {GL_COLOR_ATTACHMENT0}//NEEDS TO BE FOR'D
	glDrawBuffers(numcolorattachments,
				  DrawBuffers); // "1" is the size of DrawBuffers

	// Communism was a mistake. get it?
	GLenum communism = glGetError();
	if (communism != GL_NO_ERROR) {
		std::cout << "\n OpenGL reports an ERROR!";
		if (communism == GL_INVALID_ENUM)
			std::cout << "\n Invalid enum.";
		if (communism == GL_INVALID_OPERATION)
			std::cout << "\n Invalid operation.";
		if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
			std::cout << "\n Invalid Framebuffer Operation.";
		if (communism == GL_OUT_OF_MEMORY) {
			std::cout << "\n Out of memory. You've really messed up. How could you "
						 "do this?!?!";
			std::abort();
		}
	}
}

// GLuint FBO::getHandle() {return m_FBO;} //This was added so that
// shader.update could be removed
SafeTexture FBO::getTex(int attachment) {
	return SafeTexture(m_RenderTex[attachment]);
	;
}
GLuint FBO::getDepthBufferHandle() const { // Gonna use this in the transparency pass.
	return m_DepthBuffer;
}
void FBO::setActiveTexUnit(unsigned int unit) {
	assert(unit >= 0 && unit <= 31);
	glActiveTexture(GL_TEXTURE0 + unit);
}
void FBO::unBindRenderTarget(unsigned int screenwidth, unsigned int screenheight) {
	static GLenum GAYDrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, GAYDrawBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screenwidth, screenheight);
} // Restore defaults.
void FBO::clearTexture(float r, float g, float b, float a) {
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
unsigned int FBO::getWidth() const { return width; }
unsigned int FBO::getHeight() const { return height; }
unsigned int FBO::getNumColorAttachments() const { return numcolorattachments; }
GLenum FBO::getmyInternalDataType() const { return myInternalDataType; }
bool FBO::get_was_instantiated_with_Depth_Buffer() const { return was_instantiated_with_Depth_Buffer; }
GLuint FBO::getHandle() { return m_FBO; }

FBO::~FBO() {
	glDeleteTextures(numcolorattachments, &m_RenderTex[0]);
	glDeleteFramebuffers(1, &m_FBO);
	if (!was_instantiated_with_Depth_Buffer)
		glDeleteRenderbuffers(1, &m_DepthBuffer);
	m_DepthBuffer = 0;
	was_instantiated_with_Depth_Buffer = false; // Default Value
	if (m_RenderTex != nullptr)
		free(m_RenderTex);
	m_RenderTex = nullptr;
}

GLuint FBO::getColorAttachmentTextureHandle(unsigned int whichone) {
	if (whichone < numcolorattachments)
		return m_RenderTex[whichone];
	else
		return 0;
}

void FBO::bindDrawBuffers() {
	glDrawBuffers(numcolorattachments,
				  DrawBuffers); // it broke things, and it ain't pretty;
}
void FBO::bindRenderTarget() {
	if (numcolorattachments < 1)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glDrawBuffers(numcolorattachments,
				  DrawBuffers); // it broke things, and it ain't pretty
	glViewport(0, 0, width,
			   height); // The thing about screenquads probably isn't true, and
						// even if it is, we can adjust for it.
}
void FBO::bindasTextureGeneric(unsigned int whichone) {
	if (whichone >= numcolorattachments)
		return;
	glBindTexture(GL_TEXTURE_2D, m_RenderTex[whichone]);
}
void FBO::bindasTexture(unsigned int unit, unsigned int whichone) {
	if (whichone >= numcolorattachments)
		return;
	assert(unit >= 0 && unit <= 31);
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, m_RenderTex[whichone]);
}
GLuint FBO::getHandle(unsigned int whichone) // Used for deregistering the safetextures
											 // from the meshes. Hotfix for 106 release
{
	return m_RenderTex[whichone];
}
}; // namespace gekRender
