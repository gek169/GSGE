#version 330
//out vec4 fColor[2];

in vec2 texcoord; // 0 to 1 in both directions
int iter = 50;

uniform sampler2D diffuse; //This is actually the texture unit. limit 32. This one happens to be for the literal object's texture.

vec2 mandelbrotcoords;
vec4 colors[5];
float numcolors = 5.0;
int numcolors_int = 5;

//f(z) = z^2+C does not diverge as you call it on itself eg f(f(f(z)));

//Mandelbrot code
	int mandelbrot_test(vec2 c, int maxiter, float testvalsq) { //x component is real, y is imaginary
		float zr = 0;
		float zi = 0; //Factor of i
		float lastzr = 0;
		for (int i = 0; i < maxiter; i++)
		{
			zr = (zr * zr) - (zi * zi) + c.x;
			zi = 2 * lastzr * zi + c.y;
			if(zr * zr + zi * zi > testvalsq){
				return i;
			}
			lastzr = zr;
		}
		return maxiter;
	}
	vec2 centeraroundpointonmandelbrot(vec2 locationonscreen, vec2 centerinmandelspace,vec2 range){
		//Take the location on the screen which is from 0 to 1 in x and y, and move the center, 0.5,0.5 to the centerinmandelspace. We want to scale the the coordinates relative to it 
		vec2 result;
		//Move to origin
		result.x = locationonscreen.x -0.5;
		result.y = locationonscreen.y -0.5;
		//Scale
		result.x = result.x * range.x;
		result.y = result.y * range.y;
		//Move to center of mandelbrotspace
		result.x = result.x + centerinmandelspace.x;
		result.y = result.y + centerinmandelspace.y;
		return result;
	}
vec4 lerpColor(float input){
	// float numcolors = 5.0;
	// int numcolors_int = 5;
	// distance between each colors is equal
	// We want to find the one above and the one below the input
	vec4 color1 = vec4(0,0,0,0);
	vec4 color2 = vec4(0,0,0,0);
	float somethin = 0;
	float point = input * (numcolors-1);
	if (point == float(int(point)))
	{
		return colors[int(point)];
	}
	color1 = colors[int(point)];
	color2 = colors[1+int(point)];
	somethin = point - float(int(point));
	return mix(color1, color2, input);
}


void main()
{
	//setup colors
	colors[0] = vec4(0.0, 6.0/255.0, 43.0/255.0, 0.0);
	colors[1] = vec4(140.0/255.0, 153.0/255.0, 0.0/255.0, 0.0);
	colors[2] = vec4(252.0/255.0, 194.0/255.0, 0.0/255.0, 0.0);
	colors[3] = vec4(25.0/255.0, 252.0/255.0, 0.0/255.0, 0.0);
	colors[4] = vec4(0.0, 0.0, 255.0, 0.0);
	mandelbrotcoords = centeraroundpointonmandelbrot(texcoord, vec2(-0.5,0.0), vec2(3.0,2.0));
	float mandelresult = float(mandelbrot_test(mandelbrotcoords, 255, 1000.0))/255.0;
	gl_FragColor = lerpColor(mandelresult);
	//gl_FragColor = texture2D(diffuse, texcoord);
	
}
