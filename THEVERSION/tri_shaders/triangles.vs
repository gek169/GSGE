
#version 130

in vec4 vPosition;
in vec3 vColor; 

out vec3 color;

void
main()
{
    //gl_Position = vec4(vPosition.x,vPosition.y,0,1);
	color = vColor;
	gl_Position = vPosition;
}
