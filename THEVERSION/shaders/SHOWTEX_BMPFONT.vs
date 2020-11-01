#version 130

in vec3 vPosition;

varying vec2 texcoord;

vec3 worldpos; //Position of the fragment in the world!

uniform mat4 World2Camera; //the world to camera transform. I figure this is faster than calculating MVP seperately per vertex.
void main()
{
	vec3 vP = vPosition;
	vec4 newpos = World2Camera * vec4(vP,1.0);
	gl_Position = newpos;
	texcoord.x = (newpos.x + 1.0)*0.5;
	texcoord.y = (newpos.y + 1.0)*0.5;
}
