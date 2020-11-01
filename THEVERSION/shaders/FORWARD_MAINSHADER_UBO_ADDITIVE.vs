#version 330
#define MAX_POINT_LIGHTS 32
#define MAX_DIR_LIGHTS 2
#define MAX_AMB_LIGHTS 3
#define MAX_CAM_LIGHTS 5

//FORWARD_MAINSHADER.VS
//(C) DMHSW 2019

const float NormalOffsetShadowBias = 0.1;

const mat4 biasMatrix = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 0.5, 0.0,
0.5, 0.5, 0.5, 1.0
);

//List of flags. Some of these are no longer implemented, they DID NOT caus too much of a performance problem. I do not recommend you enable them. (Remember: Mat4 changing lag?)
const uint GK_RENDER = uint(1); // Do we render it? This is perhaps the most important flag.
const uint GK_TEXTURED = uint(2); // Do we texture it? if disabled, only the texture will be used. if both this and colored are disabled, the object will be black.
const uint GK_COLORED = uint(4);// Do we color it? if disabled, only the texture will be used. if both this and textured are disabled, the object will be black.
const uint GK_FLAT_NORMAL = uint(8); // Do we use flat normals? If this is set, then the normals output to the fragment shader in the initial opaque pass will use the flat layout qualifier. 
const uint GK_FLAT_COLOR = uint(16); // Do we render flat colors? the final, provoking vertex will be used as the color for the entire triangle.
const uint GK_COLOR_IS_BASE = uint(32); //Use the color as the primary. Uses texture as primary if disabled.
const uint GK_TEXTURE_ALPHA_MULTIPLY = uint(2048);//Do we multiply the color from the texture by the alpha before doing whatever it is we're doing? I do not recommend enabling this and alpha culling, especially if you're trying to create a texture-on-a-flat-color-model effect (Think sega saturn models)
const uint GK_ENABLE_ALPHA_CULLING = uint(4096); //Do we use the texture alpha to cull alpha fragments
const uint GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR = uint(8192); //if the alpha from the texture is <0.5 then the secondary color will replace the primary color.
const uint GK_BONE_ANIMATED = uint(16384); //Do we apply bone animation?

layout( location = 0 ) in vec3 vPosition;
layout( location = 1 ) in vec2 intexcoord;
layout( location = 2 ) in vec3 Normal;
layout( location = 3 ) in vec3 VertexColor; 
//VertexColor can also be used to store Bone information...
//<1 component is bone weight with 0.95 as maximum and 0.0 as minimum (remapping)
//Integer component is Bone ID
layout( location = 4 ) in mat4 instanced_model_matrix; //used for instancing! 4,5,6,7

out vec2 texcoord;
out vec3 normout;
out float whichVertColor;
out float alphareplace;
out float colorbase;
out vec3 Smooth_Vert_Color;
//out vec3 Flat_Vert_Color; //TODO: make this flat
// out vec3 vert_to_camera;
out vec3 worldout;

//out float isFlatNormal;
//out float isTextured;
out float isColored;
//out float isFlatColor;
//out float ColorisBase;
//out float AlphaReplaces;
//out float isTinted;
//out float isDarkened;
//out float isAveraged;
//out float isNotAnyofThose;
vec4 worldpos; //Position of the fragment in the world!


uniform uint renderflags;
uniform float windowsize_x;
uniform float windowsize_y;
uniform mat4 World2Camera; //the world to camera transform. I figure this is faster than calculating VP seperately per vertex.
uniform mat4 Model2World; //Model->World
uniform vec3 CameraPos; //Camera position in world space
uniform float is_instanced; //used for determining whether or not we should use instanced_model_matrix or Model2World
uniform float has_bones; //Used for toggling whether or not the Bone UBO is used
uniform vec2 texOffset;
out vec4 CameraLightPos[MAX_CAM_LIGHTS];//Position in camera-important space of the point
out vec4 CameraLightPosWorld[MAX_CAM_LIGHTS];//Position in camera-important space of the point

layout (std140) uniform light_data{ //UNIFORM BINDING POINT 0
	vec4 dir_direction[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 1 (STARTS AT 0)
	vec4 dir_color[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 2 (STARTS AT 16 * 2 * 1 + 0)
	
	vec4 dir_sphere1[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 3 (STARTS AT 16 * 2 * 2 + 0)
	vec4 dir_sphere2[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 4 (STARTS AT 16 * 2 * 3 + 0)
	vec4 dir_sphere3[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 5 (STARTS AT 16 * 2 * 4 + 0)
	vec4 dir_sphere4[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 6 (STARTS AT 16 * 2 * 5 + 0)
	
	vec4 dir_AABBp1[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 7 (STARTS AT 16 * 2 * 6 + 0)
	vec4 dir_AABBp2[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 8 (STARTS AT 16 * 2 * 7 + 0)
	vec4 dir_AABBp3[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 9 (STARTS AT 16 * 2 * 8 + 0)
	vec4 dir_AABBp4[MAX_DIR_LIGHTS]; // 4 bytes per float * 4 floats per element * 2 elements = 32 bytes 10 (STARTS AT 16 * 2 * 9 + 0)
	//320 subtotal
	//320 bytes so far
	
	//point lights
	vec4 point_position[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 1 (STARTS AT 16 * 32 * 0 + 320)
	vec4 point_color[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 2 (STARTS AT 16 * 32 * 1 + 320)
	//These currently go unused
	vec4 point_sphere1[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 3 (STARTS AT 16 * 32 * 2 + 320)
	vec4 point_sphere2[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 4 (STARTS AT 16 * 32 * 3 + 320)
	vec4 point_sphere3[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 5 (STARTS AT 16 * 32 * 4 + 320)
	vec4 point_sphere4[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 6 (STARTS AT 16 * 32 * 5 + 320)
	vec4 point_AABBp1[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 7 (STARTS AT 16 * 32 * 6 + 320)
	vec4 point_AABBp2[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 8 (STARTS AT 16 * 32 * 7 + 320)
	vec4 point_AABBp3[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 9 (STARTS AT 16 * 32 * 8 + 320)
	vec4 point_AABBp4[MAX_POINT_LIGHTS]; // 4 bytes per float * 4 floats per element * 32 elements = 512 bytes 10 (STARTS AT 16 * 32 * 9 + 320)
	//5120 subtotal
	//320 + 5120 bytes = 5440 bytes so far
	
	//amb lights
	vec4 amb_position[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 0 + 5440)
	vec4 amb_color[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 1 + 5440)
	
	vec4 amb_sphere1[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 2 + 5440)
	vec4 amb_sphere2[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 3 + 5440)
	vec4 amb_sphere3[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 4 + 5440)
	vec4 amb_sphere4[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 5 + 5440)
	vec4 amb_AABBp1[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 6 + 5440)
	vec4 amb_AABBp2[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 7 + 5440)
	vec4 amb_AABBp3[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes (STARTS AT 16 * 3 * 8 + 5440)
	vec4 amb_AABBp4[MAX_AMB_LIGHTS]; // 4 bytes per float * 4 floats per element * 3 elements = 48 bytes 10 (STARTS AT 16 * 3 * 9 + 5440)
	//480 subtotal
	
	//5440 + 480 = 5920 bytes so far
	
	
	//cam lights
	vec4 cam_position[MAX_CAM_LIGHTS];//Is Direction if range < 0 // 4 bytes per float * 4 floats per element * 5 elements = 80 bytes   (STARTS AT 16 * 5 * 0 + 5920)
	vec4 cam_color[MAX_CAM_LIGHTS]; // 4 bytes per float * 4 floats per element * 5 elements = 80 bytes 								(STARTS AT 16 * 5 * 1 + 5920)
	mat4 cam_viewproj[MAX_CAM_LIGHTS]; // 4 bytes per float * 16 floats per element * 5 elements = 320 bytes 							(STARTS AT 16 * 5 * 2 + 5920)
	//480 subtotal
	
	//5920 + 480 = 6400 bytes so far
	
	
	
	//All non-4-aligned things are down here, organized in this pattern: vec2s, floats, uints, ints
	vec2 cam_radii[MAX_CAM_LIGHTS]; 		// 4 bytes per float * 4 floats per element(2 float or 8 byte padding) * 5 elements = 80 bytes (STARTS AT 6400)
	vec2 cam_dim[MAX_CAM_LIGHTS];			// 4 bytes per float * 4 floats per element(2 float or 8 byte padding) * 5 elements = 80 bytes (STARTS AT 6480)
	//Arrays, no matter the type, every element is at least 4 bytes.
	float point_range[MAX_POINT_LIGHTS]; 	// 4 * 4(3 floats or 12 bytes of padding) * 32 = 512 (STARTS AT 6560)
	float point_dropoff[MAX_POINT_LIGHTS]; 	// 4 * 4(3 floats or 12 bytes of padding) * 32 = 512 (STARTS AT 7072)
	
	float amb_range[MAX_AMB_LIGHTS]; 		// 4 * 4(3 floats or 12 bytes of padding) * 3 = 48 (STARTS AT 7584)
	
	float cam_solidColor[MAX_CAM_LIGHTS]; 	// 4 * 4(3 floats or 12 bytes of padding) * 5 = 80 (STARTS AT 7632)
	float cam_range[MAX_CAM_LIGHTS]; 		// 4 * 4(3 floats or 12 bytes of padding) * 5 = 80 (STARTS AT 7712)
	float cam_shadows[MAX_CAM_LIGHTS]; 		// 4 * 4(3 floats or 12 bytes of padding) * 5 = 80 (STARTS AT 7792)
	//^If this is enabled, solidcolor will be used and the texture sample will be treated as a sample into a shadowmap
	//1472 subtotal
	//6400 + 1472 = 7872 bytes
	
	uint dir_isblacklist[MAX_DIR_LIGHTS];   	// 4 * 4(padded) * 2 = 32 bytes (STARTS AT 7872)
	uint point_isblacklist[MAX_POINT_LIGHTS]; 	// 4 * 4(padded) * 32 = 512 bytes (STARTS AT 7904) //NOTE this is currently not being set
	uint amb_isblacklist[MAX_AMB_LIGHTS];		// 4 * 4(padded) * 3 = 48 bytes (STARTS AT 8416)
	//592 byte subtotal
	//7872 + 592 = 8464 bytes so far
	
	//how many are actually being sent in?
	int numpointlights;						//AT 8464
	int numdirlights;						//AT 8468
	int numamblights;						//AT 8472
	int numcamlights;						//AT 8476
	//These 4 are all packed into a single sizeof(vec4)
	//TOTAL 8480
	//Almost certainly supported by all target machines (3.3+)
};

//Bone data
//~ layout (std140) uniform bone_data { //UNIFORM BINDING POINT 1
	//~ mat4 Bone_World_Transforms[50];
//~ };

//~ mat4 IdentityTransform = mat4(1.0, 0.0, 0.0, 0.0, 
							  //~ 0.0, 1.0, 0.0, 0.0,
							  //~ 0.0, 0.0, 1.0, 0.0,
							  //~ 0.0, 0.0, 0.0, 1.0);

void
main()
{
	mat4 worldtrans = float(is_instanced > 0) * instanced_model_matrix + (1 - float(is_instanced > 0)) * Model2World;
	//The position of this vertex in the world coordinate system.
	
	//Calculate the position of the point in modelspace, if and only if and only f
	vec4 ModelSpacePosition = vec4(vPosition,1.0);
	vec4 ModelSpaceNormal = vec4(Normal, 0.0);
	//~ ivec3 IDs = ivec3(0,0,0);
	//~ vec3 weights = vec3(0,0,0);
	//~ vec4 BoneTransformedMSPs[3];
	//~ vec4 BoneTransformedMSNs[3];
	//~ vec4 BoneCalcResults[2]; //0 is pos, 1 is norm
	//~ IDs[0] = int(clamp(VertexColor[0], 1, 50));
	//~ IDs[1] = int(clamp(VertexColor[1], 1, 50));
	//~ IDs[2] = int(clamp(VertexColor[2], 1, 50));
	//~ weights[0] = (VertexColor[0] - float(IDs[0])) * 2.0;
	//~ weights[1] = (VertexColor[1] - float(IDs[1])) * 2.0;
	//~ weights[2] = (VertexColor[2] - float(IDs[2])) * 2.0;
	//~ for(int i = 0; i < 3; i++){
		//~ //Transform by Bones[IDs[i]] and store in BoneTransformedMSPs, repeat for MSNs


		//~ //BoneTransformedMSPs[i] = Bone_World_Transforms[IDs[i] - 1] * ModelSpacePosition;
		//~ BoneTransformedMSPs[i] = IdentityTransform * ModelSpacePosition;
		//~ //BoneTransformedMSNs[i] = Bone_World_Transforms[IDs[i] - 1] * ModelSpaceNormal;
		//~ BoneTransformedMSNs[i] = IdentityTransform * ModelSpaceNormal;
	//~ }
	//~ BoneCalcResults[0] = (weights[0] * BoneTransformedMSPs[0])  + (weights[1] * BoneTransformedMSPs[1]) + (weights[2] * BoneTransformedMSPs[2]);
	//~ BoneCalcResults[1] = (weights[0] * BoneTransformedMSNs[0])  + (weights[1] * BoneTransformedMSNs[1]) + (weights[2] * BoneTransformedMSNs[2]);
	//~ BoneCalcResults[0].w = 1.0;
	//~ BoneCalcResults[1].w = 0.0;
	//~ float use_bone_version = clamp(has_bones, 0, 1) * float((renderflags & GK_BONE_ANIMATED) > uint(0));
	//~ float use_bone_version = 0;
	worldpos = (worldtrans * ModelSpacePosition);
	vec3 normie = (worldtrans * ModelSpaceNormal).xyz;
	normout = normie.xyz;
	//Transform for cameralights
	CameraLightPos[0] = cam_viewproj[0] * (worldpos + vec4(normie.xyz, 0.0) * NormalOffsetShadowBias);
	CameraLightPos[1] = cam_viewproj[1] * (worldpos + vec4(normie.xyz, 0.0) * NormalOffsetShadowBias);
	CameraLightPos[2] = cam_viewproj[2] * (worldpos + vec4(normie.xyz, 0.0) * NormalOffsetShadowBias);
	CameraLightPos[3] = cam_viewproj[3] * (worldpos + vec4(normie.xyz, 0.0) * NormalOffsetShadowBias);
	CameraLightPos[4] = cam_viewproj[4] * (worldpos + vec4(normie.xyz, 0.0) * NormalOffsetShadowBias);
	
	CameraLightPosWorld[0] = cam_viewproj[0] * (worldpos);
	CameraLightPosWorld[1] = cam_viewproj[1] * (worldpos);
	CameraLightPosWorld[2] = cam_viewproj[2] * (worldpos);
	CameraLightPosWorld[3] = cam_viewproj[3] * (worldpos);
	CameraLightPosWorld[4] = cam_viewproj[4] * (worldpos);
	
	texcoord = intexcoord + texOffset; //this is faster
	gl_Position = World2Camera * worldpos;
	
	
	Smooth_Vert_Color = VertexColor;
	//Flat_Vert_Color = VertexColor;
	
	//whichNormal = float((renderflags & GK_FLAT_NORMAL) > uint(0));
	//whichVertColor = float((renderflags & GK_FLAT_COLOR) > uint(0));
	alphareplace = float((renderflags & GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR) > uint(0));
	colorbase = float((renderflags & GK_COLOR_IS_BASE) > uint(0));
	alphareplace = float((renderflags & GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR) > uint(0));
	isColored = float((renderflags & GK_COLORED) > uint(0));
	
	// vert_to_camera = CameraPos  - worldpos;
	worldout = worldpos.xyz;
}
