#version 330
// #extension GL_ARB_conservative_depth : enable
// out vec4 fColor[2];
// INITIAL_OPAQUE.FS
// (C) DMHSW 2018
// layout (depth_greater) out float gl_FragDepth;
// ^ should probably re-enable that later

//List of flags. Some of these are no longer implemented, they caused too much of a performance problem. I do not recommend you enable them.
const uint GK_RENDER = uint(1); // Do we render it? This is perhaps the most important flag.
const uint GK_TEXTURED = uint(2); // Do we texture it? if disabled, only the texture will be used. if both this and colored are disabled, the object will be black.
const uint GK_COLORED = uint(4);// Do we color it? if disabled, only the texture will be used. if both this and textured are disabled, the object will be black.
const uint GK_FLAT_NORMAL = uint(8); // Do we use flat normals? If this is set, then the normals output to the fragment shader in the initial opaque pass will use the flat layout qualifier. 
const uint GK_FLAT_COLOR = uint(16); // Do we render flat colors? the final, provoking vertex will be used as the color for the entire triangle.
const uint GK_COLOR_IS_BASE = uint(32); //Use the color as the primary. Uses texture as primary if disabled.
const uint GK_TINT = uint(64); //Does secondary add to primary?
const uint GK_DARKEN = uint(128);//Does secondary subtract from primary?
const uint GK_AVERAGE = uint(256);//Do secondary and primary just get averaged?
const uint GK_COLOR_INVERSE = uint(512);//Do we use the inverse of the color?
const uint GK_TEXTURE_INVERSE = uint(1024);//Do we use the inverse of the texture color? DOES NOT invert alpha.
const uint GK_TEXTURE_ALPHA_MULTIPLY = uint(2048);//Do we multiply the color from the texture by the alpha before doing whatever it is we're doing? I do not recommend enabling this and alpha culling, especially if you're trying to create a texture-on-a-flat-color-model effect (Think sega saturn models)
const uint GK_ENABLE_ALPHA_CULLING = uint(4096); //Do we use the texture alpha to cull alpha fragments
const uint GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR = uint(8192); //if the alpha from the texture is <0.5 then the secondary color will replace the primary color.


//Utility functions
// vec4 when_eq(vec4 x, vec4 y) {
  // return 1.0 - abs(sign(x - y));
// }

// vec4 when_neq(vec4 x, vec4 y) {
  // return abs(sign(x - y));
// }

// vec4 when_gt(vec4 x, vec4 y) {
  // return max(sign(x - y), 0.0);
// }

// vec4 when_lt(vec4 x, vec4 y) {
  // return max(sign(y - x), 0.0);
// }

// vec4 when_ge(vec4 x, vec4 y) {
  // return 1.0 - when_lt(x, y);
// }

// vec4 when_le(vec4 x, vec4 y) {
  // return 1.0 - when_gt(x, y);
// }




uniform sampler2D diffuse; //This is actually the texture unit. limit 32. This one happens to be for the literal object's texture.
uniform samplerCube worldaroundme; //This is the cubemap we use for reflections.

in vec2 texcoord;
in vec3 normout;
flat in vec3 flatnormout;
flat in vec3 Flat_Vert_Color;
in vec3 Smooth_Vert_Color;
in vec4 ND_out;
in vec2 window_size;
in vec3 vert_to_camera;
in float ourdepth;
//Logic from the vertex level
in vec3 worldout;


uniform float ambient;
uniform float specreflectivity;
uniform float specdamp;
uniform float emissivity;
uniform float jafar;
uniform float janear;
uniform float enableCubeMapReflections;
uniform float diffusivity;
uniform vec3 CameraPos; //Camera position in world space

vec2 bettertexcoord;
vec4 texture_value;
vec3 color_value;
vec3 primary_color;
vec3 secondary_color;
vec3 finalcolor = vec3(0,0,0); //default value. Does it work?
uniform uint renderflags;

void main()
{
	bettertexcoord = vec2(texcoord.x, -texcoord.y); //Looks like blender
	vec3 UnitNormal;

	
	
	UnitNormal = normalize(flatnormout) * float((renderflags & GK_FLAT_NORMAL) > uint(0))+ normalize(normout) * (1-float((renderflags & GK_FLAT_NORMAL) > uint(0)));
	

	
	texture_value = (texture2D(diffuse, bettertexcoord)) * float((renderflags & GK_TEXTURED) > uint(0)) + vec4(0.0,0.2,0.0,1.0) * (1-float((renderflags & GK_TEXTURED) > uint(0)));
	
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//UNCOMMENT THIS LINE IF YOU WANT ALPHA CULLING! It will slow down your application, be weary!
	// if ((renderflags & GK_ENABLE_ALPHA_CULLING) > uint(0))
		// if (texture_value.w == 0)
			// discard;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 
	
	//Color stuff
	// if ((renderflags & GK_FLAT_COLOR & GK_COLORED) > uint(0))
	// {
			// color_value = Flat_Vert_Color;
	// } else {
		// if ((renderflags & GK_COLORED) > uint(0))
		// {
			// color_value = Smooth_Vert_Color;
		// } else {
			// color_value = vec3(0.0,0.0,0.0);
		// }
	// }
	float flat_color = float((renderflags & GK_FLAT_COLOR) > uint(0));
	float colored_at_all = float((renderflags & GK_COLORED) > uint(0));
	
	color_value = flat_color * colored_at_all * Flat_Vert_Color + (1-flat_color) * colored_at_all * Smooth_Vert_Color + (1-flat_color) * (1-colored_at_all) * vec3(0,0,0);
	
	//primary_color and secondary_color stuff
	
	// if ((renderflags & GK_COLOR_IS_BASE) > uint(0))
	// {
		// primary_color = color_value;
		// secondary_color = texture_value.xyz;
	// } else {
		// primary_color = texture_value.xyz;
		// secondary_color = color_value;
	// }
	float colorbase = float((renderflags & GK_COLOR_IS_BASE) > uint(0));
	
	primary_color = colorbase * color_value + (1-colorbase) * texture_value.xyz;
	secondary_color = (1-colorbase) * color_value + colorbase * texture_value.xyz;
	
	 // if ((renderflags & GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR) > uint(0))
	 // {
		 // extremely clever programming results in perfect effect...
		// primary_color = (primary_color * (1-texture_value.w)) + (texture_value.xyz * texture_value.w);
	 // }
	
	
	float alphareplace = float((renderflags & GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR) > uint(0));
	
	
	primary_color = primary_color * (1-alphareplace) + ((primary_color * (1-texture_value.w)) + (texture_value.xyz * texture_value.w)) * alphareplace;
	//EQUATION TIME!
	
	
	//Floating point logic tables?!?! 
	float isTint = float((renderflags & GK_TINT) > uint(0)); // 1 if true, 0 if false
	float isNotTint = 1-isTint;//swaps with the other value
	float isDarken = float((renderflags & GK_DARKEN) > uint(0));
	float isNotDarken = 1-isDarken;
	float isAverage = float((renderflags & GK_AVERAGE) > uint(0));
	float isNotAverage = 1-isAverage;
	//it is none of those if:
	//* More than one of them is true
	//* All of them are false
	float isNoneofThose = isTint * isDarken * isAverage + isNotTint * isAverage * isDarken + isTint * isNotAverage * isDarken + isTint * isAverage * isNotDarken + isNotTint * isNotAverage * isNotDarken;
	float isNotNoneofThose = 1-isNoneofThose;
	
	//Calc finalcolor;
	finalcolor = (primary_color + secondary_color) * isTint * isNotNoneofThose + (primary_color - secondary_color) * isDarken * isNotNoneofThose + vec3((primary_color.x + secondary_color.x)/2.0,(primary_color.y + secondary_color.y)/2.0,(primary_color.z + secondary_color.z)/2.0) * isAverage * isNotNoneofThose + primary_color * isNoneofThose;
	
	
	
	// (diffuse component * texture) + specular
	// gl_FragData[0] = mix(vec4(finalcolor,specreflectivity),vec4(texture(worldaroundme,reflect(-vert_to_camera, usefulNormal)).xyz, specreflectivity),specreflectivity/2.0); //Lol
	vec4 cubemapData = texture(worldaroundme,reflect(-vert_to_camera, UnitNormal));
	
	
	gl_FragData[0] = vec4(finalcolor,specreflectivity);
	
	// gl_FragData[1] = vec4(UnitNormal,specdamp); //Normals. Specular dampening goes as high as 128 in OpenGL Immediate Mode, so it has to be allowed up there. Note that we are using 32 bit floating-point accuracy, so dividing will not seriously reduce our abilities with regards to specdamp


	//vec3 adjusted_world = worldout - CameraPos;
		//gl_FragData[2] = vec4(adjusted_world.x/jafar + 0.5 , adjusted_world.y/jafar + 0.5, adjusted_world.z/jafar + 0.5, emissivity/2.0 + 0.5);
		// gl_FragData[2] = vec4(worldout, emissivity/2.0 + 0.5);
		// gl_FragData[3] = vec4(diffusivity, cubemapData.xyz * enableCubeMapReflections * specreflectivity * specdamp/128.0); //Not using ambient.

}
