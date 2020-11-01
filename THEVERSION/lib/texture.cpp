#include "texture.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>
// Testing this to see if we can move back to the header file
#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif
namespace gekRender {

/// grumble grumble
unsigned int uploadCubemap(std::vector<std::string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++) {
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 3);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		} else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
CubeMap::CubeMap(std::string& FIRST, std::string& SECOND, std::string& THIRD, std::string& FOURTH, std::string& FIFTH, std::string& SIXTH) {
	if (!isnull) {
		// Delete the texture, it's the only thing we have to think about.
		glDeleteTextures(1, &m_texture);
		MyName = "";
		isnull = true;
	}
	std::vector<std::string> faces;
	faces.push_back(FIRST);
	faces.push_back(SECOND);
	faces.push_back(THIRD);
	faces.push_back(FOURTH);
	faces.push_back(FIFTH);
	faces.push_back(SIXTH);
	for (auto& str : faces) {
		MyName += str;
		MyName += "|";
	}
	m_texture = uploadCubemap(faces);
	isnull = false;
}

unsigned char* Texture::stbi_load_passthrough(char* filename, int* width, int* height, int* numComponents, int something) {
	return stbi_load(filename, width, height, numComponents, something);
}
int Texture::stbi_write_png_passthrough(char const* filename, int w, int h, int comp, const void* data, int stride_in_bytes) {
	return stbi_write_png(filename, w, h, comp, data, stride_in_bytes);
}

void Texture::exportToPNGFile(std::string filename) {
	if (amINull())
		return;
	stbi_write_png_passthrough(filename.c_str(), getMyWidth(), getMyHeight(), getMyComponents(), Permanent_Data_Pointer, 0);
}

Texture::Texture(const std::string& fileName, bool _enableTransparency, GLenum minfilter, GLenum magfilter, GLenum wrap_mode, float anisotropic_level) {
	unsigned char* temp_image_data = nullptr;
	if (!isnull) // Ensures that this can never be re-instantiated
	{
		if (Permanent_Data_Pointer)
			free(Permanent_Data_Pointer);
		Permanent_Data_Pointer = nullptr;
		glDeleteTextures(1, &m_texture); // Delete the texture upon the asset being destroyed
		m_texture = 0;
	}
	transparency_enabled = _enableTransparency;
	isnull = false;					  // It is not null
	int width, height, numComponents; // DO NOT TOUCH!.
	temp_image_data = stbi_load(fileName.c_str(), &width, &height, &numComponents,
								4); // NOTE: THis was 4
	Permanent_Data_Pointer = temp_image_data;
	myWidth = width;
	myHeight = height;
	myComponents = numComponents;

	// std::cout << "\n WIDTH: " << width <<"\n HEIGHT: "<< height << "\n
	// NUMCOMPONENTS: " << numComponents;

	if (temp_image_data == NULL) {
		std::cerr << "Unable to load texture: " << fileName << std::endl;
		// TODO: use pink and black texture.
	}
	MyName = fileName;
	// initTexture(width, height, numComponents, temp_image_data,
	// GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, 4.0f);
	initTexture(width, height, 4, temp_image_data, minfilter, magfilter, wrap_mode,
				anisotropic_level); // NOTE TO SELF: num_components used to be
									// used here.
	if (Permanent_Data_Pointer == nullptr)
		free(temp_image_data);
	temp_image_data = nullptr;
}

// DO NOT TOUCH! it's fucking complicated but it's NOT Spaghetti code so you
// SHOULD NOT CHANGE IT. it took TOO LONG to make this work!
void Texture::initTexture(int size_w, int size_h, int size_components, unsigned char* data_ptr, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode,
						  float anisotropic_level) {
	// std::cout << "\n TEXTURE HAS BEEN INITIALIZED!!!" <<
	// "\n Size_W:" << size_w <<
	// "\n Size_H:" << size_h <<
	// "\n Size_Components:" << size_components <<
	// "\n Min Filter:" << min_filter <<
	// "\n Mag Filter:" << mag_filter <<
	// "\n Wrap Mode:" << wrap_mode <<
	// "\n Anisotropic Level:" << anisotropic_level;

	myWidth = size_w;
	myHeight = size_h;
	myComponents = size_components;
	// Permanent Data Pointer is already set in all constructors
	minfilt = min_filter;
	maxfilt = mag_filter;
	wrapmode = wrap_mode;
	myAnisotropicLevel = anisotropic_level;

	GLenum communism = glGetError();
	if (data_ptr == nullptr) {
		std::cerr << "\n !!!ERROR! Null data pointer to initialize texture class!";
		std::cerr << "\n FILE NAME: " << MyName << std::endl;
		std::abort();
	}
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture); // Bind it for editting
	// Set up the texture's properties
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_w, size_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_ptr);
	if (min_filter == GL_NEAREST_MIPMAP_NEAREST || min_filter == GL_NEAREST_MIPMAP_LINEAR || min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR || mag_filter == GL_NEAREST_MIPMAP_NEAREST || mag_filter == GL_NEAREST_MIPMAP_LINEAR ||
		mag_filter == GL_LINEAR_MIPMAP_NEAREST || mag_filter == GL_LINEAR_MIPMAP_LINEAR) { // if it in any way has mimapping

		glGenerateMipmap(GL_TEXTURE_2D); // Make those mipmaps son
		GLfloat maxAnisoTropy;
		glGetFloatv(HACK_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisoTropy);
		if (maxAnisoTropy > 0 && anisotropic_level < maxAnisoTropy && anisotropic_level > 0) // If we can do anisotropy at the specified
																							 // level AND the level makes sense
		{
			glTexParameterf(GL_TEXTURE_2D, HACK_GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisoTropy);
			// std::cout<<"\nANISOTROPY SUPPORTED!";
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	}

	// Standard error copy-paste.
	communism = glGetError();
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

void Texture::reinitFromDataPointer(bool respecify_wrap_and_filt, bool isSameSize) {
	if (!isnull) {
		// glActiveTexture(GL_TEXTURE0);
		// glDeleteTextures(1, &m_texture); //Delete the texture
		// glGenTextures(1, &m_texture); //Make a new one...
		glBindTexture(GL_TEXTURE_2D, m_texture); // Bind it for editting
		// Set up the texture's properties
		if (respecify_wrap_and_filt) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapmode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapmode);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilt);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, maxfilt);
		}
		if (isSameSize && !respecify_wrap_and_filt)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myWidth, myHeight, GL_RGBA, GL_UNSIGNED_BYTE, Permanent_Data_Pointer);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, myWidth, myHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, Permanent_Data_Pointer);
		if (respecify_wrap_and_filt) {
			if (minfilt == GL_NEAREST_MIPMAP_NEAREST || minfilt == GL_NEAREST_MIPMAP_LINEAR || minfilt == GL_LINEAR_MIPMAP_NEAREST ||
				minfilt == GL_LINEAR_MIPMAP_LINEAR || maxfilt == GL_NEAREST_MIPMAP_NEAREST || maxfilt == GL_NEAREST_MIPMAP_LINEAR ||
				maxfilt == GL_LINEAR_MIPMAP_NEAREST || maxfilt == GL_LINEAR_MIPMAP_LINEAR) { // if it in any way has mimapping

				glGenerateMipmap(GL_TEXTURE_2D); // Make those mipmaps son
				GLfloat maxAnisoTropy;
				GLfloat TempAnisoTropics = (GLfloat)myAnisotropicLevel;
				glGetFloatv(HACK_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisoTropy);
				if (maxAnisoTropy > 0 && TempAnisoTropics < maxAnisoTropy && TempAnisoTropics > 0) // If we can do anisotropy at the specified
																								   // level AND the level makes sense
				{
					glTexParameterf(GL_TEXTURE_2D, HACK_GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisoTropy);
					// std::cout<<"\nANISOTROPY SUPPORTED!";
				}
			} else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			}
		}
	} else {
		std::cout << "\nISNULL!!!";
	}
} // eof reInitFromDataPointer

Texture::~Texture() {
	glDeleteTextures(1, &m_texture); // Delete the texture upon the asset being destroyed
	if (Permanent_Data_Pointer)
		free(Permanent_Data_Pointer);
}

void Texture::bind(unsigned int unit) {
	assert(unit >= 0 && unit <= 31);
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, m_texture);
}

void Texture::bindGeneric() { glBindTexture(GL_TEXTURE_2D, m_texture); }
void Texture::setActiveUnit(unsigned int unit) {
	assert(unit >= 0 && unit <= 31);
	glActiveTexture(GL_TEXTURE0 + unit);
}
}; // namespace gekRender
