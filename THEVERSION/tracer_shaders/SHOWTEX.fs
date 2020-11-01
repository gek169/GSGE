#version 130
//out vec4 fColor[2];

varying vec2 texcoord;

uniform sampler2D diffuse; //This is actually the texture unit. limit 32. This one happens to be for the literal object's texture.


void main()
{
	gl_FragColor = texture2D(diffuse, texcoord);
}
