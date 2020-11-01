

#include "obj_loader.h"
#include "texture.h"
#include <fstream>
#include <iostream>
using namespace gekRender;

int main(int argc, char** argv) {
	IndexedModel loadedModel;
	std::string ifilename = "";
	std::string texfilename = "";
	std::string larg = "";
	std::string carg = "";
	int iw = 0, ih = 0, ic = 3;
	for (int i = 1; i < argc; i++) {
		carg = std::string(argv[i]);
		if (larg == "-o")
			texfilename = carg;
		if (larg == "-i")
			ifilename = carg;
		if (carg == "-h") {
			std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -o outfile.txt" << std::endl;
			return 0;
		}
		larg = carg;
	}
	if (ifilename == "" || texfilename == "") {
		std::cout << "\nUsage: " << argv[0] << " -i Objfile.obj -t texture.png -o Outfile.obj" << std::endl;
		return 1;
	}
	OBJModel myObj(ifilename);
	loadedModel = myObj.toIndexedModel();
	loadedModel.validate();
	loadedModel.removeUnusedPoints();
	if (loadedModel.indices.size() == 0 || loadedModel.positions.size() == 0) // Invalid model
		return 1;

    std::string filecontent = "\n";
	std::ofstream outfile;
	outfile.open(texfilename, std::ios::trunc);
    if(outfile.is_open()){
        for(int i = 0; i+2 < loadedModel.indices.size(); i+=3){
            glm::vec3 p1,p2,p3;
            p1 = loadedModel.positions[loadedModel.indices[i]];
            p2 = loadedModel.positions[loadedModel.indices[i+1]];
            p3 = loadedModel.positions[loadedModel.indices[i+2]];
            outfile << std::string("/* Triangle ") << std::to_string(i) << std::string(" = */\n");
            outfile << std::to_string(p1.x) << std::string(", ") << std::to_string(p1.y) << std::string(", ") << std::to_string(p1.z) << ",\n";
            outfile << std::to_string(p2.x) << std::string(", ") << std::to_string(p2.y) << std::string(", ") << std::to_string(p2.z) << ",\n";
            outfile << std::to_string(p3.x) << std::string(", ") << std::to_string(p3.y) << std::string(", ") << std::to_string(p3.z) << ",\n";            
        }
        outfile.close();
    }
	return 0;
}
