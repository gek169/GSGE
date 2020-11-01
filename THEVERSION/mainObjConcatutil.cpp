

#include "obj_loader.h"
#include <fstream>
#include <iostream>
#include <vector>
using namespace gekRender;

int main(int argc, char** argv) {
	IndexedModel loadedModel;
	std::vector<std::string> ifilenames;
	std::string ofilename = "";
	std::string larg = "";
	std::string carg = "";
	bool exportColors = false;
	for (int i = 1; i < argc; i++) {
		carg = std::string(argv[i]);
		if (larg == "-i")
			ifilenames.push_back(carg);
		if (larg == "-o")
			ofilename = carg;
		if (carg == "-h") {
			std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -i Objfile2.obj -o Outfile.obj" << std::endl;
			return 0;
		}
		larg = carg;
	}
	if (ifilenames.size() == 0 || ofilename == "") {
		std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -i Objfile2.obj -o Outfile.obj" << std::endl;
		return 1;
	}
	for (std::string& fname : ifilenames) {
		OBJModel myObj(fname);
		if (myObj.caughtError) {
			std::cout << "\nError. Skipping " << fname << std::endl;
			continue;
		}
		if (loadedModel.positions.size() == 0) { // First OBJ with data in it
			loadedModel = myObj.toIndexedModel();
		} else {
			loadedModel += myObj.toIndexedModel();
		}
		exportColors = exportColors || myObj.hasVertexColors;
		loadedModel.validate();
	}
	loadedModel.validate();
	loadedModel.removeUnusedPoints();
	if (loadedModel.indices.size() == 0 || loadedModel.positions.size() == 0) // Invalid model
		return 1;
	//
	std::ofstream outfile;
	outfile.open(ofilename, std::ios::trunc);
	if (outfile.is_open()) {
		std::cout << "\nExporting Concatenated OBJ model..." << std::endl;
		outfile << loadedModel.exportToString(exportColors);
	} else {
		std::cout << "\nError opening outfile." << std::endl;
		return 1;
		outfile.close();
	}
	outfile.close();
	return 0;
}
