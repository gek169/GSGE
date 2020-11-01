#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "SafeTexture.h"
#include "mesh.h"
#include "obj_loader.h"
#include "texture.h"
//~ #include "GekAL.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

// THIS FILE ONLY INCLUDED FOR THIS DEMO, YOU CAN REMOVE IT IN YOUR OWN PROGRAMS
// AND IN FACT THIS METHOD OF LOADING FILES IS ONLY PARTIALLY DEVELOPED SO IT IS
// RECOMMENDED YOU MANAGE RESOURCES ON YOUR OWN.

// Manages anything and everything that is on the GPU or loaded from disk so
// that it never has to be loaded from disk again It should be --more than
// simple-- for anybody, even someone who has never coded in C++ before, to
// extend this class to all assets and things which someone may want to load.
// You need to create an instance of it.
namespace gekRender {
class Resource_Manager {
  public:
	Resource_Manager();
	virtual ~Resource_Manager();
	// REMEMBER: This is the resource/asset manager. If you want a custom,
	// dynamic mesh for your scene, then you are not going to use this class.
	Mesh* loadMesh(const std::string& fileName, bool instanced, bool is_static);

	// I really don't think anyone will want to load a texture without it being
	// static, and even if they did, it's trivial to do so with a simple shader.
	// NOTE TO SELF: Make this return a literal. Pointers are less efficient.
	// We'll do it after we make Mesh the primary draw-er
	SafeTexture loadTexture(const std::string& fileName, bool enableTransparency);

	ALuint loadSound(const std::string& fileName);

  protected:
  private:
	std::vector<Mesh*> loadedMeshes;
	std::map<std::string, ALuint> loadedSoundBuffers;
	std::vector<IndexedModel> Models_from_file;
	std::vector<Texture*> loadedTextures;
};
};	 // namespace gekRender
#endif // Resource_Manager_H
