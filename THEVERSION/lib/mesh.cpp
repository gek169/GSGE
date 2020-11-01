#include "mesh.h"
#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#include "obj_loader.h"
#include <iostream>
#include <string>
#include <vector>
#define GLM_FORCE_RADIANS
#include <cstdlib>
#include <glm/glm.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <map>

namespace gekRender {

MeshInstance::MeshInstance(int _texind, Transform _transform) {
	tex = _texind;
	myTransform = _transform;
}
MeshInstance::MeshInstance() {} // Default constructor
MeshInstance::~MeshInstance() {
	/*
	if(BoneUBO > 0)
	{
			glDeleteBuffers(1, &BoneUBO);
			BoneUBO = 0;
	}*/
}

MeshInstance::MeshInstance(const MeshInstance& Other) {
	// Skeleton = Other.Skeleton;
	tex = Other.tex;
	cubeMap = Other.cubeMap;
	EnableCubemapDiffusion = Other.EnableCubemapDiffusion;
	EnableCubemapReflections = Other.EnableCubemapReflections;
	myTransform = Other.myTransform;
	myPhong = Other.myPhong;
	mymeshmask = Other.mymeshmask;
	shouldRender = Other.shouldRender;
}

Mesh::Mesh() {
	if (!isnull) {
		glDeleteBuffers(NUM_BUFFERS, m_vertexArrayBuffers);
		if (is_instanced)
			glDeleteBuffers(1, &InstanceModelMatrixVBO);
		glDeleteVertexArrays(1, &m_vertexArrayObject);
		if (MatrixDataCacheMalloced.size() > 0)
			MatrixDataCacheMalloced.clear();
		// TODO: Push this update to the main SceneAPI (In 147)
		InstanceModelMatrixVBO = 0;
		m_vertexArrayObject = 0;
	}
	isnull = true;
}

/**
Fast version:
Loads the mesh from filebuf
saves it
gets an indexed model
saves it

fiddles around with filenames a bit

then initializes
**/
Mesh::Mesh(std::string filename, bool instanced, bool isstatic, bool assetjaodernein, bool recalculatenormals) {
	// std::cout<<"\nCurrently loading mesh: " <<filename;
	IndexedModel mmphmodel; // To avoid confusing with the mesh member variable
	OBJModel mdl = OBJModel(filename);
	mmphmodel = mdl.toIndexedModel();
	if (recalculatenormals) {
		// std::cout << "\nRecalculating Normals!";
		for (size_t i = 0; i < mmphmodel.normals.size(); i++)
			mmphmodel.normals[i] = glm::vec3(0, 0, 0);
		mmphmodel.calcNormals();
	}
	// std::cout<<"\nModel " << filename << " has " << mmphmodel.indices.size()
	// << " Indices or " << mmphmodel.indices.size()/3.0 << " Tris to be
	// rendered every frame." << std::endl; std::cout << "\nDid it have
	// Normals?" << mdl.hasNormals?" TRUE!":" FALSE!";
	MyName = filename;
	is_instanced = instanced; // Do we have hardware instancing?
	is_static = isstatic;	 // can we edit the VBOs
	if (isnull)
		initMesh(mmphmodel, instanced, isstatic); // make the OpenGL Object
	else
		reShapeMesh(mmphmodel);
	is_asset = assetjaodernein; // is the resource manager responsible for
								// deleting this, external use only
								// std::cout<<"\n My name is " + MyName + "\n";
}

Mesh::Mesh(const Mesh& Other) {
	if (isnull)
		initMesh(Other.ModelData, Other.is_instanced, Other.is_static);
	else
		reShapeMesh(Other.ModelData);
}
Mesh::Mesh(IndexedModel usethisone, bool instancing, bool shallwemakeitstatic, bool assetjaodernein) {

	is_instanced = instancing;		 // Do we have hardware instancing?
	is_static = shallwemakeitstatic; // can we edit the VBOs
	is_asset = assetjaodernein;

	if (isnull)
		initMesh(usethisone, instancing, shallwemakeitstatic);
	else
		reShapeMesh(usethisone);
	//~ std::cout << "\nFinished Instantiating..." << std::endl;
}

void Mesh::optimizeCacheMemoryUsage(bool ShrinkToFit) {
	MatrixDataCacheMalloced.clear();
	auto iter = std::find(Instances.begin(), Instances.end(), nullptr);
	while (iter != Instances.end()) {
		Instances.erase(iter);
		iter = std::find(Instances.begin(), Instances.end(), nullptr);
	}
	if (ShrinkToFit)
		Instances.shrink_to_fit();
}

void Mesh::setFlags(unsigned int input) { renderflags = input; }
unsigned int Mesh::getFlags() { return renderflags; }

void Mesh::registerInstance(MeshInstance* Doggo) { // This has to be FAST, No checking.
	// Instances.push_back(Doggo);

	auto iter = std::find(Instances.begin(), Instances.end(), nullptr);
	if (iter != Instances.end()) {
		size_t whichone = std::distance(Instances.begin(), iter);
		Instances[whichone] = Doggo;
		// Instances.erase(iter);
		// return true;
	} else {
		Instances.push_back(Doggo);
	}
}
bool Mesh::deregisterInstance(MeshInstance* Doggo) {
	// if (Instances.size() > 0)
	// for (int i = 0; i < Instances.size(); i++)
	// if (Doggo == Instances[i]) // This is not working for some reason
	// {
	// std::cout<<"\n!!!Deregistering... Pre:" <<Instances.size();
	// Instances.erase(Instances.begin() + i);
	// std::cout<<"\nPost:" <<Instances.size();
	// return true;
	// }

	auto iter = std::find(Instances.begin(), Instances.end(), Doggo);
	if (iter != Instances.end()) {
		size_t whichone = std::distance(Instances.begin(), iter);
		Instances[whichone] = nullptr;
		// Instances.erase(iter);
		return true;
	}
	return false;
}
//~ void PassBoneInfoToMeshInstance(MeshInstance& Doggo){
//~ std::cout << "\nPassing info from Mesh to Meshinstance!" << std::endl;
//~ Doggo.Skeleton = ModelData.Bones;
//~ }

void Mesh::drawGeneric() {
	glBindVertexArray(m_vertexArrayObject);
	glDrawElements(GL_TRIANGLES, m_drawCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::drawInstancesPhong(GLuint Model, GLuint rflags, GLuint specr, GLuint specd, GLuint diff, GLuint emiss, GLuint enableCubemaps,
							  GLuint enableCubemaps_diffuse, GLuint EnableTransparency, GLuint EnableInstancing, GLuint BoneUBOLocation,
							  GLuint BoneUBOBindPoint, GLuint HasBones, GLuint texOffsetLocation, Shader* Shadman, bool Transparency, bool toRenderTarget, int meshmask, bool usePhong,
							  bool donotusemaps, bool force_non_instanced) {
	if (Instances.size() == 0 || //No instances to render
		meshmask == 0 || //Meshmask passed to this function is invalid.
		(meshmask != 0 && mesh_meshmask % meshmask != 0) //our meshmask is set for this draw.
	)
		return;
	if ((!is_instanced || force_non_instanced)) { //Non-instanced rendering
		glUniform1f(EnableInstancing, 0);
		
		glBindVertexArray(m_vertexArrayObject);
		glUniform1ui(rflags, renderflags);
		std::map<unsigned int,std::vector<MeshInstance*>> Batches;
		// GENERATE THE BATCHES
		for (size_t i = 0; i < Instances.size(); i++) // for each thing
		{
			//Skip instances that we shouldn't be rendering.
			if (!Instances[i] || 
				(!Instances[i]->shouldRender) || 
				Instances[i]->culled || 
				(meshmask != 0 && Instances[i]->mymeshmask % meshmask != 0)
			)
				continue;
			//Skip instances which use a texture that doesn't match the transparency flag.
			if (
					(Instances[i] && 
					MyTextures.size() > Instances[i]->tex && 
					MyTextures[Instances[i]->tex].amITransparent() != Transparency) ||
					(MyTextures.size() == 0 && Transparency)
				)
				continue;
			Batches[Instances[i]->tex].push_back(Instances[i]);
		}
		//For all batches
		for (auto it = Batches.begin(); it != Batches.end(); it++)
		{
			std::vector<MeshInstance*>& LInstances = it->second;
			//Bind the texture associated with the batch.
			if (MyTextures.size() > it->first)
				MyTextures[it->first].bind(0);
			//Draw all the instances in this batch.
			for (size_t i = 0; i < LInstances.size(); i++) // for each thing
			{
				if (LInstances[i]->cubeMap < myCubeMaps.size() &&
					LInstances[i]->cubeMap >= 0 && 
					myCubeMaps[LInstances[i]->cubeMap])
					myCubeMaps[LInstances[i]->cubeMap]->bind(1);

				glUniformMatrix4fv(Model, 1, GL_FALSE, &(LInstances[i]->myTransform.getModel()[0][0]));
				if (usePhong) {
					glUniform2f(texOffsetLocation,
					LInstances[i]->texOffset.x,
					LInstances[i]->texOffset.y
					);
					glUniform1f(specr,
								LInstances[i]->myPhong.specreflectivity); // Specr
					glUniform1f(specd, LInstances[i]->myPhong.specdamp);  // specular
																		 // dampening
					glUniform1f(diff,
								LInstances[i]->myPhong.diffusivity); // Diffusivity of the material
					glUniform1f(emiss, 
								LInstances[i]->myPhong.emissivity); // Emissivity of the material
					glUniform1f(enableCubemaps,
								LInstances[i]->EnableCubemapReflections); // show cubemap in
																		 // reflections?
					glUniform1f(enableCubemaps_diffuse, LInstances[i]->EnableCubemapDiffusion);
				}
				if (Transparency)
					glUniform1f(EnableTransparency, 1.0f);
				else
					glUniform1f(EnableTransparency, 0.0f);
				

				// Drawing
				glDrawElements(GL_TRIANGLES, m_drawCount, GL_UNSIGNED_INT, 0);
			}
		}
		glBindVertexArray(0);
		// INSTANCING ELSE
	} else { // Is instanced
		// Perform testing to ensure that there is a texture to render with, and
		// that its transparency flag matches the transparency state.
		if ((MyTextures.size() > 0 && MyTextures[0].amITransparent() == Transparency) || (MyTextures.size() == 0 && !Transparency)) 
		{

			// glGetError(); //Clear out errors. Error checking demands it!
			// std::cout << "\n ATTEMPTING INSTANCED RENDERING!";

			// Almost forgot this part!
			glEnableVertexAttribArray(4);
			glEnableVertexAttribArray(5);
			glEnableVertexAttribArray(6);
			glEnableVertexAttribArray(7);

			glBindVertexArray(m_vertexArrayObject);

			// Set Instancing Variable
			glUniform1f(EnableInstancing, 1);

			m_instanceDrawCount = 0; // reset instance count
			glBindBuffer(GL_ARRAY_BUFFER,
						 InstanceModelMatrixVBO); // Work with the Instancing Buffer
			if (!useMapping || forceNoImplicitSync) {
				MatrixDataCacheMalloced.reserve(Instances.size() * 16);
				for (size_t i = 0; i < Instances.size(); i++) // for each thing
				{
					// Complicated check to ensure that no null ptrs have their
					// methods called
					if (!Instances[i] || (!Instances[i]->shouldRender) || Instances[i]->culled || (Instances[i]->mymeshmask % meshmask != 0))
						continue;
					memcpy(&MatrixDataCacheMalloced[m_instanceDrawCount * 16], &(Instances[i]->myTransform.getModel()[0][0]), sizeof(GLfloat) * 16);
					// add 1 to the number of instances
					m_instanceDrawCount += 1;
				}

				if ((InstancedMatrixSizeLastFrame < m_instanceDrawCount * 16 * sizeof(GLfloat)) || (m_instanceDrawCount == 0) || forceNoImplicitSync)
					glBufferData(GL_ARRAY_BUFFER, m_instanceDrawCount * 16 * sizeof(GLfloat), &MatrixDataCacheMalloced[0],
								 GL_DYNAMIC_DRAW); // This does not rely on the Vsync
				else {
					glBufferSubData(GL_ARRAY_BUFFER, 0, m_instanceDrawCount * 16 * sizeof(GLfloat), &MatrixDataCacheMalloced[0]);
				}
				InstancedMatrixSizeLastFrame = m_instanceDrawCount * 16 * sizeof(GLfloat);
			} else { // Attempt to use mapping.
				uint tempsize = Instances.size();
				if ((tempsize == 0) || tempsize * 16 * sizeof(GLfloat) > InstancedMatrixSizeLastFrame)
					glBufferData(GL_ARRAY_BUFFER, tempsize * 16 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW); // This does not rely on the Vsync
				float* MappedData = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, tempsize * 16 * sizeof(GLfloat), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				if (MappedData) {
					for (size_t i = 0; i < Instances.size(); i++) // for each thing
					{
						// Complicated check to ensure that no null ptrs have their
						// methods called
						if (!Instances[i] || (!Instances[i]->shouldRender) || Instances[i]->culled || (Instances[i]->mymeshmask % meshmask != 0))
							continue;
						memcpy(&MappedData[m_instanceDrawCount * 16], &(Instances[i]->myTransform.getModel()[0][0]), sizeof(GLfloat) * 16);
						// add 1 to the number of instances
						m_instanceDrawCount += 1;
					}
					InstancedMatrixSizeLastFrame = tempsize * 16 * sizeof(GLfloat);
					glUnmapBuffer(GL_ARRAY_BUFFER);
				} else {
					std::cout << "\nERROR! GLMAPBUFFER FAILED FOR INSTANCED MESH!" << std::endl;
				}
			}
			if (usePhong) {
				glUniform2f(
					texOffsetLocation,
					instanced_texOffset.x,
					instanced_texOffset.y
				);
				glUniform1f(specr, InstancedMaterial.specreflectivity); // Specr
				glUniform1f(specd,
							InstancedMaterial.specdamp); // specular dampening
				glUniform1f(diff,
							InstancedMaterial.diffusivity); // Diffusivity of the material
				glUniform1f(emiss,
							InstancedMaterial.emissivity); // Emissivity of the material
				glUniform1f(enableCubemaps,
							instanced_enable_cubemap_reflections ? 1.0f : 0.0f); // show cubemap in reflections?
				glUniform1f(enableCubemaps_diffuse, instanced_enable_cubemap_diffusion ? 1.0f : 0.0f);
			}
			// glUniform1f(m_instanced_float_loc, 0); //We are NOT instanced,
			// use the modelmatrix VBO.
			if (Transparency)
				glUniform1f(EnableTransparency, 1.0f);
			else
				glUniform1f(EnableTransparency, 0.0f);

			// Fourth, Bind texture
			if (MyTextures.size() > 0)
				MyTextures[0].bind(0);
			// FOURTH, BIND TEXTURES
			if (myCubeMaps.size() > 0)
				myCubeMaps[0]->bind(1);
			glUniform1ui(rflags, renderflags);
			
			glDrawElementsInstanced(GL_TRIANGLES, m_drawCount, GL_UNSIGNED_INT, 0, m_instanceDrawCount);
			// */
			// Disable vertex attribute arrays
			glBindVertexArray(0);
			//~ glDisableVertexAttribArray(4);
			//~ glDisableVertexAttribArray(5);
			//~ glDisableVertexAttribArray(6);
			//~ glDisableVertexAttribArray(7);
			// std::cout << "\nDREW INSTANCED!!!" << std::endl;

		} // eof (if mytextures dot size...)
		{
			shouldremakeinstancedmodelvector = true;
			canbereshaped = true;
		}
	} // eof is instanced
} // Eof DrawInstancesPhong

void Mesh::pushTexture(SafeTexture _tex) { // CHECKED NO PROBLEMS WITH ITER
	MyTextures.push_back(_tex);
}

bool Mesh::removeTexture(GLuint _tex) {
	bool returnval = false;
	if (MyTextures.size() < 1) // Just in case...
		return false;
	for (auto i = MyTextures.begin(); i < MyTextures.end(); ++i) {
		// if (i >= MyTextures.begin() && i < MyTextures.end())
		if ((*i).getHandle() == _tex) {
			i = MyTextures.erase(i);
			returnval = true;
		}
	}
	return returnval;
}

void Mesh::pushCubeMap(CubeMap* _cube) { myCubeMaps.push_back(_cube); }

bool Mesh::removeCubeMap(GLuint _cube) {
	bool returnval = false;
	if (myCubeMaps.size() < 1) // Just in case...
		return false;
	for (auto i = myCubeMaps.begin(); i < myCubeMaps.end(); ++i) {
		// if (i >= myCubeMaps.begin() && i < myCubeMaps.end())
		if ((*i)->getHandle() == _cube) {
			i = myCubeMaps.erase(i);
			returnval = true;
		}
	}
	return returnval;
}

bool Mesh::removeCubeMap(CubeMap* TheOneTheOnly) {
	bool retval = false;
	auto iter = std::find(myCubeMaps.begin(), myCubeMaps.end(), TheOneTheOnly);
	if (iter != myCubeMaps.end()) {
		iter = myCubeMaps.erase(iter);
		retval = true;
	}
	return retval;
}

// for manually testing the vector to see which element is what texture. You
// would essentially want to use this to debug how many safetextures you have,
// and pick suitable elements.
std::vector<SafeTexture>* Mesh::getTextureVectorPtr() { return &MyTextures; }
std::vector<CubeMap*>* Mesh::getCubeMapVectorPtr() { return &myCubeMaps; }
std::vector<MeshInstance*>* Mesh::getInstanceVectorPtr() { return &Instances; }
int Mesh::numTextures() { return MyTextures.size(); }
int Mesh::numInstances() { return Instances.size(); }

GLuint Mesh::getVAOHandle() { return m_vertexArrayObject; }
unsigned int Mesh::getDrawCount() { return m_drawCount; }
IndexedModel Mesh::getShape() { return ModelData; }

// Big scary function
// Rundown:
// Generates and Binds the VAO
// Generates the VBOs
// For all the VBOs, it binds them and sends in the data
// This is the function which I imagine will confuse many newbies to OpenGL. the
// VBO/VAO system is somewhat confusing, although when you do get your head
// around it, it will become second nature DON'T USE IMMEDIATE MODE! There is
// never EVER a reason to use deprecated OpenGL on a modern computer! Just don't
// do it, it pisses me off. Steal my code (here) instead, I don't care.
void Mesh::initMesh(IndexedModel model, bool instancing, bool shallwemakeitstatic) {

	if (!isnull) { // WE DONT RE-INITIALIZE IN THIS HOUSE!!!
		std::cerr << "\n ERROR! It is INVALID to call the constructor of an "
					 "already-instantiated OpenGL Object!\n It is the convention "
					 "of this library for assets and MOST opengl objects to be "
					 "non-reinstantiatable."
				  << "\n\n at the current time that includes these classes:"
				  << "\n Shader"
				  << "\n Cubemap"
				  << "\n If you want to re-instantiate one of these classes, just "
					 "delete it and make a new one. ";
		std::abort();
		// in case that didn't get them hehe
		int* p = nullptr;
		*p = 0;
	}
	ModelData = model; // Save it.
	isnull = false;
	is_instanced = instancing;
	is_static = shallwemakeitstatic;
	if (ModelData.hadRenderFlagsInFile) {
		setFlags(ModelData.renderflags);
	} else
		renderflags = GK_RENDER | GK_TEXTURED | GK_TINT;

	MyName = ModelData.myFileName;
	unsigned int numIndices = (unsigned int)ModelData.indices.size();

	m_drawCount = numIndices; // m_ convention is incorrect here.
	unsigned int numVertices = ModelData.positions.size();
	glGenVertexArrays(1,
					  &m_vertexArrayObject); // allocate a vertex array object
	glBindVertexArray(m_vertexArrayObject);  // Work with this VAO

	/// BUFFERS!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	/// BUFFERS!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	/// BUFFERS!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	glGenBuffers(NUM_BUFFERS, m_vertexArrayBuffers); // Make NUM_BUFFERS buffers
	if (is_instanced) {
		glGenBuffers(1, &InstanceModelMatrixVBO);
		// std::cout << "\n Created Instance Model Matrix VBO!";

		glBindBuffer(GL_ARRAY_BUFFER,
					 InstanceModelMatrixVBO); // Work with the Instanced Model
											  // Matrix Buffer
		glEnableVertexAttribArray(4);
		glEnableVertexAttribArray(5);
		glEnableVertexAttribArray(6);
		glEnableVertexAttribArray(7);
		//                    #  #floats    type     normalize  how many per
		//                    cycle? offset
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16 * (int)sizeof(GL_FLOAT), 0);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 16 * (int)sizeof(GL_FLOAT), (void*)(4 * sizeof(GL_FLOAT)));
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 16 * (int)sizeof(GL_FLOAT), (void*)(8 * sizeof(GL_FLOAT)));
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 16 * (int)sizeof(GL_FLOAT), (void*)(12 * sizeof(GL_FLOAT)));

		glVertexAttribDivisor(4, 1); // col A
		glVertexAttribDivisor(5, 1); // col B
		glVertexAttribDivisor(6, 1); // col C
		glVertexAttribDivisor(7, 1); // col D
	}

	//~ std::cout << "\nMarker 3" << std::endl;

	/// Buffer 1: Positions
	glBindBuffer(GL_ARRAY_BUFFER,
				 m_vertexArrayBuffers[POSITION_VB]); // Work with the Position Buffer

	GLfloat* ContiguousMemory = (GLfloat*)malloc(numVertices * 3 * sizeof(GLfloat)); // we use this again later...
	for (int i = 0; i < numVertices; i++) {
		ContiguousMemory[(3 * i)] = ModelData.positions[i].x;
		ContiguousMemory[(3 * i) + 1] = ModelData.positions[i].y;
		ContiguousMemory[(3 * i) + 2] = ModelData.positions[i].z;
	}
	glBufferData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(GLfloat), ContiguousMemory,
				 is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW); // Note to self: Deletes all pre-existing
																// storage for the currently bound buffer,
																// so we could use it for an expensive way
																// of uploading buffer data. EDIT: Which
																// is exactly what we are doing!!!
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	free(ContiguousMemory);

	//~ std::cout << "\nMarker 4" << std::endl;

	// Texture Coordinate Information
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[TEXCOORD_VB]);
	ContiguousMemory = (GLfloat*)malloc(ModelData.texCoords.size() * 2 * sizeof(GLfloat));
	for (int i = 0; i < ModelData.texCoords.size(); i++) {
		ContiguousMemory[(2 * i)] = ModelData.texCoords[i].x;
		ContiguousMemory[(2 * i) + 1] = ModelData.texCoords[i].y;
	}
	glBufferData(GL_ARRAY_BUFFER, ModelData.texCoords.size() * 2 * sizeof(GLfloat), ContiguousMemory, is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	free(ContiguousMemory);
	//~ std::cout << "\nMarker 5" << std::endl;
	// Normal Information
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[NORMAL_VB]);
	ContiguousMemory = (GLfloat*)malloc(ModelData.normals.size() * 3 * sizeof(GLfloat));
	for (int i = 0; i < ModelData.normals.size(); i++) {
		ContiguousMemory[(3 * i)] = ModelData.normals[i].x;
		ContiguousMemory[(3 * i) + 1] = ModelData.normals[i].y;
		ContiguousMemory[(3 * i) + 2] = ModelData.normals[i].z;
	}
	glBufferData(GL_ARRAY_BUFFER, ModelData.normals.size() * 3 * sizeof(GLfloat), ContiguousMemory, is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	free(ContiguousMemory);

	//~ std::cout << "\nMarker 6" << std::endl;

	// std::cout << "\nMy name is: " << MyName << " and I have " << numVertices
	// << " but the vertex array is of size " << ModelData.positions.size() <<
	// "and the color buffer is of size " << ModelData.colors.size(); Color
	// buffer information
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[COLOR_VB]);
	ContiguousMemory = (GLfloat*)malloc(ModelData.colors.size() * 3 * sizeof(GLfloat)); // UNADDRESSABLE ACCESS according to
																						// Dr. Memory (solved??? see below)
	if (ModelData.colors.size() > 0)
		for (int i = 0; i < ModelData.colors.size(); i++) {
			ContiguousMemory[(3 * i)] = ModelData.colors[i].x;
			ContiguousMemory[(3 * i) + 1] = ModelData.colors[i].y;
			ContiguousMemory[(3 * i) + 2] = ModelData.colors[i].z;
		}
	glBufferData(GL_ARRAY_BUFFER, ModelData.colors.size() * 3 * sizeof(GLfloat), ContiguousMemory,
				 is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW); // I KNOW WHY DOCTOR MEMORY WAS ANGRY.
																// I was doing the screenquad. The
																// screenquad in the scene class has no
																// color buffer. Silly me! (solved???)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	free(ContiguousMemory);
	//~ std::cout << "\nMarker 7" << std::endl;
	// This is where you make per-instance buffers if you want.

	// NOTE: PLEASE DO NOT ADD YOUR CUSTOM BUFFER HERE UNLESS YOU ARE PREPARED
	// TO RE-WRITE THE OBJLOADER! Load your bullshit in a separate function.
	// That way, everything works out nicely and neatly

	// Index Buffer Information
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vertexArrayBuffers[INDEX_VB]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(unsigned int), &ModelData.indices[0], is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);

	//~ std::cout << "\nMarker 8" << std::endl;
	glBindVertexArray(0);
}

void Mesh::reShapeMesh(const IndexedModel& model) {
	// std::cout <<"\n Friendly neighborhood debugger again. Model is: \"" <<
	// MyName << "\"\n and I'm going be debugging the COLOR_VB buffer input.";
	// std::cout <<"\n There are " << model.colors.size() <<" color vectors.\n
	// For comparison, there are " << model.normals.size() << " normal vectors
	// and "
	// << model.positions.size()<<" verts\n"; if(MyName == "happy.OBJ") //WOOF!
	// for(int i = 0; i<model.colors.size(); i++)
	// {
	// std::cout << "\nCOLOR " << i << " R:" << model.colors[i].x << " G:" <<
	// model.colors[i].y << " B:" << model.colors[i].z;
	// }
	// std::cout << "\n initializing mesh " << model.myFileName<< " has " <<
	// model.indices.size() << " Indices or " << model.indices.size()/3.0 << "
	// Tris to be rendered every frame.";;

	// If we allowed someone to reshape a mesh multiple times before the vsync,
	// the memory might fuck up. Do not let it happen.
	if (!canbereshaped)
		return;
	//~ std::cout << "\nReshaping Mesh!" << std::endl;
	if (isnull) { // WE DONT RE-INITIALIZE IN THIS HOUSE!!!
		std::cerr << "ERROR! You tried to Re-Initialize a mesh without first "
					 "initializing! The engine will crash now. Goodbye."
				  << std::endl;
		std::abort();
		// in case that didn't get them hehe
		int* p = nullptr;
		*p = 0;
	}
	// std::cout << "\n Managed to get past isnull check!!!";
	ModelData = model; // Save it.
	if (model.hadRenderFlagsInFile) {
		setFlags(model.renderflags);
	} else
		renderflags = GK_RENDER | GK_TEXTURED | GK_TINT; // We want to ensure that users who load ordinary
														 // OBJs are fully accomodated. No encumberments.

	MyName = model.myFileName;
	unsigned int numIndices = (unsigned int)model.indices.size();

	m_drawCount = numIndices; // m_ convention is incorrect here.
	unsigned int numVertices = model.positions.size();
	// glGenVertexArrays(1, &m_vertexArrayObject);//allocate a vertex array
	// object
	glBindVertexArray(m_vertexArrayObject); // Work with this VAO

	/// Buffer 1: Positions
	glBindBuffer(GL_ARRAY_BUFFER,
				 m_vertexArrayBuffers[POSITION_VB]); // Work with the Position Buffer

	GLfloat* ContiguousMemory = (GLfloat*)malloc(numVertices * 3 * sizeof(GLfloat)); // we use this again later...
	for (int i = 0; i < numVertices; i++) {
		ContiguousMemory[(3 * i)] = model.positions[i].x;
		ContiguousMemory[(3 * i) + 1] = model.positions[i].y;
		ContiguousMemory[(3 * i) + 2] = model.positions[i].z;
	}
	glBufferData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(GLfloat), ContiguousMemory,
				 is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW); // Note to self: Deletes all pre-existing
																// storage for the currently bound buffer,
																// so we could use it for an expensive way
																// of uploading buffer data. EDIT: Which
																// is exactly what we are doing!!!
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	free(ContiguousMemory);

	// Texture Coordinate Information
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[TEXCOORD_VB]);
	ContiguousMemory = (GLfloat*)malloc(model.texCoords.size() * 2 * sizeof(GLfloat));
	for (int i = 0; i < model.texCoords.size(); i++) {
		ContiguousMemory[(2 * i)] = model.texCoords[i].x;
		ContiguousMemory[(2 * i) + 1] = model.texCoords[i].y;
	}
	glBufferData(GL_ARRAY_BUFFER, model.texCoords.size() * 2 * sizeof(GLfloat), ContiguousMemory, is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	free(ContiguousMemory);

	// Normal Information
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[NORMAL_VB]);
	ContiguousMemory = (GLfloat*)malloc(model.normals.size() * 3 * sizeof(GLfloat));
	for (int i = 0; i < model.normals.size(); i++) {
		ContiguousMemory[(3 * i)] = model.normals[i].x;
		ContiguousMemory[(3 * i) + 1] = model.normals[i].y;
		ContiguousMemory[(3 * i) + 2] = model.normals[i].z;
	}
	glBufferData(GL_ARRAY_BUFFER, model.normals.size() * 3 * sizeof(GLfloat), ContiguousMemory, is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	free(ContiguousMemory);

	// m_drawCount

	// std::cout << "\nMy name is: " << MyName << " and I have " << numVertices
	// << " but the vertex array is of size " << model.positions.size() << "and
	// the color buffer is of size " << model.colors.size(); Color buffer
	// information
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[COLOR_VB]);
	ContiguousMemory = (GLfloat*)malloc(model.colors.size() * 3 * sizeof(GLfloat)); // UNADDRESSABLE ACCESS according to
																					// Dr. Memory (solved??? see below)
	if (model.colors.size() > 0)
		for (int i = 0; i < model.colors.size(); i++) {
			ContiguousMemory[(3 * i)] = model.colors[i].x;
			ContiguousMemory[(3 * i) + 1] = model.colors[i].y;
			ContiguousMemory[(3 * i) + 2] = model.colors[i].z;
		}

	glBufferData(GL_ARRAY_BUFFER, model.colors.size() * 3 * sizeof(GLfloat), ContiguousMemory,
				 is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW); // I KNOW WHY DOCTOR MEMORY WAS ANGRY.
																// I was doing the screenquad. The
																// screenquad in the scene class has no
																// color buffer. Silly me! (solved???)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	free(ContiguousMemory);

	// This is where you make per-instance buffers if you want.

	// NOTE: PLEASE DO NOT ADD YOUR CUSTOM BUFFER HERE UNLESS YOU ARE PREPARED
	// TO RE-WRITE THE OBJLOADER! Load your bullshit in a separate function.
	// That way, everything works out nicely and neatly

	// Index Buffer Information
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vertexArrayBuffers[INDEX_VB]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(unsigned int), &model.indices[0], is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	glBindVertexArray(0);
	//~ canbereshaped = false;
	// std::cout << "\n Finished Function";
}

// This code is only here as a memory... it was my first functional, custom draw
// code in modern OpenGL. Toot your horns in HONOR!!! void Mesh::Draw(){
//	glBindVertexArray(m_vertexArrayObject);
//	//std::cout<<"\nBoundVertexArray!!!";
//	//glDrawArrays(GL_TRIANGLES, 0, m_drawCount);
//	//std::cout<<"\nDREWARRAYS!";
//	glDrawElements(GL_TRIANGLES, m_drawCount, GL_UNSIGNED_INT, 0);
//	glBindVertexArray(0);
//	//std::cout<<"\nStumped.";
//}

Mesh::~Mesh() {
	// Handles the deletion of our stuff.
	glDeleteBuffers(NUM_BUFFERS, m_vertexArrayBuffers);
	if (is_instanced)
		glDeleteBuffers(1, &InstanceModelMatrixVBO);
	glDeleteVertexArrays(1, &m_vertexArrayObject);
	if (MatrixDataCacheMalloced.size() > 0)
		MatrixDataCacheMalloced.clear();

	isnull = true;
}

// Particle Renderer crap.
void ParticleRenderer::DrawParticles(Camera* myCam) {

	if (isNull || !myShader || !myCam || myTexture.amINull())
		return;
	//~ std::cout << "\nYes, this is drawing." << std::endl;
	GLsizei numParticles;
	for (numParticles = particleData.size() / 7; numParticles > 0; numParticles--)
		if (
			// 0,1,2 are P0, 3,4,5 are V0, 6 is BirthT. So we're testing
			// Birthtime to see if it's a needs-to-be-rendered particle.
			((current_time - particleData[(numParticles - 1) * 7 + 6]) < max_age) && (current_time >= particleData[(numParticles - 1) * 7 + 6]))
			break; // We've found a particle that requires rendering! STOP
				   // SUBTRACTING!
	//~ std::cout << "\nNumParticles: " << numParticles;
	myShader->bind();
	myTexture.bind(0);
	glEnableVertexAttribArray(Attrib_initPos);
	glEnableVertexAttribArray(Attrib_initV);
	glEnableVertexAttribArray(Attrib_birthT);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[DATA_VBO]);
	//~ glBufferData(GL_ARRAY_BUFFER, numParticles * 7 * sizeof(GLfloat), &(particleData[0]), GL_DYNAMIC_DRAW);
	//~ glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * 7 * sizeof(GLfloat), &(particleData[0]), GL_DYNAMIC_DRAW);

	if (!forceNoImplicitSync) {
		float* MappedData = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, numParticles * 7 * sizeof(GLfloat), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		if (MappedData) {
			memcpy(MappedData, &(particleData[0]), numParticles * 7 * sizeof(GLfloat));
			glUnmapBuffer(GL_ARRAY_BUFFER);
		} else {
			//~ std::cout << "\nMAP FAILED!" << std::endl;
			//~ glUnmapBuffer(GL_ARRAY_BUFFER);
			glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * 7 * sizeof(GLfloat), &(particleData[0]));
		}
	} else {
		glBufferData(GL_ARRAY_BUFFER, numParticles * 7 * sizeof(GLfloat), &(particleData[0]), GL_DYNAMIC_DRAW);
	}

	// To upload the buffer to the GPU.
	// and do it here.
	if (!haveFoundUniformLocations) {
		haveFoundUniformLocations = true;
		u_ViewMatrixLoc = myShader->getUniformLocation("ViewMatrix");
		u_ProjMatrixLoc = myShader->getUniformLocation("ProjMatrix");
		u_InitColorLoc = myShader->getUniformLocation("initColor");
		u_FinalColorLoc = myShader->getUniformLocation("finalColor");
		u_MaxAgeLoc = myShader->getUniformLocation("MaxAge");
		u_TimeLoc = myShader->getUniformLocation("Time");
		u_InitTranspLoc = myShader->getUniformLocation("initTransp");
		u_FinalTranspLoc = myShader->getUniformLocation("finalTransp");
		u_ColorIsTintLoc = myShader->getUniformLocation("ColorIsTint");
		u_ParticleSizeLoc = myShader->getUniformLocation("ParticleSize");
		u_ParticleTexLoc = myShader->getUniformLocation("ParticleTex");
		u_BrightnessLoc = myShader->getUniformLocation("Brightness");
		u_AccelLoc = myShader->getUniformLocation("Accel");
	}
	glm::mat4 ViewMatrix = myCam->getViewMatrix();
	glUniformMatrix4fv(u_ViewMatrixLoc, 1, GL_FALSE, &(ViewMatrix[0][0]));
	glm::mat4 ProjMatrix = myCam->getProjection();
	glUniformMatrix4fv(u_ProjMatrixLoc, 1, GL_FALSE, &(ProjMatrix[0][0]));
	glUniform1f(u_MaxAgeLoc, max_age);
	glUniform1f(u_TimeLoc, current_time);
	glUniform3f(u_InitColorLoc, initColor.x, initColor.y, initColor.z);
	glUniform3f(u_FinalColorLoc, finalColor.x, finalColor.y, finalColor.z);
	glUniform3f(u_BrightnessLoc, brightness.x, brightness.y, brightness.z);
	glUniform1f(u_InitTranspLoc, initTransparency);
	glUniform1f(u_FinalTranspLoc, finalTransparency);
	glUniform1f(u_ColorIsTintLoc, colorIsTint ? 1.0f : 0.0f);
	glUniform1f(u_ParticleSizeLoc, particleSize);
	glUniform1i(u_ParticleTexLoc, 0);
	glUniform3f(u_AccelLoc, Acceleration.x, Acceleration.y, Acceleration.z);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, numParticles);
	glBindVertexArray(0);
	glDisableVertexAttribArray(Attrib_initPos);
	glDisableVertexAttribArray(Attrib_initV);
	glDisableVertexAttribArray(Attrib_birthT);
}

bool ParticleRenderer::addParticle(bool totallyRandom, glm::vec3 initV, glm::vec3 initP) {
	if (isNull || particleData.size() < 7)
		return false;
	if (totallyRandom) {
		initP.x = RandomFloat(minX, maxX);
		initP.y = RandomFloat(minY, maxY);
		initP.z = RandomFloat(minZ, maxZ);

		initV.x = RandomFloat(minVX, maxVX);
		initV.y = RandomFloat(minVY, maxVY);
		initV.z = RandomFloat(minVZ, maxVZ);
	}
	float* p = &(particleData[0]);
	//~ std::cout << "\nWE'RE IN!" << std::endl;
	for (size_t i = 0; i + 6 < particleData.size(); i += 7) {
		float* p = &(particleData[i]);
		float t = (current_time - p[6]);
		if (p[6] > current_time || t > max_age) {
			p[0] = initP.x;
			p[1] = initP.y;
			p[2] = initP.z;

			p[3] = initV.x;
			p[4] = initV.y;
			p[5] = initV.z;

			p[6] = current_time;
			//~ std::cout << "\nSuccessfully added particle!" << std::endl;
			return true;
		}
	}
	// If it exits here, it failed to add the particle. Return false.
	return false;
}
void ParticleRenderer::resetTime() { // To avoid floating point error. Should be done every
									 // minute or so.
	float oldtime = current_time;
	current_time = 0;
	for (size_t i = 6; i < particleData.size(); i += 7)
		particleData[i] -= oldtime;
}
void ParticleRenderer::setMaxParticles(uint MaxParticles) {
	particleData.resize(MaxParticles * 7, std::numeric_limits<float>::max());
	if (!isNull) {
		glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[DATA_VBO]); // ONE. VBO!
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 7 * sizeof(GLfloat), &(particleData[0]), GL_DYNAMIC_DRAW);
	}
}
void ParticleRenderer::init(Shader* _Shad, SafeTexture _Tex, bool _WBOIT, uint MaxParticles) {
	if (!isNull)
		destruct();
	WBOIT = _WBOIT;
	myShader = _Shad;
	particleData.clear();
	myTexture = _Tex;
	//~ if (MaxParticles) {
	//~ particleData.resize(MaxParticles * 7, std::numeric_limits<float>::max());

	//~ } // HA!
	isNull = false;
	//~ for(size_t i = 6; i < particleData.size(); i += 7)
	//~ particleData[i] = std::numeric_limits<float>::max(); //Always going to
	// be greater than max_age, pretty certain.
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);
	glGenBuffers(NUM_PARTICLE_VBOS, m_VBOs);
	//~ glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[DATA_VBO]); // ONE. VBO!
	setMaxParticles(MaxParticles);
	// using 9 and 10 to avoid conflict with Mesh.
	glEnableVertexAttribArray(Attrib_initPos);
	glEnableVertexAttribArray(Attrib_initV);
	glEnableVertexAttribArray(Attrib_birthT);

	//                    #  			#floats    type     normalize
	//                    how many per cycle?	 offset
	glVertexAttribPointer(Attrib_initPos, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GL_FLOAT), 0);
	glVertexAttribDivisor(Attrib_initPos, 1); // once per instance.

	// Attrib_initV
	glVertexAttribPointer(Attrib_initV, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GL_FLOAT), (void*)(3 * sizeof(GL_FLOAT)));
	glVertexAttribDivisor(Attrib_initV, 1); // once per instance.
	//                    #  			#floats    type     normalize
	//                    how many per cycle?	 offset
	glVertexAttribPointer(Attrib_birthT, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(GL_FLOAT), (void*)(6 * sizeof(GL_FLOAT)));
	glVertexAttribDivisor(Attrib_birthT, 1); // once per instance.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
}; // namespace gekRender
