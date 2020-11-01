#version 330
// #extension GL_ARB_conservative_depth : enable
// out vec4 fColor[2];
// SKYBOX.FS
// (C) DMHSW 2018
// layout (depth_greater) out float gl_FragDepth;
// ^ should probably re-enable that later




uniform sampler2D diffuse; //This is actually the texture unit. limit 32. This one happens to be for the literal object's texture.
uniform samplerCube worldaroundme; //This is the cubemap we use for reflections.

in vec3 normout;
in vec3 ND_out;
in vec2 window_size;
in vec3 vert_to_camera;
in float ourdepth;
uniform float jafar;
uniform float janear;

uniform float diffusivity;

vec2 bettertexcoord;
vec4 texture_value;
vec3 color_value;
vec3 primary_color;
vec3 secondary_color;
vec3 finalcolor = vec3(0,0,0); //default value. Does it work?
uniform uint renderflags;

void main()
{
	vec3 UnitNormal;
	vec3 usefulNormal;

	
	usefulNormal = normalize(normout) ;
	

	
	
	
	
	gl_FragData[0] = vec4(texture(worldaroundme,usefulNormal).rgb, 1);
}
