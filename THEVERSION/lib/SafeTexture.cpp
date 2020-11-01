#include "SafeTexture.h"
#include <cassert>
#include <iostream>
namespace gekRender {
SafeTexture::SafeTexture(GLuint texture_object) {
	m_texture = texture_object;
	DaddyO = nullptr;
	TextureUpdates = false;
	if (texture_object)
		isnull = false;
	else
		isnull = true;
}

SafeTexture::SafeTexture(Texture* inTex) // What texture should this be a copy of?
{
	if (inTex) {
		m_texture = inTex->getHandle();
		isTransparent = inTex->amITransparent();
		TextureUpdates = true;
		DaddyO = inTex;
		isnull = false;
	} else {
		isnull = true;
	}
}

void SafeTexture::bind(unsigned int unit) {
	if (DaddyO && TextureUpdates)
		m_texture = DaddyO->getHandle();
	assert(unit >= 0 && unit <= 31);
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, m_texture);
}
SafeTexture::~SafeTexture() {
	// What needs to be destroyed? Nothing! NOTHING NEEDS TO BE DESTROYED!
	DaddyO = nullptr;
	TextureUpdates = false;
};
SafeTexture::SafeTexture(const SafeTexture& other) {
	isTransparent = other.amITransparent();
	m_texture = other.getHandle();
	isnull = other.amINull();
	TextureUpdates = other.TextureUpdates;
	DaddyO = other.DaddyO;
}
bool SafeTexture::operator==(const SafeTexture& other) { return (other.getHandle() == getHandle()); }
void SafeTexture::bindGeneric() {
	if (DaddyO && TextureUpdates)
		m_texture = DaddyO->getHandle();
	glBindTexture(GL_TEXTURE_2D, m_texture);
}
void SafeTexture::setActiveUnit(unsigned int unit) {
	assert(unit >= 0 && unit <= 31);
	glActiveTexture(GL_TEXTURE0 + unit);
}
}; // namespace gekRender
