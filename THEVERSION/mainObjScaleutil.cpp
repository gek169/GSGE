

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
	float scalex = 1.0;
	float scaley = 1.0;
	float scalez = 1.0;
	bool exportColors = false;
	for (int i = 1; i < argc; i++) {
		carg = std::string(argv[i]);
		if (larg == "-s") {
			scalex *= GkAtof(carg.c_str());
			scaley *= GkAtof(carg.c_str());
			scalez *= GkAtof(carg.c_str());
		}
		if (larg == "-sx")
			scalex *= GkAtof(carg.c_str());
		if (larg == "-sy")
			scaley *= GkAtof(carg.c_str());
		if (larg == "-sz")
			scalez *= GkAtof(carg.c_str());
		if (larg == "-i")
			ifilename = carg;
		if (larg == "-o")
			ofilename = carg;
		if (carg == "-h") {
			std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -s scaleall -sx scalex -sy scaley -sz scalez -o Outfile.obj" << std::endl;
			return 0;
		}
		larg = carg;
	}
	if (ifilename == "" || ofilename == "") {
		std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -s scaleall -sx scalex -sy scaley -sz scalez -o Outfile.obj" << std::endl;
		return 1;
	}
	OBJModel myObj(ifilename);
	loadedModel = myObj.toIndexedModel();
	exportColors = myObj.hasVertexColors;
	loadedModel.validate();
	loadedModel.removeUnusedPoints();
	Transform b;
	b.setScale(glm::vec3(scalex, scaley, scalez));
	loadedModel.applyTransform(b.getModel());
	if (loadedModel.indices.size() == 0 || loadedModel.positions.size() == 0) // Invalid model
		return 1;
	std::ofstream outfile;
	outfile.open(ofilename, std::ios::trunc);
	if (outfile.is_open()) {
		std::cout << "\nExporting scaled model..." << std::endl;
		outfile << loadedModel.exportToString(exportColors);
	} else {
		return 1;
		outfile.close();
	}
	outfile.close();
	return 0;
}
