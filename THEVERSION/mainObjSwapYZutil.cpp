

#include "obj_loader.h"
#include <fstream>
#include <iostream>
using namespace gekRender;

int main(int argc, char** argv) {
	IndexedModel loadedModel;
	std::string ifilename = "";
	std::string ofilename = "";
	std::string larg = "";
	std::string carg = "";
	bool exportColors = false;
	for (int i = 1; i < argc; i++) {
		carg = std::string(argv[i]);
		if (larg == "-i")
			ifilename = carg;
		if (larg == "-o")
			ofilename = carg;
		if (carg == "-h") {
			std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -o Outfile.obj" << std::endl;
			return 0;
		}
		larg = carg;
	}
	if (ifilename == "" || ofilename == "") {
		std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -o Outfile.obj" << std::endl;
		return 1;
	}
	OBJModel myObj(ifilename);
	loadedModel = myObj.toIndexedModel();
	exportColors = myObj.hasVertexColors;
	loadedModel.validate();
	loadedModel.removeUnusedPoints();
	//~ Transform b;
	//~ b.setScale(glm::vec3(scalex, scaley, scalez));
	//~ loadedModel.applyTransform(b.getModel());
	for (size_t i = 0; i < loadedModel.positions.size(); i++) {
		float py = loadedModel.positions[i].y;
		float pz = loadedModel.positions[i].z;
		float ny = loadedModel.normals[i].y;
		float nz = loadedModel.normals[i].z;
		loadedModel.positions[i].y = pz;
		loadedModel.positions[i].z = -py;
		loadedModel.normals[i].y = nz;
		loadedModel.normals[i].z = -ny;
	}
	if (loadedModel.indices.size() == 0 || loadedModel.positions.size() == 0) // Invalid model
		return 1;
	std::ofstream outfile;
	outfile.open(ofilename, std::ios::trunc);
	if (outfile.is_open()) {
		std::cout << "\nExporting YZ corrected model..." << std::endl;
		outfile << loadedModel.exportToString(exportColors);
	} else {
		return 1;
		outfile.close();
	}
	outfile.close();
	return 0;
}
