#version 330
layout( location = 9 ) in vec3 initP;
layout( location = 10 ) in vec3 initV;
layout( location = 11 ) in float birthT;

out float Transp;
out vec2 TexCoord;
out vec3 Col;

uniform float Time;
uniform vec3 Accel;
uniform float MaxAge;
uniform float ParticleSize;
uniform vec3 initColor;
uniform vec3 finalColor;
uniform float initTransp;
uniform float finalTransp;
uniform float ColorIsTint; //1 for tint, 0 for multiply.

uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;

const vec3 offsets[] = vec3[](
	vec3(-0.5, -0.5, 0), vec3(0.5, -0.5, 0), vec3(0.5,0.5,0),
	vec3(-0.5, -0.5, 0), vec3(0.5,0.5,0), vec3(-0.5, 0.5, 0)
);

//~ const vec2 texcoords[] = vec2[](
	//~ vec2(0,0), vec2(1,0), vec2(1,1),
	//~ vec2(0,0), vec2(1,1), vec2(0,1)
//~ );

const vec2 texcoords[] = vec2[](
	vec2(0,1), vec2(1,1), vec2(1,0),
	vec2(0,1), vec2(1,0), vec2(0,0)
);

//~ u_ViewMatrixLoc = myShader->getUniformLocation("ViewMatrix");
		//~ u_ProjMatrixLoc = myShader->getUniformLocation("ProjMatrix");
		//~ u_InitColorLoc = myShader->getUniformLocation("initColor");
		//~ u_FinalColorLoc = myShader->getUniformLocation("finalColor");
		//~ u_MaxAgeLoc = myShader->getUniformLocation("MaxAge");
		//~ u_TimeLoc = myShader->getUniformLocation("Time");
		//~ u_InitTranspLoc = myShader->getUniformLocation("initTransp");
		//~ u_FinalTranspLoc = myShader->getUniformLocation("finalTransp");
		//~ u_ColorIsTintLoc = myShader->getUniformLocation("ColorIsTint");
		//~ u_ParticleSizeLoc = myShader->getUniformLocation("ParticleSize");
		//~ u_ParticleTexLoc = myShader->getUniformLocation("ParticleTex");
		//~ u_BrightnessLoc = myShader->getUniformLocation("Brightness");
		//~ u_AccelLoc = myShader->getUniformLocation("Accel");
void main(){
	TexCoord = texcoords[gl_VertexID];
	vec3 FinalVertPosition;
	float t = Time - birthT;
	if(Time >= birthT && t < MaxAge){
		vec3 pos = initP + initV * t + 0.5 * Accel * t * t;
		FinalVertPosition = (ViewMatrix * vec4(pos,1)).xyz + offsets[gl_VertexID] * ParticleSize;
		Transp = clamp(mix(initTransp, finalTransp, t/MaxAge), 0,1);
		Col = mix(initColor, finalColor, t/MaxAge);
	} else {
		Col = vec3(0,0,0);
		Transp = 0;
		FinalVertPosition = vec3(0,0,0);
	}
	//~ Transp = clamp(Transp,0,1);
	gl_Position = ProjMatrix * vec4(FinalVertPosition, 1);
}
