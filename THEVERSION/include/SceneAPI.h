#ifndef SceneAPI_H
#define SceneAPI_H

//(C) DMHSW 2018 All Rights Reserved
#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <map> //for std::map
#include <string>
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "FBO.h" //Mine
#include "GLRenderObject.h"
#include "IODevice.h"		  //Mine
#include "SafeTexture.h"	  //Mine
#include "Shader.h"			  //Benny
#include "camera.h"			  //Benny and Me
#include "mesh.h"			  //Mine... pretty much
#include "obj_loader.h"		  //Benny
#include "pointLight.h"		  //Mine
#include "resource_manager.h" //Mine
#include "texture.h"		  //Mostly me but somewhat benny
#include "transform.h"		  //Mine
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

/*
see 1Detailed Plan for Development[...].cpln (and now 11detailed plan) for more
information on this middleware project. Exciting stuff.

*/

// You interact with this class either by calling its configuration functions,
// draw functions, or registration functions You make the objects, this class
// will render them.

// Note that GkScene is AN IMPLEMENTATION of the other classes in this library.
// you can decide not to use the scene class at all and just use my other
// classes as a jumpstart on your OpenGL Project.
namespace gekRender {

struct GkCPURenderer {
  public:
	GkCPURenderer(uint _w, uint _h, uint _cw, uint _ch, float _scale = 1.0) {
		width = _w;
		height = _h;
		cpuwidth = _cw;
		cpuheight = _ch;
		cpucomp = 4;
		scale = _scale;
		glDisable(GL_CULL_FACE);  // Enable culling faces
		glDisable(GL_DEPTH_TEST); // test fragment depth when rendering.
		//~ glCullFace(GL_BACK); //cull faces with clockwise winding
		//~ glEnable(GL_ARB_conservative_depth); //conservative depth.

		// Generate texture
		std::vector<unsigned char> data_init(cpuwidth * cpuheight * cpucomp, (unsigned char)128);
		RenderingSurface = new Texture(cpuwidth, cpuheight, cpucomp, &data_init[0], GL_NEAREST, GL_NEAREST, GL_REPEAT, 0.0f);

		// Generate screenquad mesh
		if (!m_screenquad_Mesh) {
			// Set up the positions
			IndexedModel screenquad_IndexedModel;
			screenquad_IndexedModel.positions.push_back(glm::vec3(-1, -1, 0));
			screenquad_IndexedModel.positions.push_back(glm::vec3(-1, 1, 0));
			screenquad_IndexedModel.positions.push_back(glm::vec3(1, 1, 0));
			screenquad_IndexedModel.positions.push_back(glm::vec3(1, -1, 0));
			// Set up the index
			// First Triangle
			screenquad_IndexedModel.indices.push_back(0);
			screenquad_IndexedModel.indices.push_back(2);
			screenquad_IndexedModel.indices.push_back(3);
			// Second triangle
			screenquad_IndexedModel.indices.push_back(0);
			screenquad_IndexedModel.indices.push_back(1);
			screenquad_IndexedModel.indices.push_back(2);
			// Put in some bullshit for the texcoords and normals
			screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
			screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
			screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
			screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
			// We also need bullshit for the normals
			screenquad_IndexedModel.normals.push_back(glm::vec3(1, -1, 0));
			screenquad_IndexedModel.normals.push_back(glm::vec3(2, 1, 0));
			screenquad_IndexedModel.normals.push_back(glm::vec3(3, 1, 0));
			screenquad_IndexedModel.normals.push_back(glm::vec3(4, -1, 0));

			// std::cout<<"\n We got the indexed model done";
			// Make the mesh
			m_screenquad_Mesh = new Mesh(screenquad_IndexedModel, false, true, true);
			// std::cout<<"\n we got the mesh made";
			// Make the camera needed to render the screenquad
			Camera SQC = Camera();
			SQC.buildOrthogonal(-1, 1, -1, 1, 0, 1);
			SQC.pos = glm::vec3(0, 0, 0);
			SQC.forward = glm::vec3(0, 0, 1);
			SQC.up = glm::vec3(0, 1, 0);
			Screenquad_CameraMatrix = SQC.getViewProjection();
		}
	}

	void resizeSceneViewport(unsigned int newwidth, unsigned int newheight, float approxFactor = 1.0) {
		width = newwidth;
		height = newheight;
		scale = approxFactor;
	}
	~GkCPURenderer() {
		if (m_screenquad_Mesh)
			delete m_screenquad_Mesh;
		if (RenderingSurface)
			delete RenderingSurface;
	}
	void draw(Shader* currentShader) {
		FBO::unBindRenderTarget(width, height);
		FBO::clearTexture(0, 0, 0, 1);
		currentShader->bind();
		RenderingSurface->reinitFromDataPointer(true, true);
		// key does not exist, write to the spot!
		auto w2cloc = currentShader->getUniformLocation("World2Camera");
		auto tuloc = currentShader->getUniformLocation("diffuse");

		glUniformMatrix4fv(w2cloc, 1, GL_FALSE, &Screenquad_CameraMatrix[0][0]);
		glUniform1i(tuloc, 0);
		GLuint m_handle = m_screenquad_Mesh->getVAOHandle();
		glEnableVertexAttribArray(0);
		glBindVertexArray(m_handle);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glDisableVertexAttribArray(0);
	}
	Texture* getRenderingSurface() { return RenderingSurface; }
	void WritePixel(unsigned int x, unsigned int y, unsigned char r, unsigned char g, unsigned char b) {
		unsigned char* CPUFrameBufferData = RenderingSurface->getDataPointerNotConst();
		auto& CPUWIDTH = cpuwidth;
		auto& CPUHEIGHT = cpuheight;
		auto& CPUCOMPONENTS = cpucomp;
		CPUFrameBufferData[(y * CPUWIDTH + x) * CPUCOMPONENTS + 0] = r;
		CPUFrameBufferData[(y * CPUWIDTH + x) * CPUCOMPONENTS + 1] = g;
		CPUFrameBufferData[(y * CPUWIDTH + x) * CPUCOMPONENTS + 2] = b;
	}
	uint getWidth() { return width; }
	uint getHeight() { return height; }

  protected:
	uint width = 0, height = 0, cpuwidth = 0, cpuheight = 0, cpucomp = 4;
	uint scale = 1.0;
	glm::mat4 Screenquad_CameraMatrix;
	Mesh* m_screenquad_Mesh = nullptr;
	Texture* RenderingSurface = nullptr;
}; // Solely for if you don't want to have a scene, just want to render on the
   // CPU side.

class GkScene // Gk for gek
{
  public:
	GkScene(unsigned int newwidth = 640, unsigned int newheight = 480, float approxFactor = 1.0);
	GkScene(const GkScene& other) {
		std::cout << "\n The copy constructor of GkScene was called. A new GkScene "
					 "has been made for you which has config 640, 480, 1.0f.\nDO "
					 "NOT make this mistake again.";
		GkScene(640, 480, 1.0f);
	}
	virtual ~GkScene();

	// Draw Functions (NEEDS CULLING)
	void drawPipeline(int meshmask = -1, FBO* CurrentRenderTarget = nullptr, FBO* RenderTarget_Transparent = nullptr, Camera* CurrentRenderCamera = nullptr,
					  bool doFrameBufferChecks = false, glm::vec4 backgroundColor = glm::vec4(0, 0, 0, 0),
					  glm::vec2 fogRangeDescriptor = glm::vec2(10000, 15000), bool Render_Transparent = true);
	void drawShadowPipeline(int meshmask = -1, FBO* CurrentRenderTarget = nullptr, Camera* CurrentRenderCamera = nullptr, bool doFrameBufferChecks = false);
	// Custom rendering functions
	void (*customRenderingAfterTransparentObjectRendering)(int meshmask, FBO* CurrentRenderTarget, FBO* RenderTarget_Transparent, Camera* CurrentRenderCamera,
														   bool doFrameBufferChecks, glm::vec4 backgroundColor, glm::vec2 fogRangeDescriptor) = nullptr;
	void (*customMainShaderBinds)(int meshmask, FBO* CurrentRenderTarget, FBO* RenderTarget_Transparent, Camera* CurrentRenderCamera, bool doFrameBufferChecks,
								  glm::vec4 backgroundColor,
								  glm::vec2 fogRangeDescriptor) = nullptr; // For doing custom main shader binds
	void (*customRenderingAfterSkyboxBeforeMainShader)(int meshmask, FBO* CurrentRenderTarget, FBO* RenderTarget_Transparent, Camera* CurrentRenderCamera,
													   bool doFrameBufferChecks, glm::vec4 backgroundColor, glm::vec2 fogRangeDescriptor) = nullptr;

	// Engine Setting Utility Functions
	void resizeSceneViewport(unsigned int newwidth, unsigned int newheight,
							 float approxFactor = 1.0) { // Cases: Initialization, Resizing post Initializiation.
		width = newwidth;
		height = newheight;
		if (approxFactor > 0.0)
			Scene_FBOSizeFactor = approxFactor;
		// If we already have FBOs in the FBOarray, we need to get rid of
		// them(Handling case: Resizing after initialization)
		if (FboArray.size() > 0) {						 // if it has things in it
			for (size_t i = 0; i < FboArray.size(); i++) // for every thing
			{
				if (FboArray[i]) {
					delete FboArray[i]; // DOES call destructor
				}
			}
			FboArray.clear();
		}
		// FBO setups
		for (size_t i = 0; i < NUM_FORWARD_RENDERING_FBOS; i++)
			FboArray.push_back(nullptr); // push on some null pointers!
		FboArray[FORWARD_BUFFER1] = new FBO(int(width * Scene_FBOSizeFactor), int(height * Scene_FBOSizeFactor), 1,
											GL_RGBA8); // Non-Transparent
		FboArray[FORWARD_BUFFER2] = new FBO(int(width * Scene_FBOSizeFactor), int(height * Scene_FBOSizeFactor), 2, GL_RGBA16F,
											FboArray[FORWARD_BUFFER1]->getDepthBufferHandle()); // Transparent
		haveFoundLocationOfLightingUBO = false;
		HasntRunYet_Shadows = true;
		HasntRunYet = true;
		HasntRunYet_CompositionShader = true;
		haveInitializedSkyboxUniforms = false;
	}

	// std::find(vector.begin(), vector.end(), item)
	// Register custom FBO. Not sure we'll keep it but it's here.
	void registerCustomFBO(FBO* addme) {
		if (addme) {
			if (CustomFBOArray.size() > 0)
				for (size_t i = 0; i < CustomFBOArray.size(); i++) {
					if (addme == CustomFBOArray[i]) {
						return;
					}
				}
			CustomFBOArray.push_back(addme);
		}
	}
	void deregisterCustomFBO(FBO* unloveable) {
		// if (unloveable != nullptr && CustomFBOArray.size() > 0)
		// for (auto iter = (CustomFBOArray.end()--); iter >=
		// CustomFBOArray.begin(); --iter)
		// {
		// if (iter < CustomFBOArray.end())
		// if (*iter == unloveable)
		// CustomFBOArray.erase(iter);
		// }
		auto iter = std::find(CustomFBOArray.begin(), CustomFBOArray.end(), unloveable);
		if (iter != CustomFBOArray.end()) {
			CustomFBOArray.erase(iter);
			return;
		}
		return;
	}
	// Register
	// Meshes~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	void registerMesh(Mesh* assetboy) {
		if (assetboy) {
			Meshes.push_back(assetboy);
		}
	}
	bool deregisterMesh(Mesh* sadboy) {
		auto iter = std::find(Meshes.begin(), Meshes.end(), sadboy);
		if (iter != Meshes.end()) {
			Meshes.erase(iter);
			return true;
		}
		return false;
	}
	// Register
	// Lights~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	void registerPointLight(PointLight* brightboy) {
		if (brightboy) // We don't do duplication checking
			SimplePointLights.push_back(brightboy);
	}
	void deregisterPointLight(PointLight* removeme) {

		// if the given pointer points to the same
		if (SimplePointLights.size() > 0) {
			for (size_t i = 0; i < SimplePointLights.size(); i++) {
				if (removeme == SimplePointLights[i]) {						// does this thing point
																			// to the same object?
					SimplePointLights.erase(SimplePointLights.begin() + i); // It does, de-register it
					return;
				}
			}
		} else { /*std::cout << "\n There are apparently 0....";*/
		}
	}

	void registerAmbLight(AmbientLight* brightboy) {
		if (brightboy) // if this pointer is good...
		{
			AmbientLights.push_back(brightboy);
			// std::cout << "\n REGISTERED AMBIENT LIGHT!";
		}
	}
	void deregisterAmbLight(AmbientLight* removeme) {
		if (AmbientLights.size() > 0) {
			for (size_t i = 0; i < AmbientLights.size(); i++) {
				if (i < AmbientLights.size() && removeme == AmbientLights[i]) { // does this thing point to
																				// the same object?
					AmbientLights.erase(AmbientLights.begin() + i);				// It does, de-register it
					return;														// NOTE TO SELF: This is no longer necessary and
																				// creates a potential problem if the user
																				// accidentally registers a light twice and wants to
																				// remove it. Only keeping it, and this note, to
																				// remind me to change the others.
				}
			}
		} else { /*std::cout << "\n There are apparently 0....";*/
		}
	}

	void registerCamLight(CameraLight* brightboy) {
		if (brightboy) // if this pointer is good...
			CameraLights.push_back(brightboy);
	}
	void deregisterCamLight(CameraLight* removeme) {
		if (CameraLights.size() > 0) {
			for (size_t i = 0; i < CameraLights.size(); i++) {
				if (removeme == CameraLights[i]) {				  // does this thing point to
																  // the same object?
					CameraLights.erase(CameraLights.begin() + i); // It does, de-register it
					return;										  // Since we have a check in place whenever a point
																  // light is registered, we know it is the only one.
																  // if it isn't, then this function can be called
																  // multiple times... I guess...
				}
			}
		} else { /*std::cout << "\n There are apparently 0....";*/
		}
	}

	void registerDirLight(DirectionalLight* brightboy) {
		if (brightboy) // if this pointer is good...
			DirectionalLights.push_back(brightboy);
	}
	void deregisterDirLight(DirectionalLight* removeme) {
		if (DirectionalLights.size() > 0) {
			for (size_t i = 0; i < DirectionalLights.size(); i++) {
				if (removeme == DirectionalLights[i]) {						// does this thing point
																			// to the same object?
					DirectionalLights.erase(DirectionalLights.begin() + i); // It does, de-register it
					return;													// Since we have a check in place whenever a point
																			// light is registered, we know it is the only one.
																			// if it isn't, then this function can be called
																			// multiple times... I guess...
				}
			}
		} else { /*std::cout << "\n There are apparently 0....";*/
		}
	}

	// Engine Setters and Getters
	void setSceneCamera(Camera* MyEye) { SceneCamera = MyEye; }
	Camera* getSceneCamera() { return SceneCamera; }
	void setMainShader(Shader* inshader) { // NOTE: We should set HasntRunYet
		if (inshader != nullptr)
			MainShader = inshader;
		haveFoundLocationOfLightingUBO = false;
		HasntRunYet_Shadows = true;
		HasntRunYet = true;
		HasntRunYet_CompositionShader = true;
		haveInitializedSkyboxUniforms = false;
	}
	void setSkyBoxCubemap(CubeMap* _SkyBoxCubeMap) {
		if (_SkyBoxCubeMap != nullptr) // if it is not nullptr
		{
			SkyBoxCubemap = _SkyBoxCubeMap;
		}
	}
	CubeMap* getSkyBoxCubemap() { return SkyBoxCubemap; }
	void setSkyboxShader(Shader* _SkyboxShader) {
		if (_SkyboxShader)
			SkyboxShader = _SkyboxShader;
		haveFoundLocationOfLightingUBO = false;
		HasntRunYet_Shadows = true;
		HasntRunYet = true;
		HasntRunYet_CompositionShader = true;
		haveInitializedSkyboxUniforms = false;
	}
	void setWBOITCompositionShader(Shader* _CompositionShader) {
		if (_CompositionShader)
			CompositionShader = _CompositionShader;
		haveFoundLocationOfLightingUBO = false;
		HasntRunYet_Shadows = true;
		HasntRunYet = true;
		HasntRunYet_CompositionShader = true;
		haveInitializedSkyboxUniforms = false;
	}
	/*
	Shader Registration

	This is where you submit a shader!
	*/
	int getWidth() { return width; }
	int getHeight() { return height; }

	//~ void enableShadows(){doshadows = true;}
	//~ void disableShadows(){doshadows = false;}

	//~ void enableShadowFiltering(){doshadowfiltering = true;}
	//~ void disableShadowFiltering(){doshadowfiltering = false;}

	bool ImDeletingThisShaderOKThanks(Shader* currentShader) { // If you delete a shader and then make another,
															   // and both were used in screenquadtoFBO, there is
															   // a chance that it will be allocated in the exact
															   // same memory location... A really good chance.
															   // So make sure that we delete any info we have on
															   // the shader in the std::maps before proceeding.
		bool did_deregister = false;

		auto iter = World2CameraLocations.find(currentShader); // Auto is pretty cool but I don't like it... So
															   // just remember it's an iterator.
		if (iter != World2CameraLocations.end()) {
			// key does not exist, write to the spot!
			World2CameraLocations.erase(iter);
			did_deregister = true;
		}
		auto iteragain = TextureUnitLocations.find(currentShader);
		if (iteragain != TextureUnitLocations.end()) {
			TextureUnitLocations.erase(iteragain);
			did_deregister = true;
		}
		haveFoundLocationOfLightingUBO = false;
		HasntRunYet_Shadows = true;
		HasntRunYet = true;
		HasntRunYet_CompositionShader = true;
		haveInitializedSkyboxUniforms = false;
		return did_deregister;
	}

	bool fastAndSafeDeregistration(void* LiterallyAPointerToAnything = nullptr, GLuint TESTHANDLE = 0) {
		/*
				//meshes
				std::vector<Mesh*> Meshes;
				//lights
				std::vector<PointLight*> SimplePointLights;
				std::vector<DirectionalLight*> DirectionalLights;
				//std::vector<Spotlight*> Spotlights;
				std::vector<AmbientLight*> AmbientLights;
				std::vector<CameraLight*> CameraLights;
				//Shaders
				Shader* MainShader = nullptr;
				Shader* SkyboxShader = nullptr;
				Shader* CompositionShader = nullptr; //Used solely for
		   compositing the Weighted Blended OIT initial pass onto the final
		   image Camera* SceneCamera = nullptr;//The SceneRender Camera pointer
		   Camera* ScreenquadCamera = nullptr; //Unnecessary? Maybe... CubeMap*
		   SkyBoxCubemap = nullptr; //For the skybox
				
				std::vector<FBO*> FboArray; //vector and not normal array
		   because reasons std::vector<FBO*> CustomFBOArray; //For render-target
		   operations. Will be used for reflections in the future or something.
		*/
		bool did_deregister = false;
		// Make sure it isn't a light
		deregisterAmbLight((AmbientLight*)LiterallyAPointerToAnything);
		deregisterCamLight((CameraLight*)LiterallyAPointerToAnything);
		deregisterDirLight((DirectionalLight*)LiterallyAPointerToAnything);
		deregisterPointLight((PointLight*)LiterallyAPointerToAnything);
		// Make sure it isn't a camera
		if ((void*)SceneCamera == LiterallyAPointerToAnything) {
			SceneCamera = nullptr;
			did_deregister = true;
		}
		// Make sure it isn't a shader... and get rid of any info we have on it
		if ((void*)MainShader == LiterallyAPointerToAnything) {
			MainShader = nullptr;
			did_deregister = true;
		}
		if ((void*)SkyboxShader == LiterallyAPointerToAnything) {
			SkyboxShader = nullptr;
			did_deregister = true;
		}
		if ((void*)CompositionShader == LiterallyAPointerToAnything) {
			CompositionShader = nullptr;
			did_deregister = true;
		}
		did_deregister = ImDeletingThisShaderOKThanks((Shader*)LiterallyAPointerToAnything) ? true : did_deregister;
		// Make sure it isn't a mesh
		deregisterMesh((Mesh*)LiterallyAPointerToAnything); // Bug: Wont tell you if you
															// deregistered a mesh
		//...Or a meshinstance in one of the meshes.
		for (size_t i = 0; i < Meshes.size(); i++) {
			Meshes[i]->deregisterInstance((MeshInstance*)LiterallyAPointerToAnything);
			//...Or a Texture/Cubemap in one of the meshes...
			did_deregister = Meshes[i]->removeCubeMap((CubeMap*)LiterallyAPointerToAnything) ? true : did_deregister;
			did_deregister = Meshes[i]->removeTexture(TESTHANDLE) ? true : did_deregister;
		}

		//...Or the Cubemap we use for the skybox
		if ((void*)SkyBoxCubemap == LiterallyAPointerToAnything) {
			SkyBoxCubemap = nullptr;
			did_deregister = true;
		}
		//...Or an FBO
		haveFoundLocationOfLightingUBO = false;
		HasntRunYet_Shadows = true;
		HasntRunYet = true;
		HasntRunYet_CompositionShader = true;
		haveInitializedSkyboxUniforms = false;
		return did_deregister;
	}

	// Public drawing code- needed for testing
	void screenquadtoFBO(Shader* currentShader) {
		// For caching uniform locations.
		// static std::map<Shader*, GLuint> World2CameraLocations;
		// static std::map<Shader*, GLuint> TextureUnitLocations; //This will be
		// added soon... or I will implement it manually
		// Write World2Camera glGetUniformLocation location. NOTE: My first time
		// using auto in C++.
		auto iter = World2CameraLocations.find(currentShader); // Auto is pretty cool but I don't like it... So
															   // just remember it's an iterator.
		if (iter == World2CameraLocations.end()) {
			// key does not exist, write to the spot!
			World2CameraLocations[currentShader] = currentShader->getUniformLocation("World2Camera");
		}
		auto iteragain = TextureUnitLocations.find(currentShader);
		if (iteragain == TextureUnitLocations.end()) {
			TextureUnitLocations[currentShader] = currentShader->getUniformLocation("diffuse");
		}
		//
		glUniformMatrix4fv(World2CameraLocations[currentShader], 1, GL_FALSE, &Screenquad_CameraMatrix[0][0]);
		glUniform1i(TextureUnitLocations[currentShader], 0);
		GLuint m_handle = m_screenquad_Mesh->getVAOHandle();
		// glEnableVertexAttribArray(0);
		glBindVertexArray(m_handle);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// glBindVertexArray(0);
		// glDisableVertexAttribArray(0);
	}

	std::vector<Mesh*>* getMeshVectorPtr() { return &Meshes; }
	std::vector<PointLight*>* getPointLightVectorPtr() { return &SimplePointLights; }
	std::vector<DirectionalLight*>* getDirectionalLightVectorPtr() { return &DirectionalLights; }
	std::vector<AmbientLight*>* getAmbientLightVectorPtr() { return &AmbientLights; }
	std::vector<CameraLight*>* getCameraLightVectorPtr() { return &CameraLights; }
	std::vector<FBO*>* getFBOVectorPtr() { return &CustomFBOArray; }

	// Public Variables
	// IODevice* globalDevicePointer = nullptr; //This shouldn't be necessary,
	// but hey it's here if I need it.
	Shader* ShowTextureShader = nullptr;	  // TODO: Make getters and setters for
											  // this
	Shader* ShadowOpaqueMainShader = nullptr; // TODO: Make getters and setters for this too
  protected:
	// I dunno
  private: /*
	  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  PRIVATE VARIABLES
	  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  */
	//~ GkScene(const GkScene& other);
	void operator=(const GkScene& other);
	void organizeUBOforUpload(Camera* SceneCam); // Assumes that the mainshader is currently bound
	std::map<Shader*, GLuint> World2CameraLocations;
	std::map<Shader*, GLuint> TextureUnitLocations;
	// Toggles and engine settings
	bool HasntRunYet = true;				   // default value.
	bool HasntRunYet_CompositionShader = true; // Ditto!
	bool HasntRunYet_Shadows = true;
	bool doshadows = true;
	glm::vec2 screensize = glm::vec2();
	unsigned int width = 640;						// Refers to final pass.
	unsigned int height = 480;						// ditto
	unsigned int LightDepthFBODimension = 1024;		// What should the square dimension of the Depth FBO be? It should
													// be higher than the SceneRender screen dimensions.
	float Dual_Lighting_Approximation_Factor = 1.0; // what fraction of the screen resolution should the cumulation
													// buffers be?
	float Scene_FBOSizeFactor = 1.0;
	// Allocated space for storing stuff so OpenGL can reach it on a draw call
	// (because testing seems to show opengl doesn't actually grab uniform data
	// until a draw call happens...)
	glm::mat4 SceneRenderCameraMatrix = glm::mat4();		   // World -> NDC
	glm::mat4 SceneRenderCameraViewMatrix = glm::mat4();	   // Holds the glm::lookat matrix. World --> View
	glm::mat4 SceneRenderCameraProjectionMatrix = glm::mat4(); // Projection matrix.

	glm::mat4 InverseViewProjectionMatrix = glm::mat4(); // Inverse of the World->NDC matrix.
	glm::mat4 InverseProjectionMatrix = glm::mat4();	 // Inverse of Projection matrix.
	glm::mat4 InverseViewMatrix = glm::mat4();

	glm::mat4 Screenquad_CameraMatrix = glm::mat4();	   // Holds the model --> World transform
	glm::mat4 Skybox_Transitional_Transform = glm::mat4(); // Holds the model-->world transform for the skybox. We
														   // need to scale it, remember?
	// Things we plan on rendering
	std::vector<Mesh*> Meshes;
	std::vector<PointLight*> SimplePointLights;
	std::vector<DirectionalLight*> DirectionalLights;
	std::vector<AmbientLight*> AmbientLights;
	std::vector<CameraLight*> CameraLights;
	// Shaders
	Shader* MainShader = nullptr;
	Shader* SkyboxShader = nullptr;
	Shader* CompositionShader = nullptr; // Used solely for compositing the Weighted Blended OIT initial
										 // pass onto the final image
	enum { OIT_COMP_1, OIT_COMP_2, OIT_NUM_OIT_COMP_UNIFORMS };
	GLuint CompositionShaderUniforms[OIT_NUM_OIT_COMP_UNIFORMS]; // Only for handling the 2 texture units...
	Camera* SceneCamera = nullptr;								 // The SceneRender Camera pointer
	// std::vector<glm::mat4*> RendertargetCameras;//For Render-Target Renders
	bool nextRenderPipelineCallWillBeAfterVsync = true;
	//~ Camera* ScreenquadCamera = nullptr; //Unnecessary? Maybe...
	CubeMap* SkyBoxCubemap = nullptr; // For the skybox

	enum {								// THESE NEED TO BE RENAMED!
										// material
		MAINSHADER_DIFFUSE,				// Texture Unit for the literal texture of a surface
		MAINSHADER_SPECREFLECTIVITY,	// float reflectivity of material
		MAINSHADER_SPECDAMP,			// float dampening for specular component of
										// material
		MAINSHADER_DIFFUSIVITY,			// float How diffuse is this material?
		MAINSHADER_ENABLE_TRANSPARENCY, // float Should we enable transparency
		MAINSHADER_EMISSIVITY,
		MAINSHADER_IS_INSTANCED,
		// Matrices
		MAINSHADER_WORLD2CAMERA,  // 4x4 convert from world space to NDC
		MAINSHADER_MODEL2WORLD,   // 4x4 convert from model to world
		MAINSHADER_RENDERFLAGS,   // Gk Renderflags as specified in the OBJ file.
		MAINSHADER_WORLDAROUNDME, // The cubemap we use for reflections
		MAINSHADER_CAMERAPOS,	 // For calculating Cubemap Reflect
		MAINSHADER_ENABLE_CUBEMAP_REFLECTIONS,
		MAINSHADER_ENABLE_CUBEMAP_DIFFUSION,
		MAINSHADER_TEXOFFSET,
		MAINSHADER_NUM_POINTLIGHTS,
		MAINSHADER_NUM_DIRLIGHTS,
		MAINSHADER_NUM_AMBLIGHTS,
		MAINSHADER_NUM_CAMLIGHTS,
		MAINSHADER_SKYBOX_CUBEMAP,   // The cubemap of the Skybox
		MAINSHADER_BACKGROUND_COLOR, // if a is 0, it uses the skybox cubemap,
									 // if a is 1, it uses the background color
		MAINSHADER_FOG_RANGE,		 // What distance from the camera should fog start
									 // at and go to 100% at?
		MAINSHADER_CAMERATEX1,
		MAINSHADER_CAMERATEX2,
		MAINSHADER_CAMERATEX3,
		MAINSHADER_CAMERATEX4,
		MAINSHADER_CAMERATEX5,
		MAINSHADER_CAMERATEX6,
		MAINSHADER_CAMERATEX7,
		MAINSHADER_CAMERATEX8,
		MAINSHADER_CAMERATEX9,
		MAINSHADER_CAMERATEX10,
		MAINSHADER_BONE_UBO_LOCATION,
		MAINSHADER_HAS_BONES,
		MAINSHADER_NUM_SceneRender_SHADER_UNIFORMS // Self explanatory
	};
	enum {
		MAINSHADER_SHADOWS_IS_INSTANCED,
		MAINSHADER_SHADOWS_WORLD2CAMERA,
		MAINSHADER_SHADOWS_MODEL2WORLD,
		MAINSHADER_SHADOWS_DUMMY, // If we are worried that the mesh class will
								  // accidentally do something stupid, we send
								  // it this dummy!
		MAINSHADER_SHADOWS_NUM_MAINSHADER_SHADOWS_UNIFORMS
	};
	enum {
		// Matrices
		SKYBOX_WORLD2CAMERA, // 4x4 convert from world space to NDC
		SKYBOX_MODEL2WORLD,  // 4x4 convert from model to world
		SKYBOX_VIEWMATRIX,   // Result of glm::lookat, used for finding the
							 // camera's position
		SKYBOX_WINDOWx,
		SKYBOX_WINDOWy,
		SKYBOX_PROJECTION,

		SKYBOX_WORLDAROUNDME,			  // The cubemap we use for reflections
		SKYBOX_CAMERAPOS,				  // For calculating Cubemap Reflect
		SKYBOX_NUM_SKYBOX_SHADER_UNIFORMS // Self explanatory
	};
	GLuint MainShaderUniforms[MAINSHADER_NUM_SceneRender_SHADER_UNIFORMS];

	GLuint MainShaderUniforms_SHADOWTEMP[MAINSHADER_NUM_SceneRender_SHADER_UNIFORMS];

	GLuint MainShaderShadowUniforms[MAINSHADER_SHADOWS_NUM_MAINSHADER_SHADOWS_UNIFORMS];

	// these are no longer used because we don't use uniforms anymore- we use a
	// uniform buffer object ~ std::vector<GLuint> m_LightUniformHandles;//See
	// SceneAPI.cpp for format ~ std::vector<GLuint>
	// m_LightUniformHandles_SHADOWTEMP;//See SceneAPI.cpp for format ~
	// std::vector<GLuint> m_LightClippingVolumeUniformHandles;//See
	// SceneAPI.cpp for format or look below ~ std::vector<GLuint>
	// m_LightClippingVolumeUniformHandles_SHADOWTEMP;//Used for transitioning
	// to the shadow system for drawshadowpipeline function
	char LightingDataUBOData[16000];			 // maximum of 16k in size to be compatible
												 // with old systems
	GLuint LightingDataUBO = 0;					 // handle for the lighting data UBO BUFFER
	GLuint LightingDataUBOLocation = 0;			 // handle for the UNIFORM BUFFER OBJECT in the shader
	const GLuint LightingDataUBOBindPoint = 0;   // what we give to BindBufferBase
	const GLuint BoneDataUBOBindPoint = 1;		 // ditto, but for BONES
	bool haveCreatedLightingUBO = false;		 // have we made it?
	bool haveFoundLocationOfLightingUBO = false; // have we found the location?
												 // FORMAT OF m_LightClippingVolumeUniformHandles
	// Per Light:
	// 0 - 3 AABBp1-4
	// 4-7 Spheres 0-3
	// 8 Blacklist or Whitelist UINT
	GLuint SkyboxUniforms[SKYBOX_NUM_SKYBOX_SHADER_UNIFORMS]; // fuck pointers
	bool haveInitializedSkyboxUniforms = false;
	enum { FORWARD_BUFFER1, FORWARD_BUFFER2, FORWARD_BUFFER3, NUM_FORWARD_RENDERING_FBOS };
	std::vector<FBO*> FboArray;		  // vector and not normal array because reasons
	std::vector<FBO*> CustomFBOArray; // For render-target operations. Will be used for
									  // reflections in the future or something.

	// Screenquad Variables
	//~ IndexedModel screenquad_IndexedModel;
	Mesh* m_screenquad_Mesh = nullptr;

	// Skybox Geometry
	IndexedModel skyboxModel;
	Mesh* m_skybox_Mesh = nullptr;
	Transform skybox_transform = Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
};
};	 // namespace gekRender
#endif // SceneAPI_H
