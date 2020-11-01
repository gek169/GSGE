#ifndef OBJ_LOADER_H_INCLUDED
#define OBJ_LOADER_H_INCLUDED

//(C) DMHSW 2018 All Rights Reserved
#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "transform.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
namespace gekRender {
typedef unsigned int uint;
// An extremely lightweight, easy to read, 100% functional OBJ Loader
// It does not handle materials, and if presented with a file with multiple
// objects in it, will only use the first one.
static const unsigned int GK_RENDER = 1;								 // Do we render it? This is perhaps the most important flag.
static const unsigned int GK_TEXTURED = 2;								 // Do we texture it? if disabled, only the texture will be used. if both
																		 // this and colored are disabled, the object will be black.
static const unsigned int GK_COLORED = 4;								 // Do we color it? if disabled, only the texture will be used. if both
																		 // this and textured are disabled, the object will be black.
static const unsigned int GK_FLAT_NORMAL = 8;							 // Do we use flat normals? If this is set, then the normals output to the
																		 // fragment shader in the initial opaque pass will use the flat layout
																		 // qualifier.
static const unsigned int GK_FLAT_COLOR = 16;							 // Do we render flat colors? the final, provoking vertex will be used as
																		 // the color for the entire triangle.
static const unsigned int GK_COLOR_IS_BASE = 32;						 // Use the color as the primary. Uses texture as primary if disabled.
static const unsigned int GK_TINT = 64;									 // Does secondary add to primary?
static const unsigned int GK_DARKEN = 128;								 // Does secondary subtract from primary?
static const unsigned int GK_AVERAGE = 256;								 // Do secondary and primary just get averaged?
static const unsigned int GK_ENABLE_ALPHA_CULLING = 4096;				 // Do we use the texture alpha to cull alpha fragments
static const unsigned int GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR = 8192; // This one is a bit weird. It was made so that if you have a model
																		 // with per-vertex color, that you could overlay a texture on top of
																		 // it. Check out FORWARD_MAINSHADER.fs in the shaders folder if you
																		 // want to check that out.
static const unsigned int GK_BONE_ANIMATED = 16384;						 // Do we apply bone animation?
inline long long GkAtoi(const char* str) { return strtoll(str, nullptr, 10); }
inline unsigned long long GkAtoui(const char* str) { return strtoull(str, nullptr, 10); }
inline double GkAtof(const char* str) { return strtod(str, nullptr); }
inline bool floatApproxEqual(float f1, float f2, float range) { return ((f2 + range) > f1 && (f2 - range) < f1); }
inline bool vec3ApproxEqual(glm::vec3 f1, glm::vec3 f2, float range) {
	return ((f2.x + range) > f1.x && (f2.x - range) < f1.x) && ((f2.y + range) > f1.y && (f2.y - range) < f1.y) &&
		   ((f2.z + range) > f1.z && (f2.z - range) < f1.z);
}

struct OBJIndex // One of those groups of slashes in an F line.
{
	unsigned int vertexIndex;
	unsigned int uvIndex;
	unsigned int normalIndex;
	unsigned int vertColorIndex; // my custom property

	bool operator<(const OBJIndex& r) const { return vertexIndex < r.vertexIndex; }
};

static Transform getRelativeTransform(Transform parent, Transform child) {
	// Find the transformation FROM parent TO child.
	Transform r;
	r.setModel(glm::inverse(parent.getModel()) * child.getModel());
	return r;
}

struct GKBone {
	std::string name = "";
	// Initialized to the identity
	glm::mat4 animatedTransform = glm::mat4(1.0f);
	glm::mat4 localAnimatedTransform = glm::mat4(1.0f);

	int index = -1; // Should never be greater than 49
	std::vector<int> Children;
	GKBone(const GKBone& other) {
		Children = other.Children;
		animatedTransform = other.animatedTransform;
		localAnimatedTransform = other.localAnimatedTransform;
		index = other.index;
		name = other.name;
	}
	GKBone(std::string _name, int _index, glm::mat4 _localBindTransform) {
		name = _name;
		index = _index;
		if (index > 100) {
			std::cout << "\nBone Index greater than 100. I think you made a mistake "
						 "somewhere...";
		}
	}

	GKBone() {
		name = "NULL";
		index = -1;
	}

	void setName(std::string _name) { name = _name; }

	void setIndex(int _index) { index = _index; }
	//~ void setLocalBindTransform(glm::mat4
	//_localBindTransform){localBindTransform = _localBindTransform;}

	void addChild(int child_index) { Children.push_back(child_index); }

	void setLocalAnimatedTransform(glm::mat4 _animatedTransform) { localAnimatedTransform = _animatedTransform; }

	static GKBone LinInterp(GKBone& edge0, GKBone& edge1, float mix) {
		if (mix <= 0)
			return edge0;
		if (mix >= 1)
			return edge1;
		Transform e0;
		e0.setModel(edge0.localAnimatedTransform);
		Transform e1;
		e1.setModel(edge1.localAnimatedTransform);
		glm::vec3 pos = e0.getPos() * (1 - mix) + e1.getPos() * mix;
		glm::quat e0_rot = e0.getRotQUAT();
		glm::quat e1_rot = e1.getRotQUAT();
		glm::quat rot = glm::slerp(e0_rot, e1_rot, mix);
		glm::vec3 scale = e0.getScale() * (1 - mix) + e1.getScale() * mix;
		Transform b(pos, rot, scale);
		GKBone retval = edge0;
		retval.localAnimatedTransform = b.getModel();
		return retval;
	}
	// TODO fix dis
	void calcAnimTransform(glm::mat4 parentAnimTransform, std::vector<GKBone>& WholeSkeleton,
						   int RecursionLimiter = 100) { // For the root joint, use glm::mat4 m4( 1.0 )
		/*
		The Exact same thing, except it is for animated transforms
		 * */
		// std::cout << "\ncalcAnimTransform called for bone " << name <<
		// std::endl;
		animatedTransform = parentAnimTransform * localAnimatedTransform;
		// Does the transformation from the space of our parent to here.
		// Which is exactly what we have to do for rendering the bones in
		// modelspace.

		// We're only interested in making the collective movement
		// From the origin of modelspace to the bone's space.
		// So all we have to do is perform that transformation
		if (RecursionLimiter < 0 || Children.size() > 100) // If we are stuck or the arrangement of children is
														   // impossible, break immediately. Prevents stack overflows.
			return;
		for (int Child : Children) {
			if (Child > 0 && Child < WholeSkeleton.size() && WholeSkeleton.size() > 0)
				WholeSkeleton[Child].calcAnimTransform(animatedTransform, WholeSkeleton, RecursionLimiter - 1);
		}
	}

	~GKBone() {
		if (Children.size() > 0)
			Children.clear();
	}
};

struct BoneKeyframe {
	std::vector<GKBone> Skeleton;
	double t_time_index = 0;
	BoneKeyframe() {}
	bool operator<(const BoneKeyframe& other) { return t_time_index < other.t_time_index; }
	bool operator>(const BoneKeyframe& other) { return t_time_index > other.t_time_index; }
	static BoneKeyframe SmoothInterp(BoneKeyframe& e0, BoneKeyframe& e1, double time) {
		if ((e0.Skeleton.size() != e1.Skeleton.size()) || ((e1.t_time_index - e0.t_time_index) == 0) || (e1.t_time_index <= e0.t_time_index))
			return e0;
		time -= e0.t_time_index;
		time /= (e1.t_time_index - e0.t_time_index);
		if (time >= 1)
			return e1;
		if (time <= 0)
			return e0;
		// Time is now the mix ratio between all bones in e0 and e1.
		BoneKeyframe retval = e0;
		for (size_t i = 0; i < retval.Skeleton.size(); i++) {
			retval.Skeleton[i] = GKBone::LinInterp(e0.Skeleton[i], e1.Skeleton[i], glm::smoothstep(0.0f, 1.0f, (float)time));
		}
		return retval;
	}
};

struct BoneAnim {
	std::string name = "NONAME";
	std::vector<BoneKeyframe> frames;
	double totalAnimTime = 0.0;
	double calcTime() {
		if (frames.size() > 0)
			totalAnimTime = frames.back().t_time_index;
		return totalAnimTime;
	}
	void sortFrames() { std::sort(frames.begin(), frames.end()); }
	BoneKeyframe getFrameAtTime(double t) {
		if (frames.size() < 1)
			return BoneKeyframe();
		while (t < 0)
			t += totalAnimTime;
		while (t > totalAnimTime)
			;
		t -= totalAnimTime;
		double t_sofar = 0;
		BoneKeyframe* bruh = &(frames[0]);
		BoneKeyframe* Frame_In_Question = &(frames[0]);
		for (size_t i = 0; i < frames.size() - 1; i++) {
			Frame_In_Question = &(frames[i + 1]);
			t_sofar = Frame_In_Question->t_time_index;
			if (t_sofar > t) // We've gone too far
			{
				break;
			}
			bruh = Frame_In_Question;
		}
		return BoneKeyframe::SmoothInterp(*bruh, *Frame_In_Question, t);
	}
};

class IndexedModel // A model that has been completely loaded from file and
				   // optimized.
{
  public:
	IndexedModel() {}
	//~ ~IndexedModel(){
	//~ positions.clear();
	//~ texCoords.clear();
	//~ normals.clear();
	//~ colors.clear();
	//~ indices.clear();
	//~ std::cout << "\nDestroying an Indexed Model is a SIN!" << std::endl;
	//~ }
	IndexedModel(const IndexedModel& Other) {
		positions = Other.positions;
		texCoords = Other.texCoords;
		normals = Other.normals;
		colors = Other.colors;
		//~ Bones = Other.Bones;
		indices = Other.indices;
		renderflags = Other.renderflags;
		hadRenderFlagsInFile = Other.hadRenderFlagsInFile;
		smoothshading = Other.smoothshading;
		myFileName = Other.myFileName;
		//~ std::cout << "\nUsed Copy Constructor " << std::endl;
	}
	void operator=(const IndexedModel& Other) {
		positions = Other.positions;
		texCoords = Other.texCoords;
		normals = Other.normals;
		colors = Other.colors;
		indices = Other.indices;
		renderflags = Other.renderflags;
		hadRenderFlagsInFile = Other.hadRenderFlagsInFile;
		smoothshading = Other.smoothshading;
		myFileName = Other.myFileName;
		//~ std::cout << "\nUsed Operator= " << std::endl;
	}
	void operator+=(const IndexedModel& Other);

	void removeUnusedPoints();
	void clear() {
		positions.clear();
		texCoords.clear();
		normals.clear();
		colors.clear();
		indices.clear();
	}
	glm::mat3x3 getTrianglePositions(unsigned int tri_index);
	static glm::vec3 calcTriNorm(glm::mat3x3 Positions);
	void removeDegenerateTris();				 // should also remove duplicates
	void removeTriangle(unsigned int tri_index); // indices[i], i/3 is tri_index
	void pushTri(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec2 t1, glm::vec2 t2, glm::vec2 t3, glm::vec3 c1, glm::vec3 c2, glm::vec3 c3,
				 // Normals will be pushed back as 0
				 // for calcnormals
				 bool uniquePositions = false);
	// Merge identical positions.
	void MaximizeSmoothing();
	// Split all triangles.
	void MinimizeSmoothing();
	void validate();
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> texCoords;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> colors;
	std::vector<unsigned int> indices;
	unsigned int renderflags = 0; // NOTE TO SELF: Initialization matters
	bool hadRenderFlagsInFile = false;
	bool smoothshading = true;
	void calcNormals();
	size_t getIndexOfPoint(glm::vec3 p) {
		for (size_t i = 0; i < positions.size(); i++)
			if (positions[i] == p)
				return i;
		return positions.size() + 1; // Something which is clearly invalid
	}
	float calcRadius();
	void applyTransform(glm::mat4 _transform, float w_component = 1.0);
	bool calcAABB(float& _x, float& _y, float& _z);
	std::string exportToString(bool exportColors);
	std::string myFileName = "/Aux//"; // Impossible path on windows and probably linux too
};
// Needed for handling OBJ Model loading errors.
IndexedModel createBox(float Xdim, float Ydim, float Zdim, glm::vec3 color = glm::vec3(0, 0, 0), glm::vec3 texScale = glm::vec3(1, 1, 1));
IndexedModel getErrorShape(std::string error_name = std::string("//Aux/Unknown Error"));

class OBJModel // An unoptimized model that can make optimized models.
{
  public:
	std::vector<OBJIndex> OBJIndices;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> vertcolors;
	std::vector<GKBone> Bones; // For exporting Bones
	bool hasUVs = false;
	bool hasNormals = false;
	bool hasVertexColors = false;
	bool smoothshading = true;
	unsigned int renderflags = 0; // NOTE TO SELF: Initialization matters.
	bool hadRenderFlagsInFile = false;
	bool caughtError = false;
	std::string Error_Text;
	std::string myFileName = "N/A";
	OBJModel(const std::string& fileName, bool isFileContent = false);
	
	IndexedModel toIndexedModel();

  private:
	void parseOBJFile(const std::string& fileContent);
	void CreateOBJFace(const std::string& line);
	glm::vec2 ParseOBJVec2(const std::string& line);
	glm::vec3 ParseOBJVec3(const std::string& line);
	OBJIndex ParseOBJIndex(const std::string& token, bool* hasUVs, bool* hasNormals, bool* hasVertColors);
};
};	 // namespace gekRender
#endif // OBJ_LOADER_H_INCLUDED
