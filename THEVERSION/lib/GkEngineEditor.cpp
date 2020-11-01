

#include "GkEngineEditor.h"
namespace GSGE{
using namespace gekRender;






/// redirect of GLFW callback
	void GkEditor::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
		if(key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
			holdingShift = true;
		else if(key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
			holdingShift = false;
		
		if(myControlMode == EDITOR_EDIT){
			if(key == GLFW_KEY_O && action == GLFW_PRESS)
				{
					std::cout << "\n(O) Switching to Alt control interface and unpausing..." << std::endl;
					GameEngine->activeControlInterface = altControlInterface;
					GameEngine->setPauseState(false);
				}
			
			if(key == GLFW_KEY_ENTER && action == GLFW_PRESS)
			{
				if(isTranslatingBody || isRotatingBody){
					std::cout << "\nFinishing Translation or Rotation." << std::endl;
					//~ int group; int mask;
					//~ if(selectedBody->getBroadphaseHandle()){
						//~ group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
						//~ mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
					//~ }
					isTranslatingBody = false;
					isRotatingBody = false;
					//~ btTransform t = g2b_transform(originalOpTransform);
					//~ GameEngine->getWorld()->removeRigidBody(selectedBody);
					//~ selectedBody->setCenterOfMassTransform(t);
					//~ GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
				}
			
			}
			if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			{
				if(isPanning){
					isPanning = false;
					camFocalPoint = relativeFocalPointTranslationPoint;
				}
				if(isTranslatingBody && selectedBody){
					std::cout << "\nCancelling Translation." << std::endl;
					int group; int mask;
					if(selectedBody->getBroadphaseHandle()){
						group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
						mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
					}
					isTranslatingBody = false;
					btTransform t = g2b_transform(originalOpTransform);
					GameEngine->getWorld()->removeRigidBody(selectedBody);
					selectedBody->setCenterOfMassTransform(t);
					selectedBody->getMotionState()->setWorldTransform(t);
					GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
				}
				if(isRotatingBody && selectedBody){
					std::cout << "\nCancelling Rotation." << std::endl;
					int group; int mask;
					if(selectedBody->getBroadphaseHandle()){
						group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
						mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
					}
					isRotatingBody = false;
					btTransform t = g2b_transform(originalOpTransform);
					GameEngine->getWorld()->removeRigidBody(selectedBody);
					selectedBody->setCenterOfMassTransform(t);
					selectedBody->getMotionState()->setWorldTransform(t);
					GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
				}
				if(isRotatingEditCamera)
					isRotatingEditCamera = false;
			}
			if(key == GLFW_KEY_T && action == GLFW_PRESS && selectedBody && !isRotatingBody && !isPanning && !isRotatingEditCamera){
				if(!isTranslatingBody){
					std::cout << "\nStarting Translation!!" << std::endl;
					isTranslatingBody = true;
					TranslateX = false;
					TranslateY = false;
					TranslateZ = false;
					originalOpTransform = b2g_transform(selectedBody->getCenterOfMassTransform());
				} else if(isTranslatingBody && selectedBody){
					std::cout << "\nCancelling Translation." << std::endl;
					int group; int mask;
					if(selectedBody->getBroadphaseHandle()){
						group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
						mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
					}
					isTranslatingBody = false;
					btTransform t = g2b_transform(originalOpTransform);
					GameEngine->getWorld()->removeRigidBody(selectedBody);
					selectedBody->setCenterOfMassTransform(t);
					selectedBody->getMotionState()->setWorldTransform(t);
					GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
				}
			}
			if(key == GLFW_KEY_R && action == GLFW_PRESS && selectedBody && !isTranslatingBody && !isPanning && !isRotatingEditCamera){
				if(isRotatingBody){
					std::cout << "\nCancelling Rotation." << std::endl;
					int group; int mask;
					if(selectedBody->getBroadphaseHandle()){
						group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
						mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
					}
					isRotatingBody = false;
					btTransform t = g2b_transform(originalOpTransform);
					GameEngine->getWorld()->removeRigidBody(selectedBody);
					selectedBody->setCenterOfMassTransform(t);
					selectedBody->getMotionState()->setWorldTransform(t);
					GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
				} else {
					std::cout << "\nStarting Rotation!" << std::endl;
					originalOpTransform = b2g_transform(selectedBody->getCenterOfMassTransform());
					RotateX = false;
					RotateY = false;
					RotateZ = false;
					isRotatingBody = true;
				}
			}
			if(isTranslatingBody && key == GLFW_KEY_X && action == GLFW_PRESS){
				std::cout << "\nToggled X Translation." << std::endl;
				TranslateX = !TranslateX;
			}
			if(isTranslatingBody && key == GLFW_KEY_Z && action == GLFW_PRESS){
				std::cout << "\nToggled Z Translation." << std::endl;
				TranslateZ = !TranslateZ;
			}
			if(isTranslatingBody && key == GLFW_KEY_C && action == GLFW_PRESS){
				std::cout << "\nToggled Y Translation." << std::endl;
				TranslateY = !TranslateY;
			}
			
			if(isRotatingBody && key == GLFW_KEY_X && action == GLFW_PRESS){
				std::cout << "\nToggled X Rotation." << std::endl;
				RotateX = !RotateX;
			}
			if(isRotatingBody && key == GLFW_KEY_Z && action == GLFW_PRESS){
				std::cout << "\nToggled Z Rotation." << std::endl;
				RotateZ = !RotateZ;
			}
			if(isRotatingBody && key == GLFW_KEY_C && action == GLFW_PRESS){
				std::cout << "\nToggled Y Rotation." << std::endl;
				RotateY = !RotateY;
			}
			
			
			if(selectedObject && key == GLFW_KEY_DELETE && action == GLFW_PRESS){
				selectedObject->markForDeletion();
				isTranslatingBody = false;
				isRotatingBody = false;
			}
			if(selectedLight && key == GLFW_KEY_DELETE && action == GLFW_PRESS){
				//~ std::cout << "\nMARKER 0" << std::endl;
				selectedLight->deleteLight(GameEngine);
				//~ std::cout << "\nMARKER 0.25" << std::endl;
				LightSphere->deregisterInstance(selectedLight->myMeshInstance);
				//~ std::cout << "\nMARKER 0.5" << std::endl;
				GameEngine->getWorld()->removeRigidBody(selectedLight->rigidbody);
				//~ std::cout << "\nMARKER 1" << std::endl;
				delete selectedLight;
				for(auto*& l : LightWidgets)
					if(l == selectedLight)
						l = nullptr;
				LightWidgets.erase(
					std::remove(LightWidgets.begin(), LightWidgets.end(), nullptr), LightWidgets.end()
				);
				selectedBody = nullptr;
				selectedEntityComponent = nullptr;
				selectedObject = nullptr;
				isTranslatingBody = false;
				isRotatingBody = false;
				//~ std::cout << "\nMARKER 2" << std::endl;
			}
		}
	}
	
	
	
	
	/// redirect of GLFW callback
	void GkEditor::mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
		if(myControlMode == EDITOR_EDIT){
			
			//Begin Panning.
			if(button == GLFW_MOUSE_BUTTON_3 && action == GLFW_PRESS && holdingShift && !isPanning){
				glm::vec3 clickRayEndPos = GameEngine->MainCamera.getClickRay(glm::vec2(ncursorPos[0], ncursorPos[1])) * camDistance + GameEngine->MainCamera.pos;
				//The clickray has "grabbed" a point in 3-d space.
				relativeCameraTranslationPoint = clickRayEndPos;
				relativeFocalPointTranslationPoint = camFocalPoint;
				//~ camFocalPoint = clickRayEndPos;
				isPanning = true;
			} else if(button == GLFW_MOUSE_BUTTON_3 && action == GLFW_RELEASE && isPanning){
				isPanning = false;
			} 
			
			if(button == GLFW_MOUSE_BUTTON_3 && action == GLFW_PRESS && !isPanning && !holdingShift){
				isRotatingEditCamera = true;
				//~ std::cout << "\nRotating Edit Camera mode ON!" << std::endl;
			} else if(button == GLFW_MOUSE_BUTTON_3 && action == GLFW_RELEASE && isRotatingEditCamera){
				isRotatingEditCamera = false;
			}
			//Select an object.
			if(!isPanning && !isRotatingEditCamera && !isTranslatingBody && button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS){
				glm::vec3 clickRayEndPos = GameEngine->MainCamera.getClickRay(glm::vec2(ncursorPos[0], ncursorPos[1])) * GameEngine->MainCamMaxClipPlane + GameEngine->MainCamera.pos;
				btVector3 d = g2b_vec3(clickRayEndPos);
				btVector3 p = g2b_vec3(GameEngine->MainCamera.pos);
				btCollisionWorld::ClosestRayResultCallback RayCallback(p, d);
				RayCallback.m_collisionFilterGroup = COL_NORMAL | COL_EDITOR | COL_EDITOR_RAY;
				RayCallback.m_collisionFilterMask = COL_NORMAL | COL_EDITOR | COL_CHARACTERS;
				GameEngine->getWorld()->rayTest(p,d, RayCallback);
				if(RayCallback.hasHit()){
					selectedBody = (btRigidBody*)btRigidBody::upcast(RayCallback.m_collisionObject);
					if(selectedBody)
						std::cout << "\nHIT A BODY!" << std::endl;
				}
				selectedEntityComponent = nullptr;
				selectedObject = nullptr;
				selectedLight = nullptr;
				GkCollisionInfo* bodycolinfo = nullptr;
				if(selectedBody)
					 bodycolinfo = (GkCollisionInfo*)selectedBody->getUserPointer();
				if(bodycolinfo && bodycolinfo->myGkObject)
					selectedObject = (GkObject*)bodycolinfo->myGkObject;
				if(bodycolinfo && bodycolinfo->userData == "GkLightEditorWidget")
					{
						std::cout << "\nIT'S A LIGHT!" << std::endl;
						selectedLight = (GkLightEditorWidget*)bodycolinfo->userPointer;
					}
			}
			if(button == GLFW_MOUSE_BUTTON_2 && action == GLFW_RELEASE){
				if(selectedBody && isTranslatingBody){
					std::cout << "\nCancelling Translation." << std::endl;
					int group; int mask;
					if(selectedBody->getBroadphaseHandle()){
						group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
						mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
					}
					isTranslatingBody = false;
					btTransform t = g2b_transform(originalOpTransform);
					GameEngine->getWorld()->removeRigidBody(selectedBody);
					selectedBody->setCenterOfMassTransform(t);
					selectedBody->getMotionState()->setWorldTransform(t);
					GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
				}
				//TODO reset rotating body rotation.
				if(selectedBody && isRotatingBody){
					std::cout << "\nCancelling Rotation." << std::endl;
					int group; int mask;
					if(selectedBody->getBroadphaseHandle()){
						group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
						mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
					}
					isRotatingBody = false;
					btTransform t = g2b_transform(originalOpTransform);
					GameEngine->getWorld()->removeRigidBody(selectedBody);
					selectedBody->setCenterOfMassTransform(t);
					selectedBody->getMotionState()->setWorldTransform(t);
					GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
				}
				selectedBody = nullptr;
				selectedLight = nullptr;
				selectedObject = nullptr;
				selectedEntityComponent = nullptr;
			}
			if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && (isTranslatingBody || isRotatingBody) && selectedBody){
				std::cout << "\nFinishing Translation or Rotation." << std::endl;
				isTranslatingBody = false;
				isRotatingBody = false;
			}
		}
	}
	/// redirect of GLFW callback with added details.
	void GkEditor::cursor_position_callback(
		GLFWwindow* window, 
		double xpos, double ypos, 
		double nxpos, double nypos, 
		glm::vec2 CursorDelta, glm::vec2 NormalizedCursorDelta)
	{
		cursorPos[0] = xpos;
		cursorPos[1] = ypos;
		
		ncursorPos[0] = nxpos;
		ncursorPos[1] = nypos;
		if(myControlMode == EDITOR_EDIT){
			if(isPanning && !isTranslatingBody && !isRotatingBody){
				auto left = GameEngine->MainCamera.getRight();
				auto up = GameEngine->MainCamera.up;
				camFocalPoint += left * camDistance * NormalizedCursorDelta.x;
				camFocalPoint += up * camDistance * NormalizedCursorDelta.y;
			}
			if(isRotatingEditCamera && !isTranslatingBody && !isRotatingBody)
			{
				GameEngine->MainCamera.rotateY(-CursorDelta.x * 0.001f);
				GameEngine->MainCamera.pitch(CursorDelta.y * 0.001f);
			}
			if(!selectedBody) isTranslatingBody = false;
			if(isTranslatingBody && selectedBody)
			{
				glm::vec3 clickRayEndPos = GameEngine->MainCamera.getClickRay(glm::vec2(ncursorPos[0], ncursorPos[1])) * camDistance + GameEngine->MainCamera.pos;
				Transform translation_transform = originalOpTransform;
				glm::vec3 p = translation_transform.getPos();
				if(GridSnap){
					clickRayEndPos.x  = ((long int)(clickRayEndPos.x / GridUnitSize)) * GridUnitSize;
					clickRayEndPos.y  = ((long int)(clickRayEndPos.y / GridUnitSize)) * GridUnitSize;
					clickRayEndPos.z  = ((long int)(clickRayEndPos.z / GridUnitSize)) * GridUnitSize;
				}
				if(TranslateX) p.x = clickRayEndPos.x;
				if(TranslateY) p.y = clickRayEndPos.y;
				if(TranslateZ) p.z = clickRayEndPos.z;
				int group; int mask;
				if(selectedBody->getBroadphaseHandle()){
					group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
					mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
				}
				translation_transform.setPos(p);
				btTransform t = g2b_transform(translation_transform);
				GameEngine->getWorld()->removeRigidBody(selectedBody);
				selectedBody->setCenterOfMassTransform(t);
				selectedBody->getMotionState()->setWorldTransform(t);
				GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
			}
			if(isRotatingBody && selectedBody){
				Transform current;
				btTransform t;
				t = selectedBody->getCenterOfMassTransform();
				current = b2g_transform(t);
				glm::vec3 r(0,0,0);
				glm::quat r_orig = current.getRotQUAT();
				current = originalOpTransform;
				//~ std::cout << "R.y before: " << std::to_string(glm::degrees(r.y)) << std::endl;
				if(RotateX) r.x += NormalizedCursorDelta.x + NormalizedCursorDelta.y;
				if(RotateY) r.y += NormalizedCursorDelta.x + NormalizedCursorDelta.y;
				if(RotateZ) r.z += NormalizedCursorDelta.x + NormalizedCursorDelta.y;
				//~ std::cout << "R.y after: " << std::to_string(glm::degrees(r.y)) << std::endl;
				current.setRotQuat(glm::quat(r) * r_orig);
				t = g2b_transform(current);
				int group; int mask;
				if(selectedBody->getBroadphaseHandle()){
					group = selectedBody->getBroadphaseHandle()->m_collisionFilterGroup;
					mask = selectedBody->getBroadphaseHandle()->m_collisionFilterMask;
				}
				GameEngine->getWorld()->removeRigidBody(selectedBody);
				selectedBody->setCenterOfMassTransform(t);
				selectedBody->getMotionState()->setWorldTransform(t);
				GameEngine->getWorld()->addRigidBody(selectedBody, group, mask);
			}
		} 
	}
	/// redirect of GLFW callback
	void GkEditor::char_callback(GLFWwindow* window, unsigned int codepoint){
		if(myControlMode == EDITOR_EDIT){
		
		
		}
	}
	/// redirect of GLFW callback
	void GkEditor::scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
		if(myControlMode == EDITOR_EDIT){
			if(yoffset > 0.1) //Scrolling up.
				camDistance *= 0.9;
			else if (yoffset < 0.1) //scrolling down.
				camDistance *= 1.1;
			if(camDistance < 0.01) camDistance = 0.01;
			if(camDistance > 2000) camDistance = 2000;
		
		}
	}

	GkEditor::GkEditor(GkEngine* _GameEngine, uint _w, uint _h){
		width = _w;
		height = _h;
		GameEngine = _GameEngine;
		SelectorBox = new Mesh(
			createBox(1,1,1,glm::vec3(0,1,0)),
			false, true, true
		);
		LightSphere = new Mesh(
			createSphere(1.0, 6, 6, glm::vec3(204.0/255.0, 204.0/255.0, 0)),
			false, true, true
		);
		GameEngine->getScene()->registerMesh(SelectorBox);
		GameEngine->getScene()->registerMesh(LightSphere);
		SelectorBox->pushTexture(
			GameEngine->Resources.getTexture("Editor_Select_Box_Texture.png", true, GL_NEAREST, GL_REPEAT, 0.0f)
		);
		LightSphere->pushTexture(
			GameEngine->Resources.getTexture("Editor_Select_Light_Texture.png", true, GL_NEAREST, GL_REPEAT, 0.0f)
		);
		LightSphere->mesh_meshmask = Draw_Meshmask;
		SelectorBox->mesh_meshmask = Draw_Meshmask;
		BoxInstance.myPhong.emissivity = 1.0f; //Fullbright.
		SelectorBox->registerInstance(&BoxInstance);
		BoxInstance.mymeshmask = Draw_Meshmask;
	}
	
	
	
	
	void GkEditor::safety_call_before_removing_marked_GkObjects(){
		//check if our selected object is marked for deletion.
		if(selectedObject && selectedObject->markedForDeletion)
		{
			selectedBody = nullptr;
			selectedObject = nullptr;
		}
	}
	
	
	
	
	
	
	void GkEditor::tick(){
		for(auto*& pl : GameEngine->Point_Lights) if (pl){
			bool inThere = false;
			for(auto*& lw : LightWidgets) if(lw){
				inThere = inThere || lw->ownsLight(pl);
			}
			if(!inThere){
				//Assign a widget to the light.
				auto* my_lw = addLightWidget();
				LightSphere->registerInstance(my_lw->myMeshInstance);
				my_lw->assignLight(pl);
				//~ my_lw->tick();
				GameEngine->getWorld()->addRigidBody(my_lw->rigidbody, COL_EDITOR, COL_EDITOR_RAY | COL_EDITOR | COL_NORMAL);
				//~ GameEngine->getWorld()->stepSimulation(0.0f,0);
				
			}
		}
		for(auto*& al : GameEngine->Amb_Lights) if(al){
			bool inThere = false;
			for(auto*& lw : LightWidgets) if(lw){
				inThere = inThere || lw->ownsLight(nullptr, al);
			}
			if(!inThere){
				//Assign a widget to the light.
				auto* my_lw = addLightWidget();
				LightSphere->registerInstance(my_lw->myMeshInstance);
				my_lw->assignLight(nullptr, al);
				
				GameEngine->getWorld()->addRigidBody(my_lw->rigidbody, COL_EDITOR, COL_EDITOR_RAY | COL_EDITOR | COL_NORMAL);
				//~ GameEngine->getWorld()->stepSimulation(0.0f,0);
			}
		}
		for(auto*& cl : GameEngine->Cam_Lights) if(cl){
			bool inThere = false;
			for(auto*& lw : LightWidgets) if(lw){
				inThere = inThere || lw->ownsLight(nullptr, nullptr, cl);
			}
			if(!inThere){
				//Assign a widget to the light.
				auto* my_lw = addLightWidget();
				LightSphere->registerInstance(my_lw->myMeshInstance);
				my_lw->assignLight(nullptr, nullptr, cl);
				
				GameEngine->getWorld()->addRigidBody(my_lw->rigidbody, COL_EDITOR, COL_EDITOR_RAY | COL_EDITOR | COL_NORMAL);
			}
		}
		if(GameEngine->activeControlInterface == this)
		{
			//We are rotating a body or we are in FLY mode.
			if((isRotatingBody && selectedBody && myControlMode == EDITOR_EDIT))
				GameEngine->setCursorLock(true);
			else
				GameEngine->setCursorLock(false);
			LightSphere->mesh_meshmask = Draw_Meshmask;
			GameEngine->setPauseState(true);
			//~ GameEngine->getWorld()->stepSimulation(0.0f,0);
			for(auto*& lw : LightWidgets) if(lw){
				lw->tick();
				lw->myMeshInstance->mymeshmask = Draw_Meshmask;
			}
			if(selectedBody)
				BoxInstance.mymeshmask = Draw_Meshmask;
			else
				BoxInstance.mymeshmask = No_Draw_Meshmask;
			GameEngine->MainCamera.focusView(camFocalPoint, camDistance);
		} else {
			LightSphere->mesh_meshmask = No_Draw_Meshmask;
			BoxInstance.mymeshmask = No_Draw_Meshmask;
		}
		if(selectedBody)
		{
			//~ std::cout << "\nMaking selection box for selected body... " << std::endl;
			btTransform t;
			Transform t_better;
			//~ selectedBody->getMotionState()->getWorldTransform(t);
			t = selectedBody->getCenterOfMassTransform();
			btVector3 center, extend;
			btAABB aabb;
			selectedBody->getCollisionShape()->getAabb(t, aabb.m_min, aabb.m_max);
			aabb.get_center_extend(center, extend);
			t_better = Transform(
				b2g_vec3(center),
				glm::vec3(0,0,0),
				b2g_vec3(extend)
			);
			//~ std::cout << "\nCenter: " << std::to_string(b2g_vec3(center).x) << " "  << std::to_string(b2g_vec3(center).y) << " " << std::to_string(b2g_vec3(center).z) 
					  //~ << "\nExtend: " << std::to_string(b2g_vec3(extend).x) << " "  << std::to_string(b2g_vec3(extend).y) << " " << std::to_string(b2g_vec3(extend).z)  << std::endl;
			BoxInstance.myTransform = t_better;
		} 
		//~ else {
			//~ isRotatingBody = false;
			//~ isTranslatingBody = false;
			//~ selectedObject = nullptr;
			//~ selectedLight = nullptr;
			//~ selectedEntityComponent = nullptr;
		//~ }
		if(selectedLight)
			std::cout << "\nWe have a light selected!" << std::endl;
	}
	
	
	
	
	//Width and height determine the placement and scale of editor windows.
	void GkEditor::resize(uint _w, uint _h)
	{
		width = _w;
		height = _h;
		//TODO re-setup our current window mode.
	}
	GkEditor::~GkEditor(){
		
		for(auto* item: LightWidgets) if(item){
			if(LightSphere)
				LightSphere->deregisterInstance(item->myMeshInstance);
			delete item;
		}LightWidgets.clear();
		GameEngine->getScene()->deregisterMesh(LightSphere);
		GameEngine->getScene()->deregisterMesh(SelectorBox);
		if(LightSphere) delete LightSphere;
		if(SelectorBox) delete SelectorBox;
	}
	
};
