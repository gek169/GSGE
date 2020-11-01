#ifndef CAMERA_INCLUDED_H
#define CAMERA_INCLUDED_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// USAGE:
/*
Create a camera instance in your main.cpp or wherever your game code is and
register it as the finalpass camera in the scene class (pass a pointer)
*/
namespace gekRender {

struct Camera {
  public:
	// Constructor
	Camera(glm::vec3 _pos, float _fov, float _aspect, float _zNear, float _zFar, glm::vec3 _forward, glm::vec3 _up) {
		pos = _pos;
		// this->forward = glm::vec3(0.0f, 0.0f, 1.0f);
		// this->up = glm::vec3(0.0f, 1.0f, 0.0f);
		forward = _forward;
		up = _up;
		// Creates the mat4 for the projection matrix (instead of the default
		// orthogonal) NOTE That this is only applied to the VERTS and thus 3d
		// rendering is ever-so-slightly incorrect because lines are straight
		// where they should be curved
		buildPerspective(_fov, _aspect, _zNear, _zFar);
		//~ jafar = _zFar;
		//~ janear = _zNear;
		// Desperate attempt to fix bug with cameralights

		glm::vec3 right = glm::normalize(glm::cross(up, forward));

		forward = glm::vec3(glm::normalize(glm::rotate(0.0f, right) * glm::vec4(forward, 0.0)));
		up = glm::normalize(glm::cross(forward, right));
	}
	Camera() { // Who needs constructors?
	}
	Camera(const Camera& other) {
		projection = other.projection;
		pos = other.pos;
		forward = other.forward;
		up = other.up;
		jafar = other.jafar;
		janear = other.janear;
	}

	inline void buildPerspective(float fov, float aspect, float zNear, float zFar) {
		projection = glm::perspective(fov, aspect, zNear, zFar);
		jafar = zFar;
		janear = zNear;
	}

	inline void buildOrthogonal(float left, float right, float bottom, float top, float znear, float zfar) {
		projection = glm::ortho(left, right, bottom, top, znear, zfar);
		jafar = zfar; // jafar from afar ate z'far from timbuk-tartar
		janear = znear;
	}
	inline glm::vec3 getClickRay(glm::vec2 ncursorPos){
		glm::vec3 retval;
		ncursorPos *= 2;
		ncursorPos += glm::vec2(-1,-1);
		ncursorPos.y = -ncursorPos.y; //TM says to do this.
		//ncursorpos is now in OpenGL's normalized device coordinates
		glm::vec4 clipCoords = glm::vec4(ncursorPos.x, ncursorPos.y, -1, 1);
		glm::vec4 eyeCoords = glm::inverse(projection) * clipCoords;
		eyeCoords.z = -1; eyeCoords.w = 0;
		glm::vec4 worldCoords = glm::inverse(getViewMatrix()) * eyeCoords;
		retval.x = worldCoords.x;
		retval.y = worldCoords.y;
		retval.z = worldCoords.z;
		glm::normalize(retval);
		return retval;
	}
	inline glm::mat4 getViewProjection() const { return projection * glm::lookAt(pos, pos + forward, up); }
	inline glm::mat4 getProjection() { return projection; }
	inline glm::mat4 getViewMatrix() const { return glm::lookAt(pos, pos + forward, up); }
	inline void focusView(glm::vec3 focalpoint, float dist){
		pos = focalpoint - (dist * forward);
	}
	void moveUp(float amt) { pos += up * amt; }
	void moveForward(float amt) { pos += forward * amt; }
	//Quite confusingly, moveRight makes the camera move left.
	void moveRight(float amt) { pos += glm::cross(up, forward) * amt; }

	void pitch(float angle) {
		glm::vec3 right = getRight();
		forward = glm::vec3(glm::normalize(glm::rotate(angle, right) * glm::vec4(forward, 0.0)));
		up = glm::normalize(glm::cross(forward, right));
	}
	//Quite confusingly, this actually gets you a left-facing vector.
	inline glm::vec3 getRight() { return glm::normalize(glm::cross(up, forward)); }

	void rotateY(float angle) {
		static const glm::vec3 UP(0.0f, 1.0f, 0.0f);

		glm::mat4 rotation = glm::rotate(angle, UP);
		forward = glm::vec3(glm::normalize(rotation * glm::vec4(forward, 0.0)));
		up = glm::vec3(glm::normalize(rotation * glm::vec4(up, 0.0)));
	}
	glm::mat4 projection = glm::mat4();
	glm::vec3 pos = glm::vec3();
	glm::vec3 forward = glm::vec3();
	glm::vec3 up = glm::vec3();
	float jafar = 1;  // used for saving float zFar
	float janear = 0; // used for saving float zNear

	inline bool operator==(const Camera& lhs) {
		return lhs.projection == projection && lhs.pos == pos && lhs.forward == forward && lhs.up == up && lhs.jafar == jafar && lhs.janear == janear;
	}

  protected:
	// We don't use any...
};

}; // namespace gekRender
#endif
