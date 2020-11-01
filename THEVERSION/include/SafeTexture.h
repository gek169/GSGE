#ifndef SAFETEXTURE_H
#define SAFETEXTURE_H

#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#include "texture.h"
#include <cassert>
#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

// This class is kind of obvious... All it needs to know about itself is the
// GLuint handle of the texture for binding... this is a stupid class
namespace gekRender {
class SafeTexture {
  public:
	SafeTexture() { // Default values
		m_texture = 0;
		isnull = true;
		isTransparent = false;
		DaddyO = nullptr;
		TextureUpdates = false;
	}
	SafeTexture(Texture* inTex);
	SafeTexture(GLuint texture_object);
	void bind(unsigned int unit);
	void bindGeneric();
	inline bool amINull() const { return isnull; }
	inline bool amITransparent() const { return isTransparent; }
	static void setActiveUnit(unsigned int unit);
	~SafeTexture();
	SafeTexture(const SafeTexture& other);
	bool operator==(const SafeTexture& other);
	GLuint getHandle() const { return m_texture; }
	bool TextureUpdates = false;
	Texture* DaddyO = nullptr;

  protected:
  private:
	bool isTransparent = false; // transparency flag. We will later be setting this!
	bool isnull = true;			// To keep track of whether or not the texture pointer is null
	GLuint m_texture = 0;		// Initialize Everything!
};
}; // namespace gekRender
#endif
