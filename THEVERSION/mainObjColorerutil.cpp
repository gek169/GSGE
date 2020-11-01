

#include "obj_loader.h"
#include "texture.h"
#include <fstream>
#include <iostream>
using namespace gekRender;

int main(int argc, char** argv) {
	IndexedModel loadedModel;
	std::string ifilename = "";
	std::string ofilename = "";
	std::string texfilename = "";
	std::string larg = "";
	std::string carg = "";
	unsigned char* image_data = nullptr;
	int iw = 0, ih = 0, ic = 3;
	for (int i = 1; i < argc; i++) {
		carg = std::string(argv[i]);
		if (larg == "-t")
			texfilename = carg;
		if (larg == "-i")
			ifilename = carg;
		if (larg == "-o")
			ofilename = carg;
		if (carg == "-h") {
			std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -t texture.png -o Outfile.obj" << std::endl;
			return 0;
		}
		larg = carg;
	}
	if (ifilename == "" || ofilename == "" || texfilename == "") {
		std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -t texture.png -o Outfile.obj" << std::endl;
		return 1;
	}
	OBJModel myObj(ifilename);
	loadedModel = myObj.toIndexedModel();
	loadedModel.validate();
	loadedModel.removeUnusedPoints();
	if (loadedModel.indices.size() == 0 || loadedModel.positions.size() == 0) // Invalid model
		return 1;

	image_data = Texture::stbi_load_passthrough((char*)texfilename.c_str(), &iw, &ih, &ic, 3);
	ic = 3;
	if (!image_data || iw == 0 || ih == 0)
		return 1;
	loadedModel.hadRenderFlagsInFile = true;
	loadedModel.renderflags = GK_RENDER | GK_COLOR_IS_BASE | GK_COLORED | GK_TINT;
	for (size_t i = 0; i < loadedModel.colors.size(); i++) {
		glm::vec2 samplecoord = loadedModel.texCoords[i];
		samplecoord.y *= -1; // Compatibility
		samplecoord.x *= iw;
		samplecoord.y *= ih;
		if (samplecoord.x > (float)(iw-1) && samplecoord.x < (float)(iw-1) + 0.5)
			samplecoord.x -= 0.5;
		if (samplecoord.y > (float)(ih-1) && samplecoord.y < (float)(ih-1) + 0.5)
			samplecoord.y -= 0.5;
		glm::ivec2 sampleint = glm::ivec2(samplecoord.x, samplecoord.y);
		while (sampleint.x < 0)
			sampleint.x += iw;
		while (sampleint.y < 0)
			sampleint.y += ih;
		sampleint.x %= iw;
		sampleint.y %= ih;
		int x = sampleint.x;
		int y = sampleint.y;
		glm::vec3 color = glm::vec3((float)image_data[(x + y * iw) * 3 + 0] * 1.0 / 255.0, // r
									(float)image_data[(x + y * iw) * 3 + 1] * 1.0 / 255.0, // g
									(float)image_data[(x + y * iw) * 3 + 2] * 1.0 / 255.0  // b
		);
		loadedModel.colors[i] = color;
	}

	std::ofstream outfile;
	outfile.open(ofilename, std::ios::trunc);
	if (outfile.is_open()) {
		std::cout << "\nExporting colored model..." << std::endl;
		outfile << loadedModel.exportToString(true);
	} else {
		return 1;
		outfile.close();
	}
	outfile.close();
	return 0;
}
