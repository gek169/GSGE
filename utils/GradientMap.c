#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#define OUTPUTCOMPONENTS 3
//gcc -lm GradientMap.c -o GradientMap
//Performs a similar function to the "Gradient Map" tool in Gimp
//Licensed to you under the CC0 license. https://creativecommons.org/publicdomain/zero/1.0/legalcode
//(C) gek 2020

typedef unsigned int uint;

void clampf(float* c, float minval, float maxval){
	if(*c < minval) *c = minval;
	else if (*c > maxval) *c = maxval;
}

int main(int argc, char**argv){
//USAGE ./GradientMap SOURCE.PNG R,G,B R,G,B DEST.PNG
	if(argc < 5)
	{
		printf("\nusage: %s SOURCE.PNG R,G,B R,G,B DEST.PNG FMULT FBIAS\n", argv[0]);
		return 0;
	}
	char* sourceFN = argv[1];
	char* destFN = argv[4];
	float Mult = 1.0;
	float Bias = 0.0;
	if(argc > 5) Mult = atof(argv[5]);
	if(argc > 6) Bias = atof(argv[6]);
	int sw = 0, sh = 0, sc = 0;
	unsigned char* source_data = stbi_load(sourceFN, &sw, &sh, &sc, 4);
	sc = 4; //Docs
	if(!source_data) return 1;
	unsigned char* dest_data = (unsigned char*)malloc(sw * sh * OUTPUTCOMPONENTS);
	unsigned char conv[6]; //RGB, RGB
	int i = 0;
	conv[0] = strtoull(argv[2], NULL, 10);
	while(argv[2][i] != ',') i++; i++;
	conv[1] = strtoull(argv[2]+i, NULL, 10);
	while(argv[2][i] != ',') i++; i++;
	conv[2] = strtoull(argv[2]+i, NULL, 10);
	i=0;
	conv[3] = strtoull(argv[3], NULL, 10);
	while(argv[3][i] != ',') i++; i++;
	conv[4] = strtoull(argv[3]+i, NULL, 10);
	while(argv[3][i] != ',') i++; i++;
	conv[5] = strtoull(argv[3]+i, NULL, 10);
	
	//float* avgs = (float*)malloc(sw * sh * sizeof(float));
	for(int x = 0; x < sw; x++)
	for(int y = 0; y < sh; y++)
	{
		float avg = 
		(float)source_data[(x + y * sw) * 4 + 0] * 1.0/3.0 * 1/255.0 + 
		(float)source_data[(x + y * sw) * 4 + 1] * 1.0/3.0 * 1/255.0 + 
		(float)source_data[(x + y * sw) * 4 + 2] * 1.0/3.0 * 1/255.0;
		avg *= Mult; avg += Bias;
		clampf(&avg,0,1);
		i = 0;
		dest_data[(x + y * sw)*OUTPUTCOMPONENTS + 0] = avg * conv[3] + (1-avg) * conv[0];
		dest_data[(x + y * sw)*OUTPUTCOMPONENTS + 1] = avg * conv[4] + (1-avg) * conv[1];
		dest_data[(x + y * sw)*OUTPUTCOMPONENTS + 2] = avg * conv[5] + (1-avg) * conv[2];
	}
	stbi_write_png(destFN, sw, sh, OUTPUTCOMPONENTS, dest_data, 0);
	//if(avgs) free(avgs);
	if(source_data) free(source_data);
	if(dest_data) free(dest_data);

	return 0;
}
