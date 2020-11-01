#ifndef TRANSFORM_INCLUDED_H
#define TRANSFORM_INCLUDED_H

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>

// Note: Const reference means we don't make copies for the functions.



namespace gekRender {
struct Transform {
  public:
	Transform() { // Empty constructor.
	}

	Transform(const Transform& other) { setModel(other.getModel()); }
	Transform(const glm::mat4& _model) { setModel(_model); }
	void operator=(const glm::mat4& _model) { setModel(_model); }
	Transform(const glm::vec3 pos, const glm::vec3 rot, const glm::vec3 scale) {
		glm::mat4 posMat = glm::translate(pos);
		glm::mat4 scaleMat = glm::scale(scale);
		glm::mat4 rotMat = glm::toMat4(glm::quat(rot));
		model = posMat * rotMat * scaleMat;
	}
	operator glm::mat4() const {
		return getModel();
	}
	inline void reTransform(const glm::vec3 pos, const glm::vec3 rot, const glm::vec3 scale) {
		glm::mat4 posMat = glm::translate(pos);
		glm::mat4 scaleMat = glm::scale(scale);
		glm::mat4 rotMat = glm::toMat4(glm::quat(rot));
		model = posMat * rotMat * scaleMat;
		return;
	}
	// The Quaternion Alternative
	Transform(const glm::vec3 pos, const glm::quat quatrot, const glm::vec3 scale) {
		glm::mat4 posMat = glm::translate(pos);
		glm::mat4 scaleMat = glm::scale(scale);
		glm::mat4 rotMat = glm::toMat4(quatrot);
		model = posMat * rotMat * scaleMat;
	}

	inline glm::mat4 getModel() const // Returns a constant. TODO: fix camelCase
	{
		return model;
	}
	inline void setModel(glm::mat4 newmodel) { // Setting the model straight,
											   // e.g. from bullet physics
		model = newmodel;
	}

	inline glm::vec3 getPos() {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		return temppos;
	}
	inline glm::vec3 getRot() {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		temp3rot = glm::eulerAngles(tempquatrot);
		return temp3rot;
	}
	inline glm::quat getRotQUAT() {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		return tempquatrot;
	}
	inline glm::vec3 getScale() {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		return tempscale;
	}

	inline void setPos(glm::vec3 pos) {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		temppos = pos;
		// Note we are getting rid of skew and perspective transformations...
		glm::mat4 posMat = glm::translate(temppos);
		glm::mat4 scaleMat = glm::scale(tempscale);
		glm::mat4 rotMat = glm::toMat4(tempquatrot);
		model = posMat * rotMat * scaleMat;
	}
	inline void setRot(glm::vec3 rot) {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		tempquatrot = glm::quat(rot);
		// Note we are getting rid of skew and perspective transformations...
		glm::mat4 posMat = glm::translate(temppos);
		glm::mat4 scaleMat = glm::scale(tempscale);
		glm::mat4 rotMat = glm::toMat4(tempquatrot);
		model = posMat * rotMat * scaleMat;
	}
	inline void setRotMat4(glm::mat4 rot) {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		tempquatrot = glm::quat(rot);
		// Note we are getting rid of skew and perspective transformations...
		glm::mat4 posMat = glm::translate(temppos);
		glm::mat4 scaleMat = glm::scale(tempscale);
		// glm::mat4 rotMat = rot;
		model = posMat * rot * scaleMat;
	}
	inline void setRotQuat(glm::quat rot) {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		tempquatrot = rot;
		// Note we are getting rid of skew and perspective transformations...
		glm::mat4 posMat = glm::translate(temppos);
		glm::mat4 scaleMat = glm::scale(tempscale);
		glm::mat4 rotMat = glm::toMat4(tempquatrot);
		model = posMat * rotMat * scaleMat;
	}
	inline void setScale(glm::vec3 scale) {
		glm::decompose(model, tempscale, tempquatrot, temppos, tempskew, tempperspective);
		tempscale = scale;
		// Note we are getting rid of skew and perspective transformations...
		glm::mat4 posMat = glm::translate(temppos);
		glm::mat4 scaleMat = glm::scale(tempscale);
		glm::mat4 rotMat = glm::toMat4(tempquatrot);
		model = posMat * rotMat * scaleMat;
	}

  protected:
  private:
	// These are used merely to temporarily store data
	glm::vec3 temppos = glm::vec3();
	glm::vec3 temp3rot = glm::vec3();
	glm::quat tempquatrot = glm::quat();
	glm::vec3 tempscale = glm::vec3(1, 1, 1);
	glm::vec3 tempskew = glm::vec3();
	glm::vec4 tempperspective = glm::vec4();
	// This is the actual information
	glm::mat4 model = glm::mat4(1); // The model we want to send.
};
}; // namespace gekRender


#endif
