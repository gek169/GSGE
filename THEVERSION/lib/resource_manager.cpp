#include "resource_manager.h"
#include "GekAL.h"
//^^ that file says it all
namespace gekRender {
Resource_Manager::Resource_Manager() {
	Models_from_file.reserve(1000); // Make caching files a bit faster
}

Mesh* Resource_Manager::loadMesh(const std::string& fileName, bool instanced, bool is_static) {
	std::string str = fileName;
	//~ std::transform(str.begin(), str.end(), str.begin(),
	//~ ::toupper); // TODO: Remove This. Cancer I wrote for Windows
	// development.
	// it is guaranteed that we are loading an asset
	for (size_t i = 0; i < loadedMeshes.size(); i++) {
		if (loadedMeshes[i]->MyName == str && loadedMeshes[i]->is_instanced == instanced && loadedMeshes[i]->is_static == is_static) {
			// std::cout << "\n Match found.";
			return loadedMeshes[i];
		}
	}
	// Search to see if we've cached the file alraedy
	for (size_t i = 0; i < Models_from_file.size(); i++) {
		if (Models_from_file[i].myFileName == str) // If this is the correct file..
		{
			loadedMeshes.push_back( // Make a new mesh
				new Mesh(Models_from_file[i], instanced, is_static,
						 true) // Always an asset.
			);
			// and give it to them.
			return loadedMeshes[loadedMeshes.size() - 1];
		}
	}
	// If we couldn't find the mesh already loaded and we couldn't find the mesh
	// cached, then we have to load it. Load it, cache it, and instance the
	// mesh. std::cout <<"\nWe had to load " << str;

	// I get a weird error upon program ending if I don't dynamically allocate.
	// Sorry!
	OBJModel* henry = new OBJModel(str);
	Models_from_file.push_back(henry->toIndexedModel());
	loadedMeshes.push_back(new Mesh(Models_from_file[Models_from_file.size() - 1], instanced, is_static, true));
	delete henry;
	// std::cout << "\nHis name is: " <<
	// loadedMeshes[loadedMeshes.size()-1]->MyName;
	return loadedMeshes[loadedMeshes.size() - 1];
}

SafeTexture Resource_Manager::loadTexture(const std::string& fileName,
										  bool enableTransparency) { // The object is responsible for deleting what it
																	 // gets
	std::string str = fileName;
	//~ std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	for (size_t i = 0; i < loadedTextures.size(); i++) {
		if (loadedTextures[i]->MyName == str && loadedTextures[i]->amITransparent() == enableTransparency) {
			SafeTexture temp = SafeTexture(loadedTextures[i]);
			return temp;
		}
	}
	// std::cout << "\n we had to load texture: " << str << " from file";
	loadedTextures.push_back(new Texture(str, enableTransparency));
	SafeTexture temp = SafeTexture(loadedTextures[loadedTextures.size() - 1]); // No longer a memory leak thanks to doctor memory!
	return temp;
}
ALuint Resource_Manager::loadSound(const std::string& fileName) {
	if (loadedSoundBuffers.count(fileName) > 0) {
		return loadedSoundBuffers[fileName];
	} else {
		// std::cout << "\nHAD TO LOAD " << fileName;
		loadedSoundBuffers[fileName] = loadWAVintoALBuffer(fileName.c_str());
		return loadedSoundBuffers[fileName];
	}
}

Resource_Manager::~Resource_Manager() {
	/*
  for (int i = 0; i< loadedMeshes.size(); i++){
			delete loadedMeshes[i];
	}
	for (int i = 0; i< loadedTextures.size(); i++){
			delete loadedTextures[i];
	}
	*/
	if (loadedMeshes.size() > 0)
		while (loadedMeshes.size() > 0) {
			delete loadedMeshes[loadedMeshes.size() - 1];
			loadedMeshes.erase(loadedMeshes.begin() + (loadedMeshes.size() - 1));
		}
	if (loadedTextures.size() > 0)
		while (loadedTextures.size() > 0) {
			delete loadedTextures[loadedTextures.size() - 1];
			loadedTextures.erase(loadedTextures.begin() + (loadedTextures.size() - 1));
		}
	// std::cout <<"\nDELETING SOUNDS...";
	std::map<std::string, ALuint>::iterator it;
	for (it = loadedSoundBuffers.begin(); it != loadedSoundBuffers.end(); it++) {
		alDeleteBuffers(1, &(it->second));
	}
	// std::cout <<"\nSOUNDS DELETED!!!!";
}
}; // namespace gekRender
