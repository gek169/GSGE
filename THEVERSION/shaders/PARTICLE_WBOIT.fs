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
	//~ // Insert your favorite weighting function here. The color-based factor
	//~ // avoids color pollution from the edges of wispy clouds. The z-based
	//~ // factor gives precedence to nearer surfaces.
	float weight = 
		  max(min(1.0, max(max(texColor.r, texColor.g), texColor.b) * texColor.a), texColor.a) *
		  clamp(0.03 / (1e-5 + pow(gl_FragCoord.z / 200, 4.0)), 1e-2, 3e3);
		 
	
	
	vec4 transparent_color_0 = vec4(texColor.rgb * texColor.a, texColor.a) * weight;
	gl_FragData[0] = transparent_color_0; //comment this out, use the below line
	gl_FragData[1] = vec4(texColor.a, texColor.a, texColor.a, texColor.a); //Comment this out to disable wboit too

}
