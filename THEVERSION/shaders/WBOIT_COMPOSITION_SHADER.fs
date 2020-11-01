#version 330
//out vec4 fColor[2];

in vec2 texcoord;
int iter = 50;

uniform sampler2D diffuse; //This is actually the texture unit. limit 32. This one happens to be for the literal object's texture.
uniform sampler2D diffuse2; //Another texture unit

void main()
{
	vec4 accum = texture2D(diffuse, texcoord);
	float reveal = texture2D(diffuse2, texcoord).a;
	
	gl_FragColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
}
