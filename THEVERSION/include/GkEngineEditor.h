#ifndef GKEDITOR_H
#define GKEDITOR_H
#include "GkEngine.h"

namespace GSGE{
using namespace gekRender;

///< Provides a rigid body that can be selected by the editor interface and moved around
/***
 * GkEditor can move around and orient btRigidBodies.
 * */

class GkLightEditorWidget{
  public:
	GkLightEditorWidget(){
		auto* shape = new btSphereShape(1.0);
		btTransform t;t.setIdentity();
		auto* motion = new btDefaultMotionState(t);
		btVector3 inertia(0,0,0);
		//~ shape->calculateLocalInertia(3, inertia);
		btRigidBody::btRigidBodyConstructionInfo info(0, motion, shape, inertia);
		rigidbody = new btRigidBody(info);
		rigidbody->setUserPointer((void*)&myColInfo);
		myMeshInstance = new MeshInstance();
		myColInfo.userData = "GkLightEditorWidget";
		myColInfo.userPointer = (void*)this;
		myMeshInstance->myPhong.emissivity = 1.0f; // GLOW
	}
	void assignLight(
		PointLight* _pl = nullptr,
		AmbientLight* _al = nullptr,
		CameraLight* _cl = nullptr
	){
		pl = _pl;
		cl = _cl;
		al = _al;
		if(pl)
		{
			
			btTransform t; t = rigidbody->getCenterOfMassTransform();
			Transform t_better = b2g_transform(t);
			t_better.setPos(pl->myPos);
			t = g2b_transform(t_better);
			rigidbody->setCenterOfMassTransform(t);
		}
		if(al){
			btTransform t; t = rigidbody->getCenterOfMassTransform();
			Transform t_better = b2g_transform(t);
			t_better.setPos(al->myPos);
			t = g2b_transform(t_better);
			rigidbody->setCenterOfMassTransform(t);
		}
		if(cl){
			btTransform t; t = rigidbody->getCenterOfMassTransform();
			Transform t_better = b2g_transform(t);
			t_better.setPos(cl->myCamera.pos);
			t_better.setRotQuat(
				faceTowardPoint(
					cl->myCamera.pos, 
					cl->myCamera.pos + cl->myCamera.forward, 
					cl->myCamera.up
				)
			);
			t = g2b_transform(t_better);
			rigidbody->setCenterOfMassTransform(t);
		}
	}
	bool ownsLight(
		PointLight* _pl = nullptr,
		AmbientLight* _al = nullptr,
		CameraLight* _cl = nullptr
	){
		if(pl)
			return pl == _pl;
		if(al)
			return al == _al;
		if(cl)
			return cl == _cl;
		return false;
	}
	~GkLightEditorWidget(){
		if(rigidbody)
			{
				delete rigidbody->getCollisionShape();
				delete rigidbody->getMotionState();
				delete rigidbody;
			}
		if(myMeshInstance)
			delete myMeshInstance;
	}
	void deleteLight(GkEngine* GE){
		//TODO: Remove the light from the engine AND the GkScene in the engine.
		GkScene* theScene = GE->getScene();
		theScene->deregisterPointLight(pl);
		for(auto*& _pl : GE->Point_Lights)
			if( _pl == pl)
				_pl = nullptr;
		theScene->deregisterAmbLight(al);
		for(auto*& _al : GE->Amb_Lights)
			if( _al == al)
				_al = nullptr;
		theScene->deregisterCamLight(cl);
		for(auto*& _cl : GE->Cam_Lights)
			if( _cl == cl)
				_cl = nullptr;
		delete al; delete pl;
		//A little more involved for cameralights.
		//We have to delete the shadowmapping FBO.
		if(cl && cl->myShadowMappingFBO)
		{
			delete (FBO*)cl->myShadowMappingFBO;
			for(auto*& shadow_fbo: GE->Cam_Light_Shadowmap_FBOs)
				if(cl->myShadowMappingFBO == shadow_fbo)
					shadow_fbo = nullptr;
		} delete cl;
		al = nullptr;
		cl = nullptr;
		pl = nullptr;
	}
	void tick(){
		if(!rigidbody) return;
		btTransform t; t = rigidbody->getCenterOfMassTransform();
		Transform t_better = b2g_transform(t);
		t_better.setScale(glm::vec3(1,1,1));
		if(pl) pl->myPos = t_better.getPos();
		if(al) al->myPos = t_better.getPos();
		if(cl) cl->myCamera.pos = t_better.getPos();
		//Orient cameralight to match rigid body.
		if(cl){
			glm::vec4 forward(0,0,-1,0);
			glm::vec4 up(0,1,0,0);
			//Multiply by the transform matrix 
			forward = t_better.getModel() * forward;
			up = t_better.getModel() * up;
			cl->myCamera.forward = forward;
			cl->myCamera.up = up;
		}
		if(myMeshInstance) myMeshInstance->myTransform = t_better;
	}
	btRigidBody* rigidbody = nullptr;
	PointLight* pl = nullptr;
	AmbientLight* al = nullptr;
	CameraLight* cl = nullptr;
	MeshInstance* myMeshInstance = nullptr;
	GkCollisionInfo myColInfo;
};


//~ void EditorXButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void SaveButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void LoadButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void SpawnObjectButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void ApplyRigidBodySettingsButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void CreateParticleRendererButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void SelectParticleRendererButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void ApplyParticleRendererSettingsButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void SelectObjectButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void ApplyObjectSettingsButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void SelectECButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void ReadFromMapsECButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void WriteToMapsECButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void ApplyLightSettingsButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ //NOTE: Intelligently construct light editor based on selected light.
//~ //Make sure we set the selectedBody to the widget, of course.
//~ void SelectLightButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void CreateCamLightButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void CreatePointLightButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void CreateAmbLightButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void ApplySettingsButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void SaveSettingsButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void LoadSettingsButtonClick(GkControlInterface* me, int button, int action, int mods);
//~ void QuitEditorButtonClick(GkControlInterface* me, int button, int action, int mods);
///< Control method for GkEditor.
/***
 * 3 distinct modes:
 * 1) Passthrough. The cursor is on screen and one or more of the windows is in the foreground.
 * 2) Fly. The cursor is locked and off. The voxel world can be editted in this mode.
 * 3) Edit. The cursor is on-screen. Move camera with arrow keys (Fixed to most relevant axes), camera rotates around its focal
 * point when mouse3 is held down, but pans when SHIFT is held. Scrolling moves the camera backward or forward.
 * 
 * HOW MOVING OBJECTS WILL WORK
 * We take the distance from the camera to the focal point. Call this distance M.
 * When we use the T translate tool. we shoot out a ray from the camera according to the mouse's position.
 * We then multiply it by M.
 * We then position the object at the end of this ray (its center at that position)
 * 
 * If we have grid snapping on then it makes the coordinates integer multiples of the grid snap unit.
 * 
 * HOW THE SUBWINDOWS WILL WORK
 * There is only one subwindow.
 * */
class GkEditor: public GkControlInterface{
	public:
		GkEditor(GkEngine* _GameEngine, uint _w = 800, uint _h = 600);
		void safety_call_before_removing_marked_GkObjects();
		void tick();
		//Width and height determine the placement and scale of editor windows.
		void resize(uint _w, uint _h);
		virtual ~GkEditor();
		/// redirect of GLFW callback
		virtual void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) override;
		/// redirect of GLFW callback
		virtual void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) override;
		/// redirect of GLFW callback with added details.
		virtual void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
											  glm::vec2 NormalizedCursorDelta) override;
		/// redirect of GLFW callback
		virtual void char_callback(GLFWwindow* window, unsigned int codepoint) override;
		/// redirect of GLFW callback
		virtual void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) override;
		GkLightEditorWidget* addLightWidget(){
			for(auto*& lw : LightWidgets) if(!lw){
				return lw;
			}
			LightWidgets.push_back(new GkLightEditorWidget());
			return LightWidgets.back();
		}
		enum EditorControlModes : uint{
			EDITOR_EDIT = 2
		};
		EditorControlModes myControlMode = EDITOR_EDIT; //The editor starts in EDITOR_EDIT
		enum WindowMode : uint{
			WINDOW_OFF = 0,
		};
		WindowMode myWindowMode = WINDOW_OFF;
		std::string FileMenuReturnSpot = "";
		btRigidBody* selectedBody = nullptr;
		GkObject* selectedObject = nullptr;
		GkLightEditorWidget* selectedLight = nullptr;
		GkEntityComponent* selectedEntityComponent = nullptr;
		//The control interface to switch back to when switching out of the editor.
		GkControlInterface* altControlInterface = nullptr;
		//GkUIWindow CurrentWindow;
		//~ GkUIWindow FileMenuWindow; //Prompt to select a file.
		//~ GkUIWindow SettingsWindow; //Prompt to alter the editor's settings.
		Transform originalOpTransform;
		bool isTranslatingBody = false;
		bool TranslateX = false;
		bool TranslateY = false;
		bool TranslateZ = false;
		bool isRotatingBody = false;
		bool RotateX = false;
		bool RotateY = false;
		bool RotateZ = false;
		bool holdingShift = false;
		int Draw_Meshmask = 5;
		int No_Draw_Meshmask = 1;
		bool inUse = true;
		bool GridSnap = false;
		float GridUnitSize = 0.25f;
		float RotateSnapUnit = 15.0f;//In degrees.
		glm::vec3 camFocalPoint = glm::vec3(0,0,0);
		bool isPanning = false;
		bool isRotatingEditCamera = false;
		//Remember where the clickray ended when we 
		glm::vec3 relativeTranslationPoint = glm::vec3(0,0,0);
		glm::vec3 relativeCameraTranslationPoint = glm::vec3(0,0,0);
		glm::vec3 relativeFocalPointTranslationPoint = glm::vec3(0,0,0);
		float camDistance = 10.0f;
		uint width = 800;
		uint height = 600;
		GkEngine* GameEngine = nullptr;
		std::vector<GkLightEditorWidget*> LightWidgets;
		
		///< Callback to clear your scene of everything before GkEngine->Load is invoked.
		void (*ClearSceneCallback)(void) = nullptr;
		MeshInstance BoxInstance; //Single object selection.
		Mesh* SelectorBox = nullptr;
		Mesh* LightSphere = nullptr;
	protected:
};

}; //eof namespace
#endif
