#version 330

layout( location = 0 ) in vec3 vPosition;

out vec2 texcoord;

vec3 worldpos; //Position of the fragment in the world!

uniform mat4 World2Camera; //the world to camera transform. I figure this is faster than calculating MVP seperately per vertex.
void
main()
{
	vec4 newpos = World2Camera * vec4(vPosition,1.0);
	gl_Position = newpos;
	texcoord.x = (newpos.x + 1.0)*0.5;
	texcoord.y = (newpos.y + 1.0)*0.5;
}
