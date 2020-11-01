#version 330

//INITIAL_OPAQUE.VS
//(C) DMHSW 2018

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


layout( location = 0 ) in vec3 vPosition;
layout( location = 1 ) in vec2 intexcoord;
layout( location = 2 ) in vec3 Normal;
layout( location = 3 ) in vec3 VertexColor;

out vec2 texcoord;
out vec3 normout;
flat out vec3 flatnormout;
out vec3 Smooth_Vert_Color;
out vec4 ND_out;
out vec2 window_size;
flat out vec3 Flat_Vert_Color;
out vec3 vert_to_camera;
out float ourdepth;
out vec3 worldout;

vec3 worldpos; //Position of the fragment in the world!


uniform uint renderflags;
uniform float windowsize_x;
uniform float windowsize_y;
uniform mat4 World2Camera; //the world to camera transform. I figure this is faster than calculating MVP seperately per vertex.
uniform mat4 Model2World; //Model->World
uniform vec3 CameraPos; //Camera position in world space
void
main()
{
	window_size = vec2(windowsize_x, windowsize_y);
	//The position of this vertex in the world coordinate system.
	worldpos = (Model2World * vec4(vPosition,1.0)).xyz;
	vec4 big_gay = World2Camera * Model2World * vec4(vPosition,1.0);
	texcoord = intexcoord; //this is faster
	gl_Position = big_gay;
	ourdepth = big_gay.z; //Depth NDC
	normout = (Model2World * vec4(Normal, 0.0)).xyz;
	flatnormout = normout;
	ND_out = big_gay; //Note: Not actually Normalized Device Coordinates.
	Smooth_Vert_Color = VertexColor;
	Flat_Vert_Color = VertexColor;
	
	vert_to_camera = CameraPos  - worldpos;
	worldout = worldpos;
}
