#version 330

in float Transp;
in vec2 TexCoord;
in vec3 Col;

uniform float MaxAge;
uniform vec3 Brightness;
uniform sampler2D ParticleTex;
uniform float ColorIsTint; //1 for tint, 0 for multiply.

void main(){
	vec4 texColor = texture2D(ParticleTex, TexCoord);
	//~ texColor = vec4(1,0,0,0.5); //debug
	texColor.w *= Transp;
	texColor.x += ColorIsTint * Col.x;
	texColor.y += ColorIsTint * Col.y;
	texColor.z += ColorIsTint * Col.z;
	
	texColor.x *= (1-ColorIsTint > 0)?Col.x:1.0;
	texColor.y *= (1-ColorIsTint > 0)?Col.y:1.0;
	texColor.z *= (1-ColorIsTint > 0)?Col.z:1.0;
	
	texColor.x *= Brightness.x;
	texColor.y *= Brightness.y;
	texColor.z *= Brightness.z;
	gl_FragData[0] = texColor;
}
