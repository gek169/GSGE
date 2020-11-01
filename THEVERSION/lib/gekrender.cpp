

#include "gekrender.h"
#include <cmath>
namespace gekRender {
// To be programmed...
IndexedModel createSphere(float radius, unsigned int longitudinal_complexity, unsigned int latitudinal_complexity, glm::vec3 color) {
	IndexedModel result = IndexedModel();
	//~ return createBox(radius, radius, radius, color); //Attempt to fix
	// MEGA-BUG in the game engine.
	result.myFileName = std::string("gekRender Auto-Generated Sphere with Radius: ") + std::to_string(radius);
	result.hadRenderFlagsInFile = true;
	result.renderflags = GK_RENDER | GK_COLOR_IS_BASE | GK_COLORED | GK_TINT; // GK_TEXTURED is disabled by default
	if (longitudinal_complexity < 3 || latitudinal_complexity < 3)			  // Error state
	{
		std::cout << "\nErroneous args for createSphere." << std::endl;
		return getErrorShape("//Aux/Erroneous Args for createSphere");
	}
	unsigned int index1, index2, index3; // HAS TO BE RIGHT HANDED
	for (int latitude = 0; latitude < latitudinal_complexity + 1; latitude++)
		for (int longitude = 0; longitude < longitudinal_complexity; longitude++) // Create a duplicate point at the seam
		{
			const float PI = 3.14159;
			glm::vec3 point = glm::vec3(0, 0, 0);
			glm::vec2 texcoord = glm::vec2(0, 0);
			// XYZ point. It's also the normal of the surface at that point.
			if (latitude != 0) {
				point.x = sinf(PI * latitude / latitudinal_complexity) * cosf(2 * PI * longitude / longitudinal_complexity);
				point.z = sinf(PI * latitude / latitudinal_complexity) * sinf(2 * PI * longitude / longitudinal_complexity);
				point.y = cosf(PI * latitude / latitudinal_complexity);
			} else {
				point.x = 0;
				point.z = 0;
				point.y = 1;
			}
			// Texture Coordinate
			// I think this is the mercater projection
			if (point.z == 0 && point.x == 0) // Always the same value
				texcoord.x = 1.570796;
			else if ((point.x < -0.0f) || (point.x > 0.0f))
				texcoord.x = ((float)atan2(point.x, point.z)) / (2.0f * 3.14159f) + 0.5f;
			else {
				texcoord.x = 0; // Set to zero. It's what Atan2 would do anyway.
								//~ std::cout << "\nDOMAIN ERROR ON ATAN2" << std::endl;
			}
			texcoord.y = point.y * 0.5 + 0.5;

			result.normals.push_back(point);

			point = radius * point;

			result.positions.push_back(point);

			result.texCoords.push_back(texcoord);

			result.colors.push_back(color);
		}

	// Make triangles
	for (unsigned int index = longitudinal_complexity; index < result.positions.size() - 1; index++) // Start on the second row
	{
		// Make sure to skip the duplicate points
		//~ if(index % (longitudinal_complexity + 1) == longitudinal_complexity)
		//~ continue;
		// Make the first triangle, connecting:
		// The point immediately above
		// This point
		// The point above and to the right
		index1 = index - longitudinal_complexity; // point immediately above
		index2 = index;
		index3 = index - longitudinal_complexity + 1; // Point above and to the
													  // right
		result.indices.push_back(index3);
		result.indices.push_back(index2);
		result.indices.push_back(index1);

		// Make the second triangle, connecting:
		// this point
		// the point above and to the right (Already Set)
		// the point to our right

		index1 = index;
		index2 = index + 1;
		// Taking advantage of the fact that Index3 was already set
		result.indices.push_back(index1);
		result.indices.push_back(index3);
		result.indices.push_back(index2);
	}
	result.MaximizeSmoothing();
	for (size_t i = 0; i < result.normals.size(); i++) {
		auto& norm = result.normals[i];
		//~ std::cout << "\nNorm X: " << norm.x <<
		//~ "\n     Y: " << norm.y <<
		//~ "\n     Z: " << norm.z;
		result.normals[i] = ((radius < 0) ? -1.0f : 1.0f) * glm::normalize(result.positions[i]);
	}

	return result;
}

IndexedModel createCone(float radius, float h, unsigned int numsubdiv, glm::vec3 color) {
	//~ float CoMShift = h/4; //Applied at the end.
	// Bullet physics is FUCKED so we do this instead
	glm::vec3 CoMShift(0, -h / 2, 0);
	if (numsubdiv < 3) {
		numsubdiv = 3;
		std::cout << "numsubdiv too low! Fixing..." << std::endl;
	} // Minimum sub divisions.
	IndexedModel ConePart;
	IndexedModel Bottom_Plate_Part;
	glm::vec3 top_point(0, h, 0);
	top_point += CoMShift;
	glm::vec2 tc = glm::vec2(0, 0);
	std::vector<glm::vec3> oldPoints;
	oldPoints.push_back(glm::vec3(radius, 0, 0) + CoMShift);
	for (unsigned int div = 1; div < numsubdiv + 1; div++) {
		float ang = ((float)div) / ((float)numsubdiv) * 2 * 3.14159265358979323468;
		glm::vec3 this_point(0, 0, 0);
		this_point += CoMShift;
		this_point.x = radius * cosf(ang);
		this_point.z = radius * sinf(ang);
		auto& last_point = oldPoints.back();
		ConePart.pushTri(this_point, last_point, top_point, tc, tc, tc, color, color, color, false);
		if (div > 1) // There are enough points to make one triangle for the
					 // bottom plate.
		{
			Bottom_Plate_Part.pushTri(oldPoints[0], last_point, this_point, tc, tc, tc, color, color, color, true);
		}
		oldPoints.push_back(this_point);
	}
	ConePart.MinimizeSmoothing();
	Bottom_Plate_Part.MaximizeSmoothing();
	ConePart += Bottom_Plate_Part;
	ConePart.myFileName = "Auto Generated Cone with radius " + std::to_string(radius) + " and height " + std::to_string(h);
	return ConePart;
}
IndexedModel createPlane(float Xdim, float Zdim, uint Xsub, uint Zsub, bool smooth, glm::vec3 color, float texScale) {
	IndexedModel retval;
	retval.clear();
	retval.myFileName = "Auto generated Plane.";
	if (Xsub < 1)
		Xsub = 1;
	if (Zsub < 1)
		Zsub = 1;
	float xl_tc = 1.0f / (float)Xsub;
	float zl_tc = 1.0f / (float)Zsub;
	float xl = Xdim * xl_tc;
	float zl = Zdim * zl_tc;
	xl_tc *= texScale;
	zl_tc *= texScale;
	for (uint x = 0; x < Xsub; x++)
		for (uint z = 0; z < Zsub; z++) {

			// Base
			float xb = (float)x * xl;
			float xb_tc = (float)x * xl_tc;
			float zb = (float)z * zl;
			float zb_tc = (float)z * zl_tc;
			//~ std::cout << "\nX is " << x << " and xl_tc is " << xl_tc << std::endl;
			//~ std::cout << "\nZ is " << z << " and zl_tc is " << zl_tc << std::endl;
			glm::vec3 x1z1 = glm::vec3(xb + xl, 0, zb + zl);
			glm::vec3 x0z1 = glm::vec3(xb, 0, zb + zl);
			glm::vec3 x1z0 = glm::vec3(xb + xl, 0, zb);
			glm::vec3 x0z0 = glm::vec3(xb, 0, zb);

			glm::vec2 x1z1tc = glm::vec2(xb_tc + xl_tc, zb_tc + zl_tc);
			x1z1tc.x *= -1;
			glm::vec2 x0z1tc = glm::vec2(xb_tc, zb_tc + zl_tc);
			x0z1tc.x *= -1;
			glm::vec2 x1z0tc = glm::vec2(xb_tc + xl_tc, zb_tc);
			x1z0tc.x *= -1;
			glm::vec2 x0z0tc = glm::vec2(xb_tc, zb_tc);
			x0z0tc.x *= -1;
			//~ retval.pushTri(x1z1, x0z1, x0z0, x1z1tc, x0z1tc, x0z0tc, color, color, color, smooth);
			//~ retval.pushTri(x1z0, x1z1, x0z0, x1z0tc, x1z1tc, x0z0tc, color, color, color, smooth);
			retval.pushTri(x1z1, x0z0, x0z1, x1z1tc, x0z0tc, x0z1tc, color, color, color, smooth);
			retval.pushTri(x1z0, x0z0, x1z1, x1z0tc, x0z0tc, x1z1tc, color, color, color, smooth);
		}
	retval.validate();
	retval.calcNormals();
	return retval;
}

IndexedModel genHeightMap(unsigned char* data, uint w, uint h, int comp, float Xdim, float Zdim, float yScale,
						  bool smooth, // Just passed to plane generator.
						  float texScale) {
	glm::vec3 color(1, 0, 0);
	IndexedModel retval;
	retval.myFileName = "Auto generated Heightmap.";
	if (w < 1)
		return getErrorShape("Bad W in heightmap");
	if (h < 1)
		return getErrorShape("Bad H in heightmap");
	if (!data)
		return getErrorShape("Null Data in heightmap");
	float xl_tc = 1.0f / (float)w;
	float zl_tc = 1.0f / (float)h;
	float xl = Xdim * xl_tc;
	float zl = Zdim * zl_tc;
	xl_tc *= texScale;
	zl_tc *= texScale;
	for (uint x = 0; x < w - 1; x++)
		for (uint z = 0; z < h - 1; z++) {
			unsigned char hmapval0_0 = data[(x + z * w) * comp];
			unsigned char hmapval0_1 = data[(x + (z + 1) * w) * comp];
			unsigned char hmapval1_1 = data[((x + 1) + (z + 1) * w) * comp];
			unsigned char hmapval1_0 = data[((x + 1) + z * w) * comp];
			float Y0_0 = (float)hmapval0_0 * yScale / 255.0f;
			float Y0_1 = (float)hmapval0_1 * yScale / 255.0f;
			float Y1_1 = (float)hmapval1_1 * yScale / 255.0f;
			float Y1_0 = (float)hmapval1_0 * yScale / 255.0f;
			// Base
			float xb = (float)x * xl;
			float xb_tc = (float)x * xl_tc;
			float zb = (float)z * zl;
			float zb_tc = (float)z * zl_tc;
			//~ std::cout << "\nX is " << x << " and xl_tc is " << xl_tc << std::endl;
			//~ std::cout << "\nZ is " << z << " and zl_tc is " << zl_tc << std::endl;
			glm::vec3 x1z1 = glm::vec3(xb + xl, Y1_1, zb + zl);
			glm::vec3 x0z1 = glm::vec3(xb, Y0_1, zb + zl);
			glm::vec3 x1z0 = glm::vec3(xb + xl, Y1_0, zb);
			glm::vec3 x0z0 = glm::vec3(xb, Y0_0, zb);

			glm::vec2 x1z1tc = glm::vec2(xb_tc + xl_tc, zb_tc + zl_tc);
			x1z1tc.x *= -1;
			glm::vec2 x0z1tc = glm::vec2(xb_tc, zb_tc + zl_tc);
			x0z1tc.x *= -1;
			glm::vec2 x1z0tc = glm::vec2(xb_tc + xl_tc, zb_tc);
			x1z0tc.x *= -1;
			glm::vec2 x0z0tc = glm::vec2(xb_tc, zb_tc);
			x0z0tc.x *= -1;
			retval.pushTri(x1z1, x0z0, x0z1, x1z1tc, x0z0tc, x0z1tc, color, color, color, smooth);
			retval.pushTri(x1z0, x0z0, x1z1, x1z0tc, x0z0tc, x1z1tc, color, color, color, smooth);
		}
	retval.validate();
	retval.calcNormals();
	return retval;
}

IndexedModel Voxel2Model(
	unsigned char* blockData, uint xdim, uint ydim, uint zdim,
	std::vector<glm::vec3>* colors
){
	auto retval = IndexedModel();
	if(!blockData || xdim == 0 || ydim == 0 || zdim == 0)
	{
			std::cout << "\nBad voxel Creator Args!" << std::endl;
		return getErrorShape("Bad Voxel Creator Args");
	}
	for(size_t x = 0; x < xdim; x++)
	for(size_t y = 0; y < ydim; y++)
	for(size_t z = 0; z < zdim; z++)
		if(blockData[x + y * xdim + z * xdim * ydim] != 0) //test if it's not air.
		{
			float minX = x;
			float maxX = x + 1;
			float minY = y;
			float maxY = y+1;
			float minZ = z;
			float maxZ = z+1;
			uint atlasY = blockData[x + y * xdim + z * xdim * ydim] / 16;
			uint atlasX = blockData[x + y * xdim + z * xdim * ydim] % 16;
			glm::vec2 minTC;
			glm::vec2 maxTC;
			minTC.x = atlasX * 1.0f/16.0f;
			minTC.y = atlasY * 1.0f/16.0f;
			maxTC.x = minTC.x + 1.0f/16.0f;
			maxTC.y = minTC.y + 1.0f/16.0f;
			glm::vec3 color(0);
			if(colors && colors->size() > blockData[x + y * xdim + z * xdim * ydim])
				color = (*colors)[blockData[x + y * xdim + z * xdim * ydim]];
			if(	x == 0 || 
				(blockData[x-1 + y * xdim + z * xdim * ydim] == 0)
			){ //Create face pointing toward negative x.
				glm::vec3 p1, p2, p3, p4;
				glm::vec2 t1, t2, t3, t4;
				t1 = minTC;
				t2.x = minTC.x;
				t2.y = maxTC.y;
				t3 = maxTC;
				t4.x = maxTC.x;
				t4.y = minTC.y;
				p1.x = minX; p2.x = minX; p3.x = minX; p4.x = minX;
				p1.z = minZ; p2.z = minZ;
				p3.z = maxZ; p4.z = maxZ;
				p1.y = minY; p4.y = minY;
				p2.y = maxY; p3.y = maxY;
				//Push triangles.
				retval.pushTri(p3, p2, p1,
							   t3, t2, t1,
							   color, color, color);
				retval.pushTri(p1, p4, p3,
							   t1, t4, t3,
							   color, color, color);
			}
			
			if(	x == xdim - 1 || 
				(blockData[x+1 + y * xdim + z * xdim * ydim] == 0)
			){ //Create face pointing toward positive x.
				glm::vec3 p1, p2, p3, p4;
				glm::vec2 t1, t2, t3, t4;
				t1 = minTC;
				t2.x = minTC.x;
				t2.y = maxTC.y;
				t3 = maxTC;
				t4.x = maxTC.x;
				t4.y = minTC.y;
				p1.x = maxX; p2.x = maxX; p3.x = maxX; p4.x = maxX;
				p1.z = minZ; p2.z = minZ;
				p3.z = maxZ; p4.z = maxZ;
				p1.y = minY; p4.y = minY;
				p2.y = maxY; p3.y = maxY;
				//Push triangles.
				retval.pushTri(p3, p1, p2,
							   t3, t1, t2,
							   color, color, color);
				retval.pushTri(p1, p3, p4,
							   t1, t3, t4,
							   color, color, color);
			}
			
			if(	y == 0 || 
				(blockData[x + (y-1) * xdim + z * xdim * ydim] == 0)
			){ //Create face pointing toward negative y.
				glm::vec3 p1, p2, p3, p4;
				glm::vec2 t1, t2, t3, t4;
				t1 = minTC;
				t2.x = minTC.x;
				t2.y = maxTC.y;
				t3 = maxTC;
				t4.x = maxTC.x;
				t4.y = minTC.y;
				p1.y = minY; p2.y = minY; p3.y = minY; p4.y = minY;
				p1.x = minX; p2.x = minX;
				p3.x = maxX; p4.x = maxX;
				p1.z = minZ; p4.z = minZ;
				p2.z = maxZ; p3.z = maxZ;
				//Push triangles.
				retval.pushTri(p3, p2, p1,
							   t3, t2, t1,
							   color, color, color);
				retval.pushTri(p1, p4, p3,
							   t1, t4, t3,
							   color, color, color);
			}
			
			if(	y == ydim - 1 || 
				(blockData[x + (y+1) * xdim + z * xdim * ydim] == 0)
			){ //Create face pointing toward positive y.
				glm::vec3 p1, p2, p3, p4;
				glm::vec2 t1, t2, t3, t4;
				t1 = minTC;
				t2.x = minTC.x;
				t2.y = maxTC.y;
				t3 = maxTC;
				t4.x = maxTC.x;
				t4.y = minTC.y;
				p1.y = maxY; p2.y = maxY; p3.y = maxY; p4.y = maxY;
				p1.x = minX; p2.x = minX;
				p3.x = maxX; p4.x = maxX;
				p1.z = minZ; p4.z = minZ;
				p2.z = maxZ; p3.z = maxZ;
				//Push triangles.
				retval.pushTri(p3, p1, p2,
							   t3, t1, t2,
							   color, color, color);
				retval.pushTri(p1, p3, p4,
							   t1, t3, t4,
							   color, color, color);
			}
			
			if(	z == 0 || 
				(blockData[x + y * xdim + (z-1) * xdim * ydim] == 0)
			){ //Create face pointing toward negative Z.
				glm::vec3 p1, p2, p3, p4;
				glm::vec2 t1, t2, t3, t4;
				t1 = minTC;
				t2.x = minTC.x;
				t2.y = maxTC.y;
				t3 = maxTC;
				t4.x = maxTC.x;
				t4.y = minTC.y;
				p1.z = minZ; p2.z = minZ; p3.z = minZ; p4.z = minZ;
				p1.x = minX; p2.x = minX;
				p3.x = maxX; p4.x = maxX;
				p1.y = minY; p4.y = minY;
				p2.y = maxY; p3.y = maxY;
				//Push triangles.
				retval.pushTri(p3, p1, p2,
							   t3, t1, t2,
							   color, color, color);
				retval.pushTri(p1, p3, p4,
							   t1, t3, t4,
							   color, color, color);
				
			}
			
			if(	z == zdim - 1 || 
				(blockData[x + y * xdim + (z+1) * xdim * ydim] == 0)
			){ //Create face pointing toward positive Z.
				glm::vec3 p1, p2, p3, p4;
				glm::vec2 t1, t2, t3, t4;
				t1 = minTC;
				t2.x = minTC.x;
				t2.y = maxTC.y;
				t3 = maxTC;
				t4.x = maxTC.x;
				t4.y = minTC.y;
				p1.z = maxZ; p2.z = maxZ; p3.z = maxZ; p4.z = maxZ;
				p1.x = minX; p2.x = minX;
				p3.x = maxX; p4.x = maxX;
				p1.y = minY; p4.y = minY;
				p2.y = maxY; p3.y = maxY;
				//Push triangles.
				retval.pushTri(p3, p2, p1,
							   t3, t2, t1,
							   color, color, color);
				retval.pushTri(p1, p4, p3,
							   t1, t4, t3,
							   color, color, color);
			}
			
		}
	retval.calcNormals();
	//~ std::cout << "\nFinishing voxel!" << std::endl;
	return retval;
}

glm::quat faceTowardPoint(glm::vec3 pos, glm::vec3 target, glm::vec3 up) {
	//~ glm::vec3 defaultfacingvector = glm::vec3(0,0,-1);
	// glm::vec3 facingvector = target-ourpoint;
	return glm::quat(glm::inverse(glm::lookAt(pos, target, up)));
} // Autoface for Sprites


glm::vec3 planeIntersect(const glm::vec4& p1, const glm::vec4& p2, const glm::vec4& p3) {
    // determinant
    float det = p1.x * (p2.y * p3.z -
                p2.z * p3.y) - p2.x *(p1.y * p3.z -
                p1.z * p3.y) + p3.x * (p1.y * p2.z - p1.z * p2.y);
    if (fabs(det) <= 0.0001f)
        return glm::vec3();
    // Create 3 points, one on each plane.
    float p1x = -p1.x * p1.w;
    float p1y = -p1.y * p1.w;
    float p1z = -p1.z * p1.w;
    float p2x = -p2.x * p2.w;
    float p2y = -p2.y * p2.w;
    float p2z = -p2.z * p2.w;
    float p3x = -p3.x * p3.w;
    float p3y = -p3.y * p3.w;
    float p3z = -p3.z * p3.w;
    // Calculate the cross products of the normals.
    float c1x = (p2.y * p3.z) - (p2.z * p3.y);
    float c1y = (p2.z * p3.x) - (p2.x * p3.z);
    float c1z = (p2.x * p3.y) - (p2.y * p3.x);
    float c2x = (p3.y * p1.z) - (p3.z * p1.y);
    float c2y = (p3.z * p1.x) - (p3.x * p1.z);
    float c2z = (p3.x * p1.y) - (p3.y * p1.x);
    float c3x = (p1.y * p2.z) - (p1.z * p2.y);
    float c3y = (p1.z * p2.x) - (p1.x * p2.z);
    float c3z = (p1.x * p2.y) - (p1.y * p2.x);
    // Calculate the point of intersection using the formula:
    // x = (| n1 n2 n3 |)^-1 * [(x1 * n1)(n2 x n3) + (x2 * n2)(n3 x n1) + (x3 * n3)(n1 x n2)]
    float s1 = p1x * p1.x + p1y * p1.y + p1z * p1.z;
    float s2 = p2x * p2.x + p2y * p2.y + p2z * p2.z;
    float s3 = p3x * p3.x + p3y * p3.y + p3z * p3.z;
    float detI = 1.0f / det;
    
    glm::vec3 point;
    point.x = (s1 * c1x + s2 * c2x + s3 * c3x) * detI;
    point.y = (s1 * c1y + s2 * c2y + s3 * c3y) * detI;
    point.z = (s1 * c1z + s2 * c2z + s3 * c3z) * detI;
    return point;
}

std::vector<glm::vec3> getFrustCorners(const std::vector<glm::vec4>& frustum) {
    std::vector<glm::vec3> corners; 
    corners.reserve(8);
		corners.push_back(planeIntersect(frustum[4], frustum[0], frustum[2]));
		corners.push_back(planeIntersect(frustum[4], frustum[0], frustum[3]));
		corners.push_back(planeIntersect(frustum[4], frustum[1], frustum[3]));
		corners.push_back(planeIntersect(frustum[4], frustum[1], frustum[2]));
		corners.push_back(planeIntersect(frustum[5], frustum[1], frustum[2]));
		corners.push_back(planeIntersect(frustum[5], frustum[1], frustum[3]));
		corners.push_back(planeIntersect(frustum[5], frustum[0], frustum[3]));
		corners.push_back(planeIntersect(frustum[5], frustum[0], frustum[2]));
    return corners;
}

std::vector<glm::vec4> extractFrustum(const glm::mat4& source) {
	const float* matrix = &(source[0][0]);
	std::vector<glm::vec4> result;
	result.resize(6);

    // Left
    result[0].x = matrix[3] + matrix[0];
    result[0].y = matrix[7] + matrix[4];
    result[0].z = matrix[11] + matrix[8];
    result[0].w = matrix[15] + matrix[12];
    
    // Right
    result[1].x = matrix[3] - matrix[0];
    result[1].y = matrix[7] - matrix[4];
    result[1].z = matrix[11] - matrix[8];
    result[1].w = matrix[15] - matrix[12];
    
    // Top
    result[2].x = matrix[3] - matrix[1];
    result[2].y = matrix[7] - matrix[5];
    result[2].z = matrix[11] - matrix[9];
    result[2].w = matrix[15] - matrix[13];
    
    // Bottom
    result[3].x = matrix[3] + matrix[1];
    result[3].y = matrix[7] + matrix[5];
    result[3].z = matrix[11] + matrix[9];
    result[3].w = matrix[15] + matrix[13];
    
    // Near
    result[4].x = matrix[3] + matrix[2];
    result[4].y = matrix[7] + matrix[6];
    result[4].z = matrix[11] + matrix[10];
    result[4].w = matrix[15] + matrix[14];
    
    // Far
    result[5].x = matrix[3] - matrix[2];
    result[5].y = matrix[7] - matrix[6];
    result[5].z = matrix[11] - matrix[10];
    result[5].w = matrix[15] - matrix[14];
    
    for (size_t i = 0; i < 6; i++) {
        float mag = sqrtf(
			result[i].x * result[i].x + 
			result[i].y * result[i].y + 
			result[i].z * result[i].z
		);
        result[i].x /= mag;
        result[i].y /= mag;
        result[i].z /= mag;
        result[i].w /= mag;
    }

    return result;
}

}; // namespace gekRender
