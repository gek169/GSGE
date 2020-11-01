vec4 stepColorNotAlpha(vec4 stepme, int steps) //Simulate a limited color palette
{
	vec3 Start = stepme.xyz * float(steps);
	vec3 Intermediate = vec3(float(int(Start.x)), float(int(Start.y)), float(int(Start.z)));
	vec4 Final = vec4(Intermediate / vec3(float(steps)), stepme.a);
	return Final;
}

vec4 gammaCorrect(vec4 raw)
{
	vec3 gamma = vec3(1.0/2.2);
	return vec4(pow(raw.xyz, gamma),raw.w);
}

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 saturate(vec4 input, float amount){
	vec3 HSV = rgb2hsv(input.xyz);
	HSV.y += amount;
	HSV = clamp(HSV, vec3(0), vec3(1));
	vec3 resultingRGB = hsv2rgb(HSV);
	resultingRGB = clamp(resultingRGB, vec3(0), vec3(1));
	return vec4(resultingRGB,input.a);
}

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 whitenoise(vec2 co)
{
	vec2 moreRandom = vec2(co.x + float(gl_PrimitiveID), co.y + float(gl_PrimitiveID));
	return vec3(rand(moreRandom));
}

vec3 convertToHDR(vec3 input) //Not actually HDR...
{
	vec3 underExposed = input / 1.5;
	vec3 overExposed = input * 1.2;
	return mix(underExposed, overExposed, input);
}

vec3 ReinHartToneMap(vec3 hdrColor) //HDR!!!
{
	return hdrColor / (hdrColor + vec3(1.0));
}