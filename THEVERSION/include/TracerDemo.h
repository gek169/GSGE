#include "gekrender.h"
#include <glm/gtx/intersect.hpp>

using namespace gekRender; // For convenience... don't do this in your
							   // programs normally

struct TraceablePointLight {
	glm::vec3 position = glm::vec3(0, 0, 0);
	TraceablePointLight() {}
	glm::vec3 color = glm::vec3(1, 1, 1);
	float range = -1; // Infinite
	// Linear Dropoff...
	float getMultiplier(float distance) {
		if (distance == 0 || range < 1)
			return 1;
		if (distance > range)
			return 0;
		return distance / (range * range);
	}
};

struct TraceableShape {
	glm::vec3 color = glm::vec3(1, 0, 0); // RED
	float reflectivity = 0.8;
	float specularDampening = 20; // PHONG
	virtual glm::vec3 FindClosestCollisionPoint(glm::vec3 RayDir, glm::vec3 EyePos, float drawdistance) {
		std::cout << "\nWRONG FUNCTION!!!";
		return glm::vec3();
	}
	virtual glm::vec3 BounceRay(glm::vec3 RayDir, glm::vec3 BouncingPosition) {
		std::cout << "\nWRONG FUNCTION!!!- Bounce";
		return glm::vec3();
	}
	virtual glm::vec3 GetColorAtPoint(glm::vec3 PointOnSurface) {
		std::cout << "\nWRONG FUNCTION!!!- Color";
		return color;
	}
	virtual glm::vec3 GetNormalAtPoint(glm::vec3 PointOnSurface) {
		std::cout << "\nWRONG FUNCTION!!!!- Normal";
		return glm::vec3();
	}
	virtual ~TraceableShape() {}
};

struct Sphere : public TraceableShape {

	Sphere(glm::vec3 _position, glm::vec3 _color, float _radius) {
		radius = _radius;
		position = _position;
		color = _color;
	}

	glm::vec3 position = glm::vec3(0, 0.2, 10);
	float radius = 1;
	glm::vec3 FindClosestCollisionPoint(glm::vec3 RayDir, glm::vec3 EyePos,
										float drawdistance) { // RAYDIR MUST BE NORMALIZED!!!
		float distance = 0;
		bool doesIntersect = glm::intersectRaySphere(EyePos, RayDir, position, radius * radius, distance);

		if (!doesIntersect)
			distance = 0;

		glm::vec3 CollisionPoint = RayDir * distance + EyePos;
		return CollisionPoint;

	} // if no collision point, returns EyePos
	glm::vec3 GetNormalAtPoint(glm::vec3 PointOnSurface) { return glm::normalize(PointOnSurface - position); }

	glm::vec3 BounceRay(glm::vec3 RayDir, glm::vec3 BouncingPosition) {
		glm::vec3 Normal = glm::normalize(BouncingPosition - position);
		return glm::normalize(glm::reflect(RayDir, Normal));
	}

	glm::vec3 GetColorAtPoint(glm::vec3 PointOnSurface) { return color; }
	// float raySphereIntersect(glm::vec3 r0, glm::vec3 rd, glm::vec3 s0, float
	// sr) { //found on internet
	// - r0: ray origin
	// - rd: normalized ray direction
	// - s0: sphere center
	// - sr: sphere radius
	// - Returns distance from r0 to first intersecion with sphere,
	// or -1.0 if no intersection.
	// float a = glm::dot(rd, rd);
	// glm::vec3 s0_r0 = r0 - s0;
	// float b = 2.0 * glm::dot(rd, s0_r0);
	// float c = glm::dot(s0_r0, s0_r0) - (sr * sr);
	// if (b*b - 4.0*a*c < 0.0) {
	// return -1.0;
	// }
	// return (-b - sqrt((b*b) - 4.0*a*c))/(2.0*a);
	// }
	~Sphere() {
		// None?
	}
};

struct TracerWorld {
	// background color
	glm::vec3 BackgroundColor = glm::vec3(0, 0, 0.3);	 // A nice blue
	glm::vec3 theAmbientLight = glm::vec3(0.1, 0.1, 0.1); // Dark gray

	float TraceRayDistance2(glm::vec3 RayDir, glm::vec3 EyePos,
							float renderdistance) // Finds the closest object and gets the
												  // distance... used for shadows only
	{
		// bool didFind = false;
		glm::vec3 closestCollisionPoint = EyePos;
		// size_t indexofclosestobject = TracerObjects.size(); //This value will
		// fail the test
		float closestDistance = -1;
		// Test against all the TracerObjects to find the closest collision
		// point to the EyePos
		for (size_t i = 0; i < TracerObjects.size(); i++) {
			glm::vec3 CollisionPoint = TracerObjects[i]->FindClosestCollisionPoint(glm::normalize(RayDir), EyePos, renderdistance);
			float distance = glm::length2(EyePos - CollisionPoint);
			// std::cout << "\nDistance:" << distance;
			if ((distance < closestDistance || closestDistance == -1) && distance > 0.0001 && CollisionPoint != EyePos) {
				// std::cout << "\nFOUND SOMETHING AT DISTANCE: " << distance;
				// didFind = true;
				// indexofclosestobject = i;
				closestCollisionPoint = CollisionPoint;
				closestDistance = distance;
			}
		}
		// if (!didFind)
		// std::cout << "\n DIDNT FIND ANYTHING";
		// Now that we have the closest collision point in the scene, return the
		// distance
		return closestDistance;
	}

	glm::vec3 TraceRay(glm::vec3 RayDir, glm::vec3 EyePos, int RayCountDown = 5,
					   float renderdistance = 300) // Bounces 5 times, returns a color.
												   // Finds the closest collision point
	{
		// if(RayCountDown < 3)
		// std::cout << "\nRAYDIR:" <<
		// "\nX: " << RayDir.x <<
		// "\nY: " << RayDir.y <<
		// "\nZ: " << RayDir.z <<
		// "\nEYEPOS:" <<
		// "\nX: " << EyePos.x <<
		// "\nY: " << EyePos.y <<
		// "\nZ: " << EyePos.z;
		bool didFind = false;
		glm::vec3 closestCollisionPoint = glm::vec3(0, renderdistance * renderdistance, 0);
		glm::vec3 BouncedRay = glm::vec3(1, 0, 0);
		size_t indexofclosestobject = TracerObjects.size(); // This value will fail the test
		float closestDistance = renderdistance * renderdistance;
		// Test against all the TracerObjects to find the closest collision
		// point to the EyePos
		for (size_t i = 0; i < TracerObjects.size(); i++) {
			glm::vec3 CollisionPoint = TracerObjects[i]->FindClosestCollisionPoint(glm::normalize(RayDir), EyePos, renderdistance);
			float distance = glm::length2(EyePos - CollisionPoint);
			// std::cout << "\nDistance:" << distance;
			if (distance < closestDistance && distance > 0.0001 && CollisionPoint != EyePos) {
				// if (RayCountDown < 3)
				// std::cout << "\nRay Strike!";
				didFind = true;
				indexofclosestobject = i;
				closestCollisionPoint = CollisionPoint;
				closestDistance = distance;
				BouncedRay = TracerObjects[i]->BounceRay(glm::normalize(RayDir), CollisionPoint);
			}
		}

		// Now that we have the closest collision point in the scene, use it to
		// compute the ray color. Bounce, of course...
		if (RayCountDown > 0 && didFind && indexofclosestobject < TracerObjects.size()) {
			glm::vec3 LightingAdder = glm::vec3(0, 0, 0);
			glm::vec3 LightingMultiplier = theAmbientLight;
			for (size_t i = 0; i < TracerLights.size(); i++) {
				float disttolight = glm::length2(TracerLights[i].position - closestCollisionPoint);
				glm::vec3 RayDirForTest = glm::normalize(TracerLights[i].position - closestCollisionPoint);
				float disttoclosestobject = TraceRayDistance2(RayDirForTest, closestCollisionPoint, renderdistance);
				float dotproduct = glm::dot(RayDirForTest, TracerObjects[indexofclosestobject]->GetNormalAtPoint(closestCollisionPoint));
				dotproduct = (dotproduct < 0) ? 0 : dotproduct;
				if ((disttoclosestobject > disttolight || disttoclosestobject < 0) && dotproduct > 0) {
					LightingMultiplier += dotproduct * TracerLights[i].color * TracerLights[i].getMultiplier(disttoclosestobject);
					/*Reference GLSL code for Specular Lighting
							vec3 reflectedLightDir =
		  reflect(dir_lightArray[i].direction, UnitNormal);

							float specFactor = max(
									dot(reflectedLightDir, unit_frag_to_camera),
									0.0
							);
							float specDampFactor = pow(specFactor,specdamp);
							speceffect += shouldRenderSpecEffect *
		  renderthislight * specDampFactor * specreflectivity *
		  dir_lightArray[i].color;
					*/
					glm::vec3 reflectedLightDir = TracerObjects[indexofclosestobject]->BounceRay(-RayDirForTest, closestCollisionPoint);
					// std::cout << "\ndotproduct:" <<
					// glm::dot(reflectedLightDir, -RayDir);
					float specFactor = glm::max(glm::dot(reflectedLightDir, -RayDir), 0.0f);
					// std::cout << "\nspecFactor: " << specFactor;
					float specDampFactor = glm::pow(specFactor, TracerObjects[indexofclosestobject]->specularDampening);
					LightingAdder += specDampFactor * TracerLights[i].color * TracerLights[i].getMultiplier(disttoclosestobject);
				}
			}
			LightingMultiplier = glm::clamp(LightingMultiplier, glm::vec3(0), glm::vec3(1));

			glm::vec3 returnval = (LightingMultiplier * TracerObjects[indexofclosestobject]->GetColorAtPoint(closestCollisionPoint)) *
									  (1 - TracerObjects[indexofclosestobject]->reflectivity) +
								  TracerObjects[indexofclosestobject]->reflectivity *
									  (LightingAdder + TraceRay(BouncedRay, closestCollisionPoint, RayCountDown - 1, renderdistance));

			returnval = glm::clamp(returnval, glm::vec3(0), glm::vec3(1));
			return returnval;
		} else if (didFind && indexofclosestobject < TracerObjects.size()) {
			glm::vec3 LightingAdder = glm::vec3(0, 0, 0);
			glm::vec3 LightingMultiplier = theAmbientLight;
			for (size_t i = 0; i < TracerLights.size(); i++) {
				float disttolight = glm::length2(TracerLights[i].position - closestCollisionPoint);
				glm::vec3 RayDirForTest = glm::normalize(TracerLights[i].position - closestCollisionPoint);
				float disttoclosestobject = TraceRayDistance2(RayDirForTest, closestCollisionPoint, renderdistance);
				float dotproduct = glm::dot(RayDirForTest, TracerObjects[indexofclosestobject]->GetNormalAtPoint(closestCollisionPoint));
				dotproduct = (dotproduct < 0) ? 0 : dotproduct;
				if ((disttoclosestobject > disttolight || disttoclosestobject < 0) && dotproduct > 0) {
					LightingMultiplier += dotproduct * TracerLights[i].color * TracerLights[i].getMultiplier(disttoclosestobject);
					/*Reference GLSL code for Specular Lighting
							vec3 reflectedLightDir =
		  reflect(dir_lightArray[i].direction, UnitNormal);

							float specFactor = max(
									dot(reflectedLightDir, unit_frag_to_camera),
									0.0
							);
							float specDampFactor = pow(specFactor,specdamp);
							speceffect += shouldRenderSpecEffect *
		  renderthislight * specDampFactor * specreflectivity *
		  dir_lightArray[i].color;
					*/
					glm::vec3 reflectedLightDir = TracerObjects[indexofclosestobject]->BounceRay(-RayDirForTest, closestCollisionPoint);
					float specFactor = glm::max(glm::dot(reflectedLightDir, -RayDir), 0.0f);
					float specDampFactor = glm::pow(specFactor, TracerObjects[indexofclosestobject]->specularDampening);
					LightingAdder += specDampFactor * TracerLights[i].color * TracerLights[i].getMultiplier(disttoclosestobject);
				}
			}
			LightingMultiplier = glm::clamp(LightingMultiplier, glm::vec3(0), glm::vec3(1));

			return glm::clamp(TracerObjects[indexofclosestobject]->GetColorAtPoint(closestCollisionPoint) * LightingMultiplier +
								  TracerObjects[indexofclosestobject]->reflectivity * LightingAdder,
							  glm::vec3(0), glm::vec3(1));
		}

		return BackgroundColor;
	}

	std::vector<TraceableShape*> TracerObjects;
	std::vector<TraceablePointLight> TracerLights;
};
