#include "obj_loader.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

namespace gekRender {

static bool CompareOBJIndexPtr(const OBJIndex* a, const OBJIndex* b);
static inline unsigned int FindNextChar(unsigned int start, const char* str, unsigned int length, char token);
static inline unsigned int ParseOBJIndexValue(const std::string& token, unsigned int start, unsigned int end);
static inline float ParseOBJFloatValue(const std::string& token, unsigned int start, unsigned int end);
static inline std::vector<std::string> SplitString(const std::string& s, char delim);

OBJModel::OBJModel(const std::string& fileName, bool isFileContent){
	if(isFileContent){
		myFileName = "";
		parseOBJFile(fileName);
	} else {
		myFileName = fileName;
		std::ifstream file; file.open(fileName.c_str());
		if(file.is_open())
			parseOBJFile(
				std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()))
			);
		else {
			caughtError = true;
			Error_Text = "Unable to open file " + fileName;
		}
	}
}

void OBJModel::parseOBJFile(const std::string& fileContent){
	hasUVs = false;
	hasNormals = false;
	hasVertexColors = false;
	smoothshading = true; // this is the default value.
	renderflags = 0;
	std::stringstream file(fileContent);
	//~ file.open(fileName.c_str());
	int numOs = 0; // Used to avoid 2nd, 3rd objects
	std::string line;
	if (file.good()) {
		
		int current_bone_index = -1; // Invalid
		while (file.good() && numOs <= 1) {
			getline(file, line);
			unsigned int lineLength = line.length();

			if (lineLength < 2)
				continue;

			const char* lineCStr = line.c_str();

			switch (lineCStr[0]) {
			case 'v':
				if (lineCStr[1] == 't') {
					this->uvs.push_back(ParseOBJVec2(line));
					hasUVs = true;
				} else if (lineCStr[1] == 'n') {
					this->normals.push_back(ParseOBJVec3(line));
					hasNormals = true;
				} else if (lineCStr[1] == ' ' || lineCStr[1] == '\t')
					this->vertices.push_back(ParseOBJVec3(line));
				else if (lineCStr[1] == 'c') // Bada bing bada boom
				{
					this->vertcolors.push_back(ParseOBJVec3(line));
					hasVertexColors = true;
				}
				/*std::cout << "\n We are detecting colors in this file...";*/

				break;
			case 'f':
				CreateOBJFace(line);
				break;
			case 'o': // Now compatible with OBJ files with multiple objects in
					  // them!!!
				numOs++;
				break;
			case 's':
				if (lineLength > 3 && lineCStr[3] == 'f') // Testing for the second f
					smoothshading = false;
				else if (lineLength > 3 && lineCStr[3] == 'n') // testing for the n in on
					smoothshading = true;
				break;
			case '#': // Handle GKMODE comments
				if (lineLength > 7 && lineCStr[1] == 'G' && lineCStr[2] == 'K' && lineCStr[3] == 'M' && lineCStr[4] == 'O' && lineCStr[5] == 'D' &&
					lineCStr[6] == 'E' && lineCStr[7] == ' ') {
					std::string gay = line;
					std::vector<std::string> tokens = SplitString(gay, ' '); // Remember: the first token will be #GKMODE_
																			 // (space not underscore) and the second will
																			 // be the renderflag or something.
					if (tokens.size() > 1) {
						if (tokens[1] == "GK_RENDER") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_RENDER;
							//~ std::cout << "\nGK_RENDER";
						} else if (tokens[1] == "GK_TEXTURED") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_TEXTURED;
							//~ std::cout << "\nGK_TEXTURED is IN " << myFileName;
							std::cout << "\nrenderflags = " << renderflags;
						} else if (tokens[1] == "GK_COLORED") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_COLORED;
							// std::cout << "\nGK_COLORED";
						} else if (tokens[1] == "GK_FLAT_NORMAL") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_FLAT_NORMAL;
							// std::cout << "\nGK_FLAT_NORMAL";
						} else if (tokens[1] == "GK_FLAT_COLOR") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_FLAT_COLOR;
							// std::cout << "\nGK_FLAT_COLOR";
						} else if (tokens[1] == "GK_COLOR_IS_BASE") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_COLOR_IS_BASE;
							// std::cout << "\nGK_COLOR_IS_BASE";
						} else if (tokens[1] == "GK_TINT") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_TINT;
							//~ std::cout << "\nGK_TINT";
						} else if (tokens[1] == "GK_DARKEN") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_DARKEN;
							// std::cout << "\nGK_DARKEN";
						} else if (tokens[1] == "GK_AVERAGE") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_AVERAGE;
							// std::cout << "\nGK_AVERAGE";
						} else if (tokens[1] == "GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR;
							// std::cout << "\nGK_TEXTURE_ALPHA_REPLACE";
						} else if (tokens[1] == "GK_ENABLE_ALPHA_CULLING") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_ENABLE_ALPHA_CULLING;
							// std::cout << "\nGK_ENABLE_ALPHA_CULLING";
						} else if (tokens[1] == "GK_BONE_ANIMATED") {
							hadRenderFlagsInFile = true;
							renderflags = renderflags | GK_BONE_ANIMATED;
							// std::cout << "\nGK_BONE_ANIMATED";
						}
					}
				}

				break; // End of case: comment
			default:
				break;
			};
		}
	}
}

std::string IndexedModel::exportToString(bool exportColors) {
	std::string Result = "#Exported by gekRender\n";
	Result.append("o ExportedIndexedModel\n");
	// First, export the GKMODE flags
	if ((renderflags & GK_RENDER) > 0)
		Result.append("#GKMODE GK_RENDER\n");
	if ((renderflags & GK_TEXTURED) > 0)
		Result.append("#GKMODE GK_TEXTURED\n");
	if ((renderflags & GK_COLORED) > 0)
		Result.append("#GKMODE GK_COLORED\n");
	if ((renderflags & GK_FLAT_NORMAL) > 0)
		Result.append("#GKMODE GK_FLAT_NORMAL\n");
	if ((renderflags & GK_FLAT_COLOR) > 0)
		Result.append("#GKMODE GK_FLAT_COLOR\n");
	if ((renderflags & GK_COLOR_IS_BASE) > 0)
		Result.append("#GKMODE GK_COLOR_IS_BASE\n");
	if ((renderflags & GK_TINT) > 0)
		Result.append("#GKMODE GK_TINT\n");
	if ((renderflags & GK_DARKEN) > 0)
		Result.append("#GKMODE GK_DARKEN\n");
	if ((renderflags & GK_AVERAGE) > 0)
		Result.append("#GKMODE GK_AVERAGE\n");
	if ((renderflags & GK_ENABLE_ALPHA_CULLING) > 0)
		Result.append("#GKMODE GK_ENABLE_ALPHA_CULLING\n");
	if ((renderflags & GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR) > 0)
		Result.append("#GKMODE GK_TEXTURE_ALPHA_REPLACE_PRIMARY_COLOR\n");
	if ((renderflags & GK_BONE_ANIMATED) > 0)
		Result.append("#GKMODE GK_BONE_ANIMATED\n");
	// Next, export positions, making no attempt at optimization, so that
	// exporting the f lines will be super simple
	for (size_t i = 0; i < positions.size(); i++) {
		Result.append("v ");
		Result.append(std::to_string(positions[i].x)); // X
		Result.append(" ");
		Result.append(std::to_string(positions[i].y)); // Y
		Result.append(" ");
		Result.append(std::to_string(positions[i].z)); // Z
		Result.append("\n");						   // NEWLINE
	}
	// Export texture coordinates... vt lines
	for (size_t i = 0; i < texCoords.size(); i++) {
		Result.append("vt ");
		Result.append(std::to_string(texCoords[i].x)); // X
		Result.append(" ");
		Result.append(std::to_string(texCoords[i].y)); // Y
		Result.append("\n");						   // NEWLINE
	}
	// Export normals... vn lines
	for (size_t i = 0; i < normals.size(); i++) {
		Result.append("vn ");
		Result.append(std::to_string(normals[i].x)); // X
		Result.append(" ");
		Result.append(std::to_string(normals[i].y)); // Y
		Result.append(" ");
		Result.append(std::to_string(normals[i].z)); // Z
		Result.append("\n");						 // NEWLINE
	}
	// Export Colors but only if we are doing that
	if (exportColors)
		for (size_t i = 0; i < colors.size(); i++) {
			Result.append("vc ");
			Result.append(std::to_string(colors[i].x)); // X
			Result.append(" ");
			Result.append(std::to_string(colors[i].y)); // Y
			Result.append(" ");
			Result.append(std::to_string(colors[i].z)); // Z
			Result.append("\n");						// NEWLINE
		}
	// Export Indices
	size_t indices_I = 0;
	while (indices_I < indices.size()) {
		size_t human_indices_index = indices_I + 1;
		if (human_indices_index % 3 == 1)
			Result.append("f ");
		// Make the slash thingie
		Result.append(std::to_string(indices[indices_I] + 1)); // POSITION
		Result.append("/");
		Result.append(std::to_string(indices[indices_I] + 1)); // TEXCOORD
		Result.append("/");
		Result.append(std::to_string(indices[indices_I] + 1)); // NORMAL
		if (exportColors) {
			Result.append("/");
			Result.append(std::to_string(indices[indices_I] + 1)); // COLOR
		}
		if (human_indices_index % 3 != 0) // Bug fix I think.
			Result.append(" ");
		// Increment indices_I
		indices_I++;
		// But if the human indices index (which HAS NOT CHANGED) is divisible
		// by three, we must end the line
		if (human_indices_index % 3 == 0)
			Result.append("\n");
	}
	return Result;
}
bool IndexedModel::calcAABB(float& _x, float& _y, float& _z) {
	if (positions.size() < 1)
		return false;
	float out_x = positions[0].x * positions[0].x;
	float out_y = positions[0].y * positions[0].y;
	float out_z = positions[0].z * positions[0].z;

	// find the largest of each
	for (size_t i = 0; i < positions.size(); i++) {
		float curr_x = positions[i].x * positions[i].x;
		float curr_y = positions[i].y * positions[i].y;
		float curr_z = positions[i].z * positions[i].z;
		if (curr_x > out_x)
			out_x = curr_x;
		if (curr_y > out_y)
			out_y = curr_y;
		if (curr_z > out_z)
			out_z = curr_z;
	}
	_x = 2 * (float)sqrt(out_x);
	_y = 2 * (float)sqrt(out_y);
	_z = 2 * (float)sqrt(out_z);
	return true;
}
void IndexedModel::applyTransform(glm::mat4 _transform, float w_component) {
	for (size_t i = 0; i < positions.size() && i < normals.size(); i++) {
		// Positions
		glm::vec4 pos = glm::vec4(positions[i], w_component);
		pos = _transform * pos;
		positions[i] = glm::vec3(pos.x, pos.y, pos.z);
		// Normals
		glm::vec4 normie = glm::vec4(normals[i], 0.0);
		normie = _transform * normie;
		normie = glm::vec4(glm::normalize(glm::vec3(normie)), 0.0);
		normals[i] = glm::vec3(normie.x, normie.y, normie.z);
	}
}
float IndexedModel::calcRadius() { // Unscaled
	if (positions.size() < 1)
		return 0.0;
	float LargestRadius = glm::length(positions[0]);
	for (size_t i = 0; i < positions.size(); i++) {
		float rad = glm::length(positions[i]);
		if (rad > LargestRadius)
			LargestRadius = rad;
	}
	return LargestRadius;
}
void IndexedModel::validate() {
	bool requireRecalcNormals = false;
	if (!positions.size()) // No positions
		return;
	// First, ensure that texCoords and Normals and Colors are
	// Filled to equally to positions.
	for (unsigned int& index : indices)
		if (index > positions.size())
			index = positions.size() - 1;
	while (texCoords.size() < positions.size())
		texCoords.push_back(glm::vec2(0, 0));
	while (normals.size() < positions.size()) {
		normals.push_back(glm::vec3(0, 0, 0));
		requireRecalcNormals = true;
	}
	while (colors.size() < positions.size())
		colors.push_back(glm::vec3(0, 0, 0));
	if (requireRecalcNormals) {
		for (glm::vec3& n : normals)
			n = glm::vec3(0, 0, 0);
		calcNormals();
	}
	//~ std::cout << "\nVALIDATION COMPLETE!" << std::endl;
}
void IndexedModel::operator+=(const IndexedModel& Other) {
	unsigned int oldMaxIndex = positions.size();
	validate();
	for (size_t i = 0; i < Other.positions.size(); i++)
		positions.push_back(Other.positions[i]);
	for (size_t i = 0; i < Other.normals.size(); i++)
		normals.push_back(Other.normals[i]);
	for (size_t i = 0; i < Other.texCoords.size(); i++)
		texCoords.push_back(Other.texCoords[i]);
	for (size_t i = 0; i < Other.colors.size(); i++)
		colors.push_back(Other.colors[i]);
	for (size_t i = 0; i < Other.indices.size(); i++)
		indices.push_back(oldMaxIndex + Other.indices[i]);
}
void IndexedModel::removeUnusedPoints() {
	validate();
	std::vector<bool> used;
	while (used.size() < positions.size())
		used.push_back(false);
	for (unsigned int index : indices)
		if (index < used.size())
			used[index] = true;
	for (size_t i = used.size() - 1; i < used.size(); i--) // Loops back around to SUPER BIG NUMBER
	{
		//~ std::cout << "\nLooping through used..." << std::endl;
		if (!(used[i])) { // NOTE: this is pretty much the exact code we need to
						  // remove a position. Except whole tris should be
						  // REMOVED.
			// We need to remove the position, normal,
			// texcoord, and color.
			// Then subtract 1 from every index in indices greater than
			// or equal to i.
			{
				auto& vec = positions;
				vec.erase(vec.begin() + i);
			}
			{
				auto& vec = normals;
				vec.erase(vec.begin() + i);
			}
			{
				auto& vec = colors;
				vec.erase(vec.begin() + i);
			}
			{
				auto& vec = texCoords;
				vec.erase(vec.begin() + i);
			}
			for (unsigned int& index : indices) {
				if (index >= i)
					index--;
			}
		}
		if (i == 0)
			break; // Avoid going to big numbers
	}
	//~ std::cout << "\nEOF remove unused points." << std::endl;
}
void IndexedModel::removeDegenerateTris() {
	unsigned int numTris = indices.size() / 3;
	// Test for degenerate indices not creating a triangle at the tail end.
	removeTriangle(numTris);
	// Remove triangles that form straight lines.

	for (size_t tri_index_human = 1; tri_index_human <= indices.size() / 3; tri_index_human++) {
		glm::mat3x3 tripos = getTrianglePositions(tri_index_human - 1);
		// Remove triangles that form straight lines or are singular points.
		if (vec3ApproxEqual(tripos[0], tripos[1], 0.0001) || vec3ApproxEqual(tripos[1], tripos[2], 0.0001) || vec3ApproxEqual(tripos[0], tripos[2], 0.0001)) {
			//~ std::cout << "Removed for reason: Duplicate Points in Tri" <<
			// std::endl;
			removeTriangle(tri_index_human - 1);
			tri_index_human--; // We'll have to re-evaluate this index.
			continue;		   // Don't bother with the other checks we already know it's
							   // degenerate.
		}
		// Remove duplicate triangles. Same center and normal pointing in same
		// direction. A mesh with, say, 2 triangles where one was exactly  a
		// scale of the other would fail this test But any normal-use-case mesh
		// will be fine.
		for (size_t tri2_index_human = tri_index_human + 1; tri2_index_human <= indices.size() / 3; tri2_index_human++) {
			//~ std::cout << "\nTRI2_INDEX_HUMAN: " << tri2_index_human <<
			// std::endl;
			glm::mat3x3 tri2pos = getTrianglePositions(tri2_index_human - 1);
			glm::vec3 avg1, avg2;
			avg1 = tripos[0] + tripos[1] + tripos[2];
			avg1 *= 1.0 / 3.0;
			avg2 = tri2pos[0] + tri2pos[1] + tri2pos[2];
			avg2 *= 1.0 / 3.0;
			glm::vec3 norm1 = calcTriNorm(tripos);
			glm::vec3 norm2 = calcTriNorm(tri2pos);
			if ((glm::dot(norm1, norm2) > 0.98) && vec3ApproxEqual(avg1, avg2, 0.0001)) {
				//~ std::cout << "Removed for reason: Duplicate Triangle." <<
				// std::endl;
				removeTriangle(tri2_index_human - 1);
				tri2_index_human--; // We'll have to re-evaluate this index.
			}
		}
	}
}

glm::mat3x3 IndexedModel::getTrianglePositions(unsigned int tri_index) { // Assumes model is validated.
	unsigned int r = tri_index * 3;
	if (r + 2 >= indices.size()) { // Invalid triangle
		const float p = std::numeric_limits<float>::max();
		return glm::mat3x3(glm::vec3(p, p, p), glm::vec3(p, p, p), glm::vec3(p, p, p));
	}
	glm::mat3x3 retval;
	retval[0] = positions[indices[r]];
	r++;
	retval[1] = positions[indices[r]];
	r++;
	retval[2] = positions[indices[r]];
	r++;
	return retval;
}
glm::vec3 IndexedModel::calcTriNorm(glm::mat3x3 pos) {
	glm::vec3 v1 = pos[1] - pos[0];
	glm::vec3 v2 = pos[2] - pos[0];
	return glm::normalize(glm::cross(v1, v2));
}
void IndexedModel::removeTriangle(unsigned int tri_index) {
	unsigned int r = tri_index * 3;
	auto& b = indices; // faster coding
	if (r < b.size())
		b.erase(b.begin() + r);
	if (r < b.size())
		b.erase(b.begin() + r);
	if (r < b.size())
		b.erase(b.begin() + r);
}

void IndexedModel::pushTri(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec2 t1, glm::vec2 t2, glm::vec2 t3, glm::vec3 c1, glm::vec3 c2, glm::vec3 c3,
						   bool uniquePositions) {
	size_t i1, i2, i3;
	if (!uniquePositions || getIndexOfPoint(p1) >= positions.size()) {
		i1 = positions.size();
		positions.push_back(p1);
		texCoords.push_back(t1);
		colors.push_back(c1);
		normals.push_back(glm::vec3(0, 0, 0));
	} else {
		i1 = getIndexOfPoint(p1);
	}

	if (!uniquePositions || getIndexOfPoint(p2) >= positions.size()) {
		i2 = positions.size();
		positions.push_back(p2);
		texCoords.push_back(t2);
		colors.push_back(c2);
		normals.push_back(glm::vec3(0, 0, 0));
	} else {
		i2 = getIndexOfPoint(p2);
	}

	if (!uniquePositions || getIndexOfPoint(p3) >= positions.size()) {
		i3 = positions.size();
		positions.push_back(p3);
		texCoords.push_back(t3);
		colors.push_back(c3);
		normals.push_back(glm::vec3(0, 0, 0));
	} else {
		i3 = getIndexOfPoint(p3);
	}
	indices.push_back(i1);
	indices.push_back(i2);
	indices.push_back(i3);
}
// Merge identical positions.
void IndexedModel::MaximizeSmoothing() {
	validate();
	removeDegenerateTris();
	removeUnusedPoints();
	IndexedModel futureMe;
	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		//~ std::cout <<
		size_t i1 = indices[i], i2 = indices[i + 1], i3 = indices[i + 2];
		glm::mat3x3 pos = getTrianglePositions(i / 3);
		futureMe.pushTri(pos[0], pos[1], pos[2], texCoords[i1], texCoords[i2], texCoords[i3], colors[i1], colors[i2], colors[i3], true);
	}
	positions = futureMe.positions;
	texCoords = futureMe.texCoords;
	normals = futureMe.normals;
	colors = futureMe.colors;
	indices = futureMe.indices;
	calcNormals();
}
// Split all triangles.
void IndexedModel::MinimizeSmoothing() {
	validate();
	removeDegenerateTris();
	removeUnusedPoints();
	IndexedModel futureMe;
	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		size_t i1 = indices[i], i2 = indices[i + 1], i3 = indices[i + 2];
		glm::mat3x3 pos = getTrianglePositions(i / 3);
		futureMe.pushTri(pos[0], pos[1], pos[2], texCoords[i1], texCoords[i2], texCoords[i3], colors[i1], colors[i2], colors[i3], false);
	}
	positions = futureMe.positions;
	texCoords = futureMe.texCoords;
	normals = futureMe.normals;
	colors = futureMe.colors;
	indices = futureMe.indices;
	calcNormals();
}

void IndexedModel::calcNormals() // Only works on a valid indexed model.
{
	for (unsigned int i = 0; i < indices.size(); i += 3) {
		int i0;
		int i1;
		int i2;

		i0 = indices[i];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		glm::vec3 v1 = positions[i1] - positions[i0];
		glm::vec3 v2 = positions[i2] - positions[i0];

		glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
		normals[i0] += normal;
		normals[i1] += normal;
		normals[i2] += normal;
	}
	for (unsigned int i = 0; i < normals.size(); i++)
		normals[i] = glm::normalize(normals[i]);
}

IndexedModel createBox(float Xdim, float Ydim, float Zdim, glm::vec3 color, glm::vec3 texScale) {
	unsigned int index = 0;
	float Xdimt = Xdim / texScale.x;
	float Ydimt = Ydim / texScale.y;
	float Zdimt = Zdim / texScale.z;
	IndexedModel returnval = IndexedModel();
	returnval.hadRenderFlagsInFile = true;
	returnval.renderflags = GK_RENDER | GK_TEXTURED | GK_COLORED | GK_TINT;
	// std::cout << "\n Made IndexedModel";
	// Side 1, Triangle 1, Facing toward -Y
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, Zdim));   // Upper Right Point
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, Zdim));  // Upper Left Point
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, -Zdim)); // bottom Left Point
	// Push back 3! (UPPER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));

	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));

	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, -Zdimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	// Side 1, Triangle 2, Facing toward -Y
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, -Zdim)); // bottom Left Point
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, -Zdim));  // bottom Right Point
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, Zdim));   // Upper Right Point
	// Push back 3! (LOWER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, -Zdimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, -Zdimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));

	// The exact same thing but +Ydim instead of -Ydim, and one dimension has to
	// be flipped

	// Side 2, Triangle 1, Facing toward +Y
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, Zdim)); // Upper Right Point
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, Zdim));  // Upper Left Point
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, -Zdim)); // bottom Left Point
	// Push back 3! (UPPER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, -Zdimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	// Side 2, Triangle 2, Facing toward +Y
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, -Zdim));  // bottom Left Point
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, -Zdim)); // bottom Right Point
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, Zdim));  // Upper Right Point
	// Push back 3! (LOWER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, -Zdimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, -Zdimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));

	// Now, we swap Zdim and Ydim for switching around...

	// Side 3, Triangle 1, Facing toward Z
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, Zdim));   // Upper Right Point
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, Zdim));  // Upper Left Point
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, Zdim)); // bottom Left Point
	// Push back 3! (UPPER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, -Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	// Side 1, Triangle 2, Facing toward Z
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, Zdim)); // bottom Left Point
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, Zdim));  // bottom Right Point
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, Zdim));   // Upper Right Point
	// Push back 3! (LOWER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, -Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, -Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));

	// The exact same thing but +Zdim instead of -Zdim, and one dimension has to
	// be flipped

	// Side 3, Triangle 1, Facing toward -Z
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, -Zdim)); // Upper Right Point
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, -Zdim));  // Upper Left Point
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, -Zdim)); // bottom Left Point
	// Push back 3! (UPPER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, -Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	// Side 3, Triangle 2, Facing toward -Z
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, -Zdim));  // bottom Left Point
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, -Zdim)); // bottom Right Point
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, -Zdim));  // Upper Right Point
	// Push back 3! (LOWER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, -Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, -Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Xdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));

	// Now, we return Z to before, but X is swapped with Y

	// Bottom Left is actually bottom right.
	// Upper Left is actually bottom left.
	// Upper Right is actually Upper Left. //DONE
	// Bottom Right is actually Upper Right
	// Side 1, Triangle 1, Facing toward -Y
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, Zdim));   // Upper Right Point
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, Zdim));  // Upper Left Point
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, -Zdim)); // bottom Left Point
	// Push back 3! (UPPER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Zdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	// Side 1, Triangle 2, Facing toward +X
	returnval.positions.push_back(glm::vec3(Xdim, -Ydim, -Zdim)); // bottom Left Point
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, -Zdim));  // bottom Right Point
	returnval.positions.push_back(glm::vec3(Xdim, Ydim, Zdim));   // Upper Right Point
	// Push back 3! (LOWER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Zdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Zdimt, Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));

	// The exact same thing but +Ydim instead of -Ydim, and one dimension has to
	// be flipped

	// Side 6, Triangle 1, Facing toward -X
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, Zdim)); // Upper Right Point
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, Zdim));  // Upper Left Point
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, -Zdim)); // bottom Left Point
	// Push back 3! (UPPER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Zdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Zdimt, Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	// Side 6, Triangle 2, Facing toward -X
	returnval.positions.push_back(glm::vec3(-Xdim, Ydim, -Zdim));  // bottom Left Point
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, -Zdim)); // bottom Right Point
	returnval.positions.push_back(glm::vec3(-Xdim, -Ydim, Zdim));  // Upper Right Point
	// Push back 3! (LOWER TRI)
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, Ydimt));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(0, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));
	returnval.indices.push_back(index);
	index++;
	returnval.colors.push_back(glm::vec3(0, 0, 0));
	returnval.texCoords.push_back(glm::vec2(Zdimt, 0));
	returnval.normals.push_back(glm::vec3(0, 0, 0));

	returnval.colors.clear();
	while (returnval.colors.size() < returnval.positions.size())
		returnval.colors.push_back(color);

	// Finally, calcNormals and get out of here!
	// std::cout << "\n Before calcNormals";
	returnval.calcNormals();
	// std::cout << "\n Made a Box!!!";
	return returnval;
};

IndexedModel getErrorShape(std::string error_name) { // Creates a SOLID PINK cube.
	auto retval = createBox(1, 1, 1, glm::vec3(1, 0.5, 0.5), glm::vec3(1, 1, 1));
	retval.myFileName = error_name;
	return retval;
}

IndexedModel OBJModel::toIndexedModel() {
	//~ std::cout << "\nStarting ToIndexedModel!" << std::endl;
	IndexedModel result;
	IndexedModel normalModel;
	if (caughtError) {
		std::string name = "ERROR loading model ";
		name += myFileName;
		name += " Error Text: ";
		name += Error_Text;
		return getErrorShape(name);
	}
	result.renderflags = renderflags;
	normalModel.renderflags = renderflags;
	result.hadRenderFlagsInFile = hadRenderFlagsInFile;

	normalModel.hadRenderFlagsInFile = hadRenderFlagsInFile; // We have to do it to both!
	result.myFileName = myFileName;
	normalModel.myFileName = myFileName;
	result.smoothshading = smoothshading;
	normalModel.smoothshading = smoothshading;
	//~ std::cout << "\nMarker 1!" << std::endl;
	unsigned int numIndices = OBJIndices.size();

	//~ std::vector<OBJIndex*> indexLookup;

	//~ for (unsigned int i = 0; i < numIndices; i++)
	//~ indexLookup.push_back(&OBJIndices[i]);
	//~ std::cout << "\nMarker 2!" << std::endl;
	//~ std::sort(indexLookup.begin(), indexLookup.end(), CompareOBJIndexPtr);
	//~ std::cout << "\nMarker 3!" << std::endl;
	if(OBJIndices.size() < 3){ //Has no faces. Point cloud. Just copy the data over.
		result.positions = vertices;
		result.texCoords = uvs;
		result.normals = normals;
		result.colors = vertcolors;
		result.validate();
		return result;
	}
	for (unsigned int i = 0; i < numIndices; i++) {
		//~ std::cout << "\nMarker A!" << std::endl;
		OBJIndex& currentIndex = OBJIndices[i];
		bool canSave = false; // See if we can merge verts.
		glm::vec3 currentPosition;
		glm::vec2 currentTexCoord;
		glm::vec3 currentNormal;
		glm::vec3 currentVertColor;

		if (vertices.size() > currentIndex.vertexIndex)
			currentPosition = vertices[currentIndex.vertexIndex];
		else {
			currentPosition = glm::vec3(0);
			//~ std::cout << "\nSERIOUS issue!" << std::endl;
			//~ std::cout << "\nVertex Index is " << currentIndex.vertexIndex << "\nBut we only have"
			//~ << vertices.size() << "\nVerts!" << std::endl;
		}
		if (hasUVs && uvs.size() > currentIndex.uvIndex)
			currentTexCoord = uvs[currentIndex.uvIndex];
		else
			currentTexCoord = glm::vec2(0, 0);
		if (hasNormals && normals.size() > currentIndex.normalIndex)
			currentNormal = normals[currentIndex.normalIndex];
		else
			currentNormal = glm::vec3(0, 0, 0);

		if (hasVertexColors && vertcolors.size() > currentIndex.vertColorIndex) {
			currentVertColor = vertcolors[currentIndex.vertColorIndex];
		} else
			currentVertColor = glm::vec3(0, 0, 0);
		//~ std::cout << "\nMarker B!" << std::endl;
		// Stupidly push back all the data onto each vector, and then push back the index.

		//~ for(size_t b = 0; b < result.positions.size(); b++){
		//~ if((result.positions[b] == currentPosition) &&
		//~ (result.texCoords[b] == currentTexCoord) &&
		//~ (result.normals[b] == currentNormal) &&
		//~ (result.colors[b] == currentVertColor) &&
		//~ !(!hasNormals && !smoothshading)
		//~ ) //It has all the same shit
		//~ {canSave = true; result.indices.push_back(b); break;}
		//~ }
		if (!canSave) {
			uint ti = result.positions.size();
			result.positions.push_back(currentPosition);
			result.texCoords.push_back(currentTexCoord);
			result.normals.push_back(currentNormal);
			result.colors.push_back(currentVertColor);
			result.indices.push_back(ti);
		}
	}
	if (!hasNormals) // Smart!
	{
		for (unsigned int i = 0; i < result.positions.size(); i++)
			result.normals[i] = glm::vec3(0, 0, 0);
		result.calcNormals();
	}
	//~ std::cout << "\nEnding ToIndexedModel!" << std::endl;
	result.validate();
	return result;
};

void OBJModel::CreateOBJFace(const std::string& line) {
	// Split into the 4 tokens, first being f, second being the first index...
	std::vector<std::string> tokens = SplitString(line, ' ');
	for (size_t i = 1; i < tokens.size() && i < 4; i++) {
		bool bad = false;
		if (tokens[i] == "") // Don't process empty indices.
			bad = true;
		if (bad) {
			std::cout << "Line that produced errors: \n" << line << std::endl;
			return;
		}
	}
	this->OBJIndices.push_back(ParseOBJIndex(tokens[1], &this->hasUVs, &this->hasNormals, &this->hasVertexColors));
	this->OBJIndices.push_back(ParseOBJIndex(tokens[2], &this->hasUVs, &this->hasNormals, &this->hasVertexColors));
	this->OBJIndices.push_back(ParseOBJIndex(tokens[3], &this->hasUVs, &this->hasNormals, &this->hasVertexColors));

	if ((int)tokens.size() > 4 && tokens[4] != "") // Quad. Second triangle.
	{
		this->OBJIndices.push_back(ParseOBJIndex(tokens[1], &this->hasUVs, &this->hasNormals, &this->hasVertexColors));
		this->OBJIndices.push_back(ParseOBJIndex(tokens[3], &this->hasUVs, &this->hasNormals, &this->hasVertexColors));
		this->OBJIndices.push_back(ParseOBJIndex(tokens[4], &this->hasUVs, &this->hasNormals, &this->hasVertexColors));
	}
}

OBJIndex OBJModel::ParseOBJIndex(const std::string& token, bool* hasUVs, bool* hasNormals, bool* hasVertColors) {
	unsigned int tokenLength = token.length();
	const char* tokenString = token.c_str();

	unsigned int vertIndexStart = 0;
	unsigned int vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, '/');

	OBJIndex result;
	result.vertexIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
	//~ if(result.vertexIndex > vertices.size())
	//~ std::cout << "\nERRONEOUS OBJ INDEX. TOKEN: \"" << token << "\"" << std::endl;
	result.uvIndex = 0;
	result.normalIndex = 0;
	result.vertColorIndex = 0;
	bool hadUV = false;
	bool hadNormal = false;
	bool hadColor = false;

	if (vertIndexEnd >= tokenLength) // handle the case of v1
		return result;

	// REGARDING TEXTURE COORDINATES:
	vertIndexStart = vertIndexEnd + 1;
	if (tokenString[vertIndexStart] != '/') // Case: No UV
	{
		vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength,
									'/'); // This is finding the wrong slash!
	} else {
		vertIndexEnd = vertIndexStart;
		vertIndexStart--;
	}

	//
	if (tokenString[vertIndexStart] != '/' && vertIndexStart < tokenLength) {
		result.uvIndex = ParseOBJIndexValue(token, vertIndexStart,
											vertIndexEnd); // Then we have a valid uvIndex
		*hasUVs = true;									   // Don't set this to true if it's not true
		hadUV = true;
	}

	if (vertIndexEnd >= tokenLength) {
		return result;
	}

	vertIndexStart = vertIndexEnd + 1;
	if (tokenString[vertIndexStart] != '/') // Case: No Normal
	{
		vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength,
									'/'); // This is finding the wrong slash!
	} else {
		vertIndexEnd = vertIndexStart;
		vertIndexStart--;
	}																		// Case: No Normal
	if (vertIndexStart < tokenLength && tokenString[vertIndexStart] != '/') // handle the v/t1//c1 case
	{
		// if (myFileName == "SPHERE_TEST.OBJ")
		// std::cout << "\n Big Uh oh.";
		result.normalIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
		*hasNormals = true;
		hadNormal = true;
	}
	if (vertIndexEnd >= tokenLength) // Make sure this is not the case: #/#//0
									 // where we are on the second slash.
	{
		return result;
	}
	vertIndexEnd++;

	vertIndexStart = vertIndexEnd;
	vertIndexEnd = tokenLength;

	if (vertIndexStart < tokenLength && tokenString[vertIndexStart] != '/') { // So long as we didn't start on a slash
		*hasVertColors = true;
		result.vertColorIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
		// if (myFileName == "SPHERE_TEST.OBJ")
		// {
		// std::cout << "\n WE HAVE COLORS!";
		// std::cout << "\n TOKEN:~" << token;
		// std::cout << "\n TOKENLENGTH: " << tokenLength;
		// std::cout << "\n VERTINDEXSTART: " << vertIndexStart;
		// std::cout << "\n VERTINDEXEND: " << vertIndexEnd;
		// std::cout << "\n COLOR INDEX: " << result.vertColorIndex;
		// }
		hadColor = true;
	}

	return result;
}

glm::vec3 OBJModel::ParseOBJVec3(const std::string& line) {
	unsigned int tokenLength = line.length();
	const char* tokenString = line.c_str();

	unsigned int vertIndexStart = 2;

	while (vertIndexStart < tokenLength) {
		if (tokenString[vertIndexStart] != ' ')
			break;
		vertIndexStart++;
	}

	unsigned int vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float x = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	vertIndexStart = vertIndexEnd + 1;
	vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float y = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	vertIndexStart = vertIndexEnd + 1;
	vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float z = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	return glm::vec3(x, y, z);

	// glm::vec3(GkAtof(tokens[1].c_str()), GkAtof(tokens[2].c_str()),
	// GkAtof(tokens[3].c_str()))
}

glm::vec2 OBJModel::ParseOBJVec2(const std::string& line) {
	unsigned int tokenLength = line.length();
	const char* tokenString = line.c_str();

	unsigned int vertIndexStart = 3;

	while (vertIndexStart < tokenLength) {
		if (tokenString[vertIndexStart] != ' ')
			break;
		vertIndexStart++;
	}

	unsigned int vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float x = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	vertIndexStart = vertIndexEnd + 1;
	vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float y = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	return glm::vec2(x, y);
}

static bool CompareOBJIndexPtr(const OBJIndex* a, const OBJIndex* b) { return a->vertexIndex < b->vertexIndex; }

static inline unsigned int FindNextChar(unsigned int start, const char* str, unsigned int length, char token) {
	unsigned int result = start;
	while (result < length) // Length actually has to be the maximum index size... if we
							// gave the traditional idea of length (E.g. array[3] which
							// is 0,1,2) then we would be bamboozleed
	{
		result++;
		if (str[result] == token)
			break;
	}

	return result;
}

static inline unsigned int ParseOBJIndexValue(const std::string& token, unsigned int start, unsigned int end) {
	return GkAtoui(token.substr(start, end - start).c_str()) - 1;
}

static inline float ParseOBJFloatValue(const std::string& token, unsigned int start, unsigned int end) {
	return GkAtof(token.substr(start, end - start).c_str());
}

static inline std::vector<std::string> SplitString(const std::string& s, char delim) {
	std::vector<std::string> elems;

	const char* cstr = s.c_str();
	unsigned int strLength = s.length();
	unsigned int start = 0;
	unsigned int end = 0;

	while (end <= strLength) {
		while (end <= strLength) {
			if (cstr[end] == delim)
				break;
			end++;
		}

		elems.push_back(s.substr(start, end - start));
		start = end + 1;
		end = start;
	}

	return elems;
}
}; // namespace gekRender
