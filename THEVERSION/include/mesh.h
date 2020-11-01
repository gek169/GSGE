#ifndef MESH_H
#define MESH_H

#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <cstdlib>
#include <glm/glm.hpp>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
//#include <list> //std::list
#include "SafeTexture.h"
#include "Shader.h"
#include "camera.h"
#include "obj_loader.h"
#include "transform.h"
#include <GLFW/glfw3.h>

namespace gekRender {

inline float RandomFloat(float a, float b) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

struct Phong_Material {
  public:
	Phong_Material(float amb, float diff, float specr, float specd, float emiss) {
		ambient = amb;
		specreflectivity = specr;
		specdamp = specd;
		emissivity = emiss;
		diffusivity = diff;
	}
	Phong_Material() {} // Default Constructor
	float diffusivity = 1.0;
	float ambient = 1;
	float specreflectivity = 0.4;
	float specdamp = 30;
	float emissivity = 0;
};

// getting rid of RenderableMesh one step at a time!
struct MeshInstance {
  public:
	MeshInstance(int _texind, Transform _transform);
	MeshInstance();

	// GLuint BoneUBO = 0;
	// std::vector<GKBone> Skeleton; //Doesn't rely on pointers!!! Hooray!!!
	// std::vector<GLfloat> SkeletonVBOLoader; //Will be size 0 unless it's
	// expanded, so it's cool bro

	~MeshInstance();

	MeshInstance(const MeshInstance& Other);

	float EnableCubemapReflections = 0; // Chrome reflections
	float EnableCubemapDiffusion = 0;   // A lightmap (Thanks, Brian Hook Speech)
	unsigned int tex = 0;
	glm::vec2 texOffset = glm::vec2(0,0);
	unsigned int cubeMap = 0;
	Transform myTransform = Transform(); // We have a default constructor
	void* userPointer = nullptr;		 // points to the user's small and effeminate pickle jar
	Phong_Material myPhong = Phong_Material();
	// Transitional is no longer needed because we now use a cache in Mesh.
	// glm::mat4 Transitional = glm::mat4(); //We need this for the renderer in
	// NON-INSTANCED mode.
	int mymeshmask = -1; // Always Render is 1, if not 1, meshmask is valid IF
						 // it is divisible by the renderer's mask with no
						 // remainder, so modulo comes out as 0
	bool shouldRender = true;
	bool culled = false;
};

class Mesh {
  public:
	Mesh(std::string fileName, bool instanced, bool is_static, bool assetjaodernein, bool recalculatenormals = false);
	Mesh();
	Mesh(const Mesh& Other);
	Mesh(IndexedModel usethisone, bool instancing, bool shallwemakeitstatic, bool assetjaodernein);
	void optimizeCacheMemoryUsage(bool ShrinkToFit = true);
	void setFlags(unsigned int input);
	unsigned int getFlags();
	void registerInstance(MeshInstance* Doggo);
	bool deregisterInstance(MeshInstance* Doggo);
	void drawGeneric();
	void drawInstancesPhong(GLuint Model, GLuint rflags, GLuint specr, GLuint specd, GLuint diff, GLuint emiss, GLuint enableCubemaps,
							GLuint enableCubemaps_diffuse, GLuint EnableTransparency, GLuint EnableInstancing, GLuint BoneUBOLocation, GLuint BoneUBOBindPoint,
							GLuint HasBones, GLuint texOffsetLocation, Shader* Shadman, bool Transparency, bool toRenderTarget, int meshmask, bool usePhong = true,
							bool donotusemaps = false, bool force_non_instanced = false);
	void pushTexture(SafeTexture _tex);
	void pushTexture(Texture* _tex){pushTexture(SafeTexture(_tex));}
	bool removeTexture(GLuint _tex);
	void pushCubeMap(CubeMap* _cube);
	bool removeCubeMap(GLuint _cube);
	bool removeCubeMap(CubeMap* TheOneTheOnly);
	// for manually testing the vector to see which element is what texture. You
	// would essentially want to use this to debug how many safetextures you
	// have, and pick suitable elements.
	std::vector<SafeTexture>* getTextureVectorPtr();
	std::vector<CubeMap*>* getCubeMapVectorPtr();
	std::vector<MeshInstance*>* getInstanceVectorPtr();
	int numTextures();
	int numInstances();

	GLuint getVAOHandle();
	unsigned int getDrawCount();
	IndexedModel getShape();
	virtual ~Mesh();
	std::string MyName = ""; // Stores the filename. There's a small problem
							 // though... Capitalization... Yeah. It's big.

	bool is_instanced = false; // default is false.
	bool is_static = true;	 // default is true.
	bool is_asset = true;	  // default is true.

	Phong_Material InstancedMaterial;				   // Only used if this mesh has instancing enabled
	glm::vec2 instanced_texOffset = glm::vec2(0,0);
	bool instanced_enable_cubemap_reflections = false; // Only used if this mesh has instancing enabled.
	bool instanced_enable_cubemap_diffusion = false;

	void reShapeMesh(const IndexedModel& model);
	int mesh_meshmask = -1; // See explanation in the Instance class
	bool useMapping = true;
	bool forceNoImplicitSync = false;
	uint renderflags = 0; // Flags for this mesh by default.
  protected:
	void initMesh(IndexedModel model, bool instancing, bool shallwemakeitstatic);

	enum {
		POSITION_VB,
		TEXCOORD_VB,
		INDEX_VB,
		NORMAL_VB,
		COLOR_VB,
		// INSTANCELOC_VB, //Uncomment these before doing instancing.
		// INSTANCECUSTOM_VB,
		NUM_BUFFERS
	};
	// Uniform Locations

	GLuint m_vertexArrayObject = 0;
	GLuint m_vertexArrayBuffers[NUM_BUFFERS];
	// used for instancing. Only the first texture and cubemap are used in
	// instancing mode.

	GLuint InstanceModelMatrixVBO;
	uint InstancedMatrixSizeLastFrame = 0;
	// Indicate the length of the VBO, in 16 * sizeof(float) and vec4
	// respectively. Transparency toggle does not need to be set, because all
	// instances will use the same texture and therefore will all either be
	// transparent or not.
	// size_t lengthof_InstanceModelMatrixVBO = 0; //Default 0
	// Measured in number of floats

	// Used for sending in per-instance info. Note that we will only expand this
	// when we need to, no clear calls should be necessary. Send in as columns!
	// This one is used for Non-Instanced mode to store the Mat4s
	// std::vector<glm::mat4*> MatrixDataCache; //When we used this for
	// instancing, it was GLfloat.
	std::vector<GLfloat> MatrixDataCacheMalloced; // Std vector will manage resizing for us... NO
												  // LONGER MALLOCED SO IT SHOULDNT BE CALLED
												  // THAT!!!

	bool shouldremakeinstancedmodelvector = true;
	bool canbereshaped = true;
	uint m_instanceDrawCount = 0; // Number of instances. Used for instancing!
	uint m_drawCount = 0;
	
	bool isnull = true;

	// The other booleans are up above, in public.
	IndexedModel ModelData;

	std::vector<MeshInstance*> Instances; // Vector of meshinstance pointers. To be used when we get
										  // rid of that DARN renderablemesh class. If this is a
										  // dynamic, non-static mesh then the typical use-case will be
										  // to only have one instance.
	std::vector<SafeTexture> MyTextures;  // To be used in the initial opaque shader. Maximum 8 for
										  // instanced situations.
	std::vector<CubeMap*> myCubeMaps;	 // To be used in the initial opaque shader.
										  // Maximum 8 for instanced situations.
};

class ParticleRenderer { // Requires instancing support. Draws a FUCKton of
						 // particles.
  public:
	ParticleRenderer() {
		isNull = true;
		for (size_t i = 0; i < NUM_PARTICLE_VBOS; i++)
			m_VBOs[i] = 0;
	}
	~ParticleRenderer() { destruct(); }
	SafeTexture myTexture;
	Shader* myShader = nullptr; // REMEMBER that whenever you change the shader you MUST
	// Set haveFoundUniformLocations = false;

	void DrawParticles(Camera* myCam); // Renders particles to the currently bound framebuffer.
	void init(Shader* _Shad, SafeTexture _Tex, bool _WBOIT, uint MaxParticles = 10000);

	void destruct() {
		if (!isNull) {
			glDeleteBuffers(NUM_PARTICLE_VBOS, m_VBOs);
			glDeleteVertexArrays(1, &m_VAO);
		}
		//~ if(myShader) delete myShader;
		myShader = nullptr;
		particleData.clear();
	}
	bool addParticle(bool totallyRandom = true, glm::vec3 initV = glm::vec3(0, 0, 0), glm::vec3 initP = glm::vec3(0, 0, 0));
	void setMaxParticles(uint newMaxParticles);
	void colorTints() { colorIsTint = true; }
	void colorMultiplies() { colorIsTint = false; }
	void tickTime(float _t) { current_time += _t; }
	void resetTime();
	// AABB for particles to spawn inside of.
	float minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
	// Init Velocity Limits
	float minVX = 0, maxVX = 0, minVY = 0, maxVY = 0, minVZ = 0, maxVZ = 0;

	std::vector<float> particleData;			 // Initpos, Initvel, birthtime.  (x,y,z,
												 // x,y,z, t). 7 floats.
	glm::vec3 Acceleration = glm::vec3(0, 0, 0); // Default: No acceleration.
	glm::vec3 initColor = glm::vec3(0, 0, 0), finalColor = glm::vec3(0, 0, 0);

	bool shouldRender = true;
	bool colorIsTint = true;
	bool WBOIT = true;
	bool forceNoImplicitSync = false;
	bool haveFoundUniformLocations = false;
	float max_age = 5; // seconds, presumably.
	float particleSize = 1;
	float current_time = 0;
	// Lighting on this particle system.
	// Literally only diffuse.
	// Just multiplies...
	glm::vec3 brightness = glm::vec3(1, 1, 1);
	float initTransparency = 1;
	float finalTransparency = 0;
	bool AutoAddParticles = true;

  protected:
	ParticleRenderer(const ParticleRenderer& other) {}
	void operator=(const ParticleRenderer& other) {}
	bool isNull = true;
	// Uniform locations.
	GLuint u_ViewMatrixLoc = 0;
	GLuint u_ProjMatrixLoc = 0;
	GLuint u_InitColorLoc = 0;
	GLuint u_FinalColorLoc = 0;
	GLuint u_MaxAgeLoc = 0;
	GLuint u_TimeLoc = 0;
	GLuint u_InitTranspLoc = 0;
	GLuint u_FinalTranspLoc = 0;
	GLuint u_ColorIsTintLoc = 0;
	GLuint u_ParticleSizeLoc = 0;
	GLuint u_ParticleTexLoc = 0;
	GLuint u_BrightnessLoc = 0;
	GLuint u_AccelLoc = 0;
	GLuint m_VAO = 0;
	const GLuint Attrib_initPos = 9;
	const GLuint Attrib_initV = 10;
	const GLuint Attrib_birthT = 11;
	enum { DATA_VBO, NUM_PARTICLE_VBOS };
	GLuint m_VBOs[NUM_PARTICLE_VBOS];
};

};	 // namespace gekRender
#endif // Mesh_H
