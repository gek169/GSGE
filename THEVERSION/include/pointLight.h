#ifndef PointLight_H
#define PointLight_H

#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#define GLM_FORCE_RADIANS
#include "SafeTexture.h"
#include "camera.h"
#include <glm/glm.hpp>

namespace gekRender {
// Cpu Representation of point light
struct PointLight {
  public:
	PointLight() {
		myPos = glm::vec3(0, 0, 0);
		myColor = glm::vec3(1, 1, 1);
	}
	PointLight(glm::vec3 newpos, glm::vec3 newcolor) {
		myPos = newpos;
		myColor = newcolor;
	}
	~PointLight() {
		// delet this
	}

	void setPos(glm::vec3 pos) { myPos = pos; }
	glm::vec3 getPos() { return myPos; }

	void bindToUniformLight(GLuint position, GLuint color, GLuint m_range, GLuint m_dropoff) const {
		glUniform3f(position, myPos.x, myPos.y, myPos.z);
		glUniform3f(color, myColor.x, myColor.y, myColor.z);
		glUniform1f(m_range, this->range);
		glUniform1f(m_dropoff, this->dropoff);
	}

	void setRangeProperties(float newrange, float newdropoff) {
		range = newrange * newrange;
		dropoff = newdropoff;
	}
	glm::vec3 myPos;
	glm::vec3 myColor;
	float dist2 = 0; // used for sorting.

	//~ glm::vec4 sphere1 = glm::vec4(0);
	//~ glm::vec4 sphere2 = glm::vec4(0);
	//~ glm::vec4 sphere3 = glm::vec4(0);
	//~ glm::vec4 sphere4 = glm::vec4(0);

	//~ glm::vec3 AABBp1 = glm::vec3(0);
	//~ glm::vec3 AABBp2 = glm::vec3(0);
	//~ glm::vec3 AABBp3 = glm::vec3(0);
	//~ glm::vec3 AABBp4 = glm::vec3(0);

	bool whitelist = false;

	float range = 1000;		  // range
	float dropoff = 1;		  // no dropoff
	bool shouldRender = true; // Render this point light.
};
// Cpu representation of DirectionalLight
struct DirectionalLight {
  public:
	DirectionalLight() {
		myColor = glm::vec3(0.2, 0.2, 0.2);
		myDirection = glm::vec3(0, 0, 1);
	}
	void bindToUniformDirectionalLight(GLuint col, GLuint dir) const {
		glUniform3f(col, myColor.x, myColor.y, myColor.z);

		glUniform3f(dir, myDirection.x, myDirection.y, myDirection.z);
	}
	glm::vec3 myColor;
	glm::vec3 myDirection;

	//~ glm::vec4 sphere1 = glm::vec4(0);
	//~ glm::vec4 sphere2 = glm::vec4(0);
	//~ glm::vec4 sphere3 = glm::vec4(0);
	//~ glm::vec4 sphere4 = glm::vec4(0);

	//~ glm::vec3 AABBp1 = glm::vec3(0);
	//~ glm::vec3 AABBp2 = glm::vec3(0);
	//~ glm::vec3 AABBp3 = glm::vec3(0);
	//~ glm::vec3 AABBp4 = glm::vec3(0);

	bool shouldRender = true;
	bool whitelist = false;
};
// Spotlight with shadows, spotlight that projects a texture, orthoganal
// repeating projected texture (e.g. for caustics) etc.
struct CameraLight {
	CameraLight() {
		myColor = glm::vec3(0.2, 0.2, 0.2);
		myCamera = Camera(glm::vec3(0, 0, 0), // Pos
						  70.0,				  // FOV
						  1,				  // Aspect
						  0.1,				  // zNear
						  1000,				  // zFar
						  glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));
		Tex2Project = SafeTexture();
	}
	CameraLight(glm::vec3 pos, float fov, float aspect, float zNear, float zFar, glm::vec3 forward, glm::vec3 up, glm::vec3 _color, bool _Shadowtoggle,
				SafeTexture _tex) {
		myCamera = Camera(pos, fov, aspect, zNear, zFar, forward, up);
		myColor = _color;
		Tex2Project = _tex;
	}
	void bindToUniformCameraLight(GLuint m_matrix, GLuint m_camerapos, GLuint m_color, GLuint m_SolidColorToggle, GLuint m_range, GLuint m_shadows,
								  GLuint m_radii, unsigned int textureunit) {
		MatrixCache = myCamera.getViewProjection();
		glUniformMatrix4fv(m_matrix, 1, GL_FALSE, &MatrixCache[0][0]);
		// Setup the far and near plane variables

		// Setup the position and color
		if (range >= 0) {
			glUniform3f(m_camerapos, myCamera.pos.x, myCamera.pos.y, myCamera.pos.z);
		} else {
			glUniform3f(m_camerapos, myCamera.forward.x, myCamera.forward.y, myCamera.forward.z);
		}
		glUniform3f(m_color, myColor.x, myColor.y, myColor.z);
		glUniform2f(m_radii, radii.x, radii.y);
		glUniform1f(m_SolidColorToggle, solidColorToggle);
		glUniform1f(m_range, range);
		glUniform1f(m_shadows, isShadowed ? 1.0f : 0.0f);

		// Bind texture
		Tex2Project.bind(textureunit);
	}

	void bindToUniformBufferCameraLight(glm::mat4* m_matrix, glm::vec3* m_camerapos, glm::vec3* m_color, GLfloat* m_SolidColorToggle, GLfloat* m_range,
										GLfloat* m_shadows, glm::vec2* m_radii, glm::vec2* m_shadowmapdimensions, unsigned int textureunit) {
		MatrixCache = myCamera.getViewProjection();
		*m_matrix = MatrixCache;
		// Setup the far and near plane variables

		// Setup the position and color
		if (range >= 0) {
			*m_camerapos = glm::vec3(myCamera.pos.x, myCamera.pos.y, myCamera.pos.z);
		} else {
			*m_camerapos = glm::vec3(myCamera.forward.x, myCamera.forward.y, myCamera.forward.z);
		}
		*m_color = myColor;
		*m_radii = radii;
		*m_SolidColorToggle = solidColorToggle;
		*m_range = range;
		*m_shadows = isShadowed ? 1.0f : 0.0f;
		*m_shadowmapdimensions = Dimensions;
		// Bind texture
		Tex2Project.bind(textureunit);
	}
	void setRange(float _range) { range = _range; } // square range
	// TODO rename and retype solidColorToggle to a bool "useSolidColor"
	float solidColorToggle = 0.0f;			// By default, use texture
	glm::vec3 myColor;						// the color to use in solid color mode
	glm::vec2 radii = glm::vec2(0.5, 0.5);  // Inner and outer radii used
	glm::vec2 Dimensions = glm::vec2(1, 1); // Default value, don't actually let it be this lol
	float range = 200.0f;
	Camera myCamera;	   // Contains position and etc.
	glm::mat4 MatrixCache; // Needed to call glUniformMatrix4fv
	bool shouldRender = true;
	bool isShadowed = false;
	SafeTexture Tex2Project;			///< Overrides myColor in projection mode OR serves
										///< for depth in shadow mode
	void* myShadowMappingFBO = nullptr; ///< Pointer to Depth buffer FBO.
										//~ std::string CreationInfo = "default"; ///< for GkEngine exporting.
};

// Simulates ambient light. Covers a volume.
struct AmbientLight {
	AmbientLight() {}
	void bindToUniformAmbientLight(GLuint _pos, GLuint _col, GLuint _range) const {
		glUniform3f(_pos, myPos.x, myPos.y, myPos.z);
		glUniform3f(_col, myColor.x, myColor.y, myColor.z);
		glUniform1f(_range, range);
	}
	glm::vec3 myColor;
	glm::vec3 myPos;
	float range;
	float dist2 = 0; // Used for sorting
	bool shouldRender = true;

	//~ glm::vec4 sphere1 = glm::vec4(0);
	//~ glm::vec4 sphere2 = glm::vec4(0);
	//~ glm::vec4 sphere3 = glm::vec4(0);
	//~ glm::vec4 sphere4 = glm::vec4(0);

	//~ glm::vec3 AABBp1 = glm::vec3(0);
	//~ glm::vec3 AABBp2 = glm::vec3(0);
	//~ glm::vec3 AABBp3 = glm::vec3(0);
	//~ glm::vec3 AABBp4 = glm::vec3(0);

	bool whitelist = false;
};

};	 // namespace gekRender
#endif // PointLight_H
