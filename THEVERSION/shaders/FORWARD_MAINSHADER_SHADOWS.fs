#version 330
#define MAX_POINT_LIGHTS 32
#define MAX_DIR_LIGHTS 2
#define MAX_AMB_LIGHTS 3
#define MAX_CAM_LIGHTS 5
#extension GL_ARB_conservative_depth : enable
// out vec4 fColor[2];
// FORWARD_MAINSHADER.FS
// (C) DMHSW 2018
// layout (depth_greater) out float gl_FragDepth;
// ^ should probably re-enable that later

//List of flags. Some of these are no longer implemented, they caused too much of a performance problem. I do not recommend you enable them.
const uint GK_RENDER = uint(1); // Do we render it? This is perhaps the most important flag.
const uint GK_TEXTURED = uint(2); // Do we texture it? if disabled, only the texture will be used. if both this and colored are disabled, the object will be black.
const uint GK_COLORED = uint(4);// Do we color it? if disabled, only the texture will be used. if both this and textured are disabled, the object will be black.
const uint GK_FLAT_NORMAL = uint(8); // Do we use flat normals? If this is set, then the normals output to the fragment shader in the initial opaque pass will use the flat layout qualifier. 
const uint GK_FLAT_COLOR = uint(16); // Do we render flat colors? the final, provoking vertex will be used as the color for the entire triangle.
const uint GK_COLOR_IS_BASE = uint(32); //Use the color as the primary. Uses texture as primary if disabled.
const uint GK_TINT = uint(64); //Does secondary add to primary?
const uint GK_DARKEN = uint(128);//Does secondary subtract from primary?
const uint GK_AVERAGE = uint(256);//Do secondary and primary just get averaged?
const uint GK_COLOR_INVERSE = uint(512);//Do we use the inverse of the color?
const uint GK_TEXTURE_INVERSE = uint(1024);//Do we use the inverse of the texture color? DOES NOT invert alpha.
const uint GK_TEXTURE_ALPHA_MULTIPLY = uint(2048);//Do we multiply the color from the texture by the alpha before doing whatever it is we're doing? I do not recommend enabling this and alpha culling, especially if you're trying to create a texture-on-a-flat-color-model effect (Think sega saturn models)
const uint GK_ENABLE_ALPHA_CULLING = uint(4096); //Do we use the texture alpha to cull alpha fragments
const uint GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR = uint(8192); //if the alpha from the texture is <0.5 then the secondary color will replace the primary color.


struct dirlight {
	vec3 direction;
	vec3 color;
	vec4 sphere1;
	vec4 sphere2;
	vec4 sphere3;
	vec4 sphere4;
	vec3 AABBp1;
	vec3 AABBp2;
	vec3 AABBp3;
	vec3 AABBp4;
	uint isblacklist;
};

struct pointlight{
	vec3 position;
	vec3 color;
	float range;
	float dropoff;
	vec4 sphere1;
	vec4 sphere2;
	vec4 sphere3;
	vec4 sphere4;
	vec3 AABBp1;
	vec3 AABBp2;
	vec3 AABBp3;
	vec3 AABBp4;
	uint isblacklist;
};

struct amblight{
	vec3 position;
	vec3 color;
	float range;
	vec4 sphere1;
	vec4 sphere2;
	vec4 sphere3;
	vec4 sphere4;
	vec3 AABBp1;
	vec3 AABBp2;
	vec3 AABBp3;
	vec3 AABBp4;
	uint isblacklist;
};

struct cameralight{
	mat4 viewproj;
	vec3 position;//Is Direction if range < 0
	vec3 color;
	float solidColor;
	float range;
	float shadows; //If this is enabled, solidcolor will be used and the texture sample will be treated as a sample into a shadowmap
	vec4 sphere1;
	vec4 sphere2;
	vec4 sphere3;
	vec4 sphere4;
	vec3 AABBp1;
	vec3 AABBp2;
	vec3 AABBp3;
	vec3 AABBp4;
	uint isblacklist;
};





uniform sampler2D diffuse; //0
uniform sampler2D CameraTex1; //3
uniform sampler2D CameraTex2;//4
uniform sampler2D CameraTex3; //5
uniform sampler2D CameraTex4; //6
uniform sampler2D CameraTex5; //7
uniform sampler2D CameraTex6; //8
uniform sampler2D CameraTex7; //9
uniform sampler2D CameraTex8; //10
uniform sampler2D CameraTex9; //11
uniform sampler2D CameraTex10; //12
//vec4 CameraTexSamples[10]; //Grab the samples from the Sampler2Ds
uniform samplerCube worldaroundme; //1
uniform samplerCube SkyboxCubemap; //2
uniform vec4 backgroundColor;
uniform vec2 fogRange;

in vec2 texcoord;
//~ in vec3 normout;
//flat in vec3 flatnormout;

in float isFlatNormal;
in float isTextured;
in float isColored;
in float isFlatColor;
in float ColorisBase;
in float AlphaReplaces;
in float isTinted;
in float isDarkened;
in float isAveraged;
in float isNotAnyofThose;

flat in vec3 Flat_Vert_Color;
flat in float whichVertColor;
flat in float alphareplace;
flat in float colorbase;
in vec3 Smooth_Vert_Color;
// in vec3 vert_to_camera;
//Logic from the vertex level
in vec3 worldout;


// uniform float ambient;
uniform float specreflectivity;
uniform float specdamp;
uniform float emissivity;
uniform float enableCubeMapReflections;
uniform float enableCubeMapDiffusivity;
uniform float diffusivity;
uniform vec3 CameraPos; //Camera position in world space
uniform float EnableTransparency;


//The lights
uniform dirlight dir_lightArray[MAX_DIR_LIGHTS];
uniform pointlight point_lightArray[MAX_POINT_LIGHTS];
uniform amblight amb_lightArray[MAX_AMB_LIGHTS];
uniform cameralight camera_lightArray[MAX_CAM_LIGHTS];
//how many are actually being sent in?
uniform int numpointlights;
uniform int numdirlights;
uniform int numamblights;
uniform int numcamlights;


vec2 bettertexcoord;
vec4 texture_value;
vec3 color_value;
vec3 primary_color;
vec3 secondary_color;
uniform uint renderflags;

float length2vec3(vec3 getmylength){
	return dot(getmylength, getmylength);
}

void main()
{	
	gl_FragData[0] = vec4(gl_FragCoord.z);
}
