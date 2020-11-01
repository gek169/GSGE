#version 330

//SKYBOX.VS
//(C) DMHSW 2018



//We only use the position
layout( location = 0 ) in vec3 vPosition;
layout( location = 1 ) in vec2 intexcoord;
layout( location = 2 ) in vec3 Normal; // we need it
// layout( location = 3 ) in vec3 VertexColor;

out vec3 normout;
flat out vec3 flatnormout;
out vec3 Smooth_Vert_Color;
out vec3 ND_out;
out vec2 window_size;
flat out vec3 Flat_Vert_Color;
out vec3 vert_to_camera;
out float ourdepth;

vec3 worldpos; //Position of the fragment in the world!

uniform float windowsize_x;
uniform float windowsize_y;
uniform mat4 World2Camera; //the world to camera transform. I figure this is faster than calculating MVP seperately per vertex.
uniform mat4 viewMatrix; //Moves the camera to where it's supposed to be
uniform mat4 projection; //Morphs the world into a frustum
uniform mat4 Model2World; //Model->World directly, format: Projection * View
uniform vec3 CameraPos; //Camera Position in world space.
void
main()
{
	mat4 viewModelWithoutTranslation = viewMatrix;
	viewModelWithoutTranslation[3][0] = 0;
	viewModelWithoutTranslation[3][1] = 0;
	viewModelWithoutTranslation[3][2] = 0;
	window_size = vec2(windowsize_x, windowsize_y);
	//The position of this vertex in the world coordinate system.
	worldpos = (Model2World * vec4(vPosition,1.0)).xyz;
	vec4 big_gay = (projection * viewModelWithoutTranslation)* Model2World * vec4(vPosition,1.0);
	gl_Position = big_gay;
	ourdepth = big_gay.z; //Depth
	normout = (Model2World * vec4(Normal, 0.0)).xyz; //We auto-calculated the normals for the skybox model, remember?
	// flatnormout = normout;
	ND_out = big_gay.xyz;
	// Smooth_Vert_Color = VertexColor;
	// Flat_Vert_Color = VertexColor;
	
	vert_to_camera = CameraPos  - worldpos;
}
