#include "SceneAPI.h"
#include "gekrender.h"
//(C) DMHSW 2018 All Rights Reserved

namespace gekRender {

// Render shadows to rendertarget (Just a depth buffer render)
void GkScene::drawShadowPipeline(int meshmask, FBO* CurrentRenderTarget, Camera* CurrentRenderCamera, bool doFrameBufferChecks) {
	// Used for error checking
	GLenum communism; // communism is a mistake
	if (!CurrentRenderTarget) {
		std::cout << "\nMust have render target to render shadows";
		return;
	}
	if (nextRenderPipelineCallWillBeAfterVsync) // What this does: Prevent the
												// user from accidentally
												// increasing lag exponentially
												// by reshaping a mesh while
												// it's rendering. Also, some
												// other stuff maybe. TODO:
												// Check it out later
	{
		nextRenderPipelineCallWillBeAfterVsync = false;
	}

	/* Error Check code. Paste where you need it.
	
	// communism = glGetError(); //Ensure there are no errors listed before we
	start.
	// if (communism != GL_NO_ERROR) //if communism has made an error (which is
	pretty typical)
	// {
			// std::cout<<"\n OpenGL reports an ERROR!";
			// if (communism == GL_INVALID_ENUM)
					// std::cout<<"\n Invalid enum.";
			// if (communism == GL_INVALID_OPERATION)
					// std::cout<<"\n Invalid operation.";
			// if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
					// std::cout <<"\n Invalid Framebuffer Operation.";
			// if (communism == GL_OUT_OF_MEMORY)
			// {
					// std::cout <<"\n Out of memory. You've really done it now.
	I'm so angry, i'm going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
					// std::abort();
			// }
	// }
	*/

	if (!ShadowOpaqueMainShader || !ShowTextureShader /*|| !CompositionShader*/) { // if you don't have one
																				   // of these, there's no
																				   // chance we can render
																				   // shadows
		std::cout << "\nYou got kicked out boi 3";
		return; // gtfo
	}

	glEnable(GL_CULL_FACE); // Enable culling faces
	glCullFace(GL_BACK);	// cull faces with clockwise winding
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	FBO* MainFBO = nullptr;
	FBO* TransparencyFBO = nullptr;

	MainFBO = CurrentRenderTarget;
	//~ TransparencyFBO = RenderTarget_Transparent;
	// CurrentRenderTarget->BindRenderTarget();

	MainFBO->bindRenderTarget();
	FBO::clearTexture(1, 1, 1, 1);
	ShadowOpaqueMainShader->bind(); // Bind the shader!

	// Runs whenever the window is resized or a shader is reassigned, so that we
	// only need to get uniform locations once. It's not optimized fully yet...
	if (HasntRunYet_Shadows) {
		HasntRunYet_Shadows = false;
		MainShaderUniforms_SHADOWTEMP[MAINSHADER_WORLD2CAMERA] = ShadowOpaqueMainShader->getUniformLocation("World2Camera"); // World --> NDC
		MainShaderUniforms_SHADOWTEMP[MAINSHADER_MODEL2WORLD] = ShadowOpaqueMainShader->getUniformLocation("Model2World");   // Model --> World
		MainShaderUniforms_SHADOWTEMP[MAINSHADER_CAMERAPOS] = ShadowOpaqueMainShader->getUniformLocation("CameraPos");
		MainShaderUniforms_SHADOWTEMP[MAINSHADER_ENABLE_TRANSPARENCY] = ShadowOpaqueMainShader->getUniformLocation("EnableTransparency");
		MainShaderUniforms_SHADOWTEMP[MAINSHADER_IS_INSTANCED] = ShadowOpaqueMainShader->getUniformLocation("is_instanced");
		MainShaderUniforms_SHADOWTEMP[MAINSHADER_BONE_UBO_LOCATION] = ShadowOpaqueMainShader->getUniformBlockIndex("bone_data");
		MainShaderUniforms_SHADOWTEMP[MAINSHADER_HAS_BONES] = ShadowOpaqueMainShader->getUniformLocation("has_bones");
		MainShaderUniforms_SHADOWTEMP[MAINSHADER_RENDERFLAGS] = ShadowOpaqueMainShader->getUniformLocation("renderflags");
	}

	glUniform3f(MainShaderUniforms_SHADOWTEMP[MAINSHADER_CAMERAPOS], CurrentRenderCamera->pos.x, CurrentRenderCamera->pos.y, CurrentRenderCamera->pos.z);

	{ glUniformMatrix4fv(MainShaderUniforms_SHADOWTEMP[MAINSHADER_WORLD2CAMERA], 1, GL_FALSE, &(CurrentRenderCamera->getViewProjection()[0][0])); }
	// glEnable(GL_CULL_FACE);
	// Now that we have the shader stuff set up, let's get to rendering!
	glEnableVertexAttribArray(0); // Position
	glEnableVertexAttribArray(1); // Texture
	glEnableVertexAttribArray(2); // Normal
	glEnableVertexAttribArray(3); // Color
	// Render Opaque Objects. Note we configured depth masking and blending for
	// this pass much earlier on.
	if (Meshes.size() > 0)						   // if there are any
		for (size_t i = 0; i < Meshes.size(); i++) // for all of them
			if (Meshes[i])						   // don't call methods on nullptrs
			{
				// unsigned int flagerinos = Meshes[i]->getFlags(); //Set flags
				// glUniform1ui(MainShaderUniforms[MAINSHADER_RENDERFLAGS],
				// flagerinos);
				// //Set flags on GPU
				Meshes[i]->drawInstancesPhong(
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_MODEL2WORLD], // Model->World transformation
																		   // matrix
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_RENDERFLAGS],
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_SPECREFLECTIVITY], // Specular reflective
																				// material component
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_SPECDAMP],			// Specular dampening material
																				// component
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_DIFFUSIVITY],		// Diffusivity. Reaction to
																				// diffuse light.
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_EMISSIVITY], MainShaderUniforms_SHADOWTEMP[MAINSHADER_ENABLE_CUBEMAP_REFLECTIONS],
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_ENABLE_CUBEMAP_DIFFUSION], MainShaderUniforms_SHADOWTEMP[MAINSHADER_ENABLE_TRANSPARENCY],
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_IS_INSTANCED], MainShaderUniforms_SHADOWTEMP[MAINSHADER_BONE_UBO_LOCATION], BoneDataUBOBindPoint,
					MainShaderUniforms_SHADOWTEMP[MAINSHADER_HAS_BONES], 0, ShadowOpaqueMainShader,
					false,											 // NOT transparent
					(CurrentRenderTarget != nullptr) ? true : false, // is it to render target?
					meshmask,
					false, // Phong?
					true,  // Do not use maps
					false  // Force non-instanced
				);
			}

	glDisableVertexAttribArray(0); // Position
	glDisableVertexAttribArray(1); // Texture
	glDisableVertexAttribArray(2); // Normal
	glDisableVertexAttribArray(3); // Color

	{
		if (doFrameBufferChecks)
			while (true) {
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
					break;
			}
		nextRenderPipelineCallWillBeAfterVsync = false;
	}

} // eof drawShadowPipeline

void GkScene::drawPipeline(int meshmask, FBO* CurrentRenderTarget, FBO* RenderTarget_Transparent, Camera* CurrentRenderCamera, bool doFrameBufferChecks,
						   glm::vec4 backgroundColor, glm::vec2 fogRangeDescriptor,
						   bool Render_Transparent) { // Approaches spaghetti levels of unreadability
	// Used for error checking
	GLenum communism;
	bool doTransparency = !(CurrentRenderTarget && !RenderTarget_Transparent);

	if (nextRenderPipelineCallWillBeAfterVsync) // Reset everything if this
												// render call was immediately
												// after the vsync
	{
		nextRenderPipelineCallWillBeAfterVsync = false;
	}

	if (!SceneCamera || !MainShader || !MainShaderUniforms || !ShowTextureShader /*|| !CompositionShader*/) { // if one is not
																											  // present, no chance
		std::cout << "\nYou got kicked out boi" << std::endl;
		return; // gtfo
	}

	// Setup Camera mat4s so that we can send in the addresses.
	if (CurrentRenderCamera == nullptr) {
		SceneRenderCameraMatrix = SceneCamera->getViewProjection();
		SceneRenderCameraViewMatrix = SceneCamera->getViewMatrix();
		SceneRenderCameraProjectionMatrix = SceneCamera->getProjection();
	}

	// CONFIGURE BLENDING AND CULLING FOR SKYBOX RENDERING
	glEnable(GL_CULL_FACE); // Enable culling faces
	glCullFace(GL_BACK);	// cull faces with clockwise winding
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	FBO* MainFBO = nullptr;
	FBO* TransparencyFBO = nullptr;
	if (CurrentRenderTarget == nullptr) {
		MainFBO = FboArray[FORWARD_BUFFER1];
		TransparencyFBO = FboArray[FORWARD_BUFFER2];
		//std::cout << "\nGot the right FBOs!" << std::endl;
	} else {
		MainFBO = CurrentRenderTarget;
		TransparencyFBO = RenderTarget_Transparent;
		// CurrentRenderTarget->BindRenderTarget();
	}
	MainFBO->bindRenderTarget();
	//std::cout << "\nRight before the FBO::clearTexture that i'm investigating rn" << std::endl;
	FBO::clearTexture(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
	/*
	
	Skybox Render
	customMainShaderBinds

	*/
	// This draws the skybox in the background
	if (SkyBoxCubemap && SkyboxShader) {
		//~ std::cout << "\nAttempting to draw Skybox!" << std::endl;
		SkyboxShader->bind();
		if (!haveInitializedSkyboxUniforms) {
			SkyboxUniforms[SKYBOX_WORLD2CAMERA] = SkyboxShader->getUniformLocation("World2Camera");
			SkyboxUniforms[SKYBOX_MODEL2WORLD] = SkyboxShader->getUniformLocation("Model2World");
			SkyboxUniforms[SKYBOX_VIEWMATRIX] = SkyboxShader->getUniformLocation("viewMatrix");
			SkyboxUniforms[SKYBOX_PROJECTION] = SkyboxShader->getUniformLocation("projection");
			SkyboxUniforms[SKYBOX_WORLDAROUNDME] = SkyboxShader->getUniformLocation("worldaroundme");
			haveInitializedSkyboxUniforms = true; // We've done it boys.
		}

		if (CurrentRenderCamera == nullptr) {
			glUniformMatrix4fv(SkyboxUniforms[SKYBOX_WORLD2CAMERA], 1, GL_FALSE, &SceneRenderCameraMatrix[0][0]);
			glUniformMatrix4fv(SkyboxUniforms[SKYBOX_VIEWMATRIX], 1, GL_FALSE, &SceneRenderCameraViewMatrix[0][0]);
			glUniformMatrix4fv(SkyboxUniforms[SKYBOX_PROJECTION], 1, GL_FALSE, &SceneRenderCameraProjectionMatrix[0][0]);
		} else {
			//&((*RendertargetCameras[RendertargetCameras.size()-3])[0][0])
			glUniformMatrix4fv(SkyboxUniforms[SKYBOX_WORLD2CAMERA], 1, GL_FALSE, &(CurrentRenderCamera->getViewProjection()[0][0]));
			glUniformMatrix4fv(SkyboxUniforms[SKYBOX_VIEWMATRIX], 1, GL_FALSE, &(CurrentRenderCamera->getViewMatrix()[0][0]));
			glUniformMatrix4fv(SkyboxUniforms[SKYBOX_PROJECTION], 1, GL_FALSE, &(CurrentRenderCamera->getProjection()[0][0]));
		}
		skybox_transform.reTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0),
									 glm::vec3(SceneCamera->jafar * 0.5, SceneCamera->jafar * 0.5, SceneCamera->jafar * 0.5));

		SkyBoxCubemap->bind(1); // Bind to 1
		glDepthMask(GL_FALSE);  // We want it to appear infinitely far away

		glUniform1i(SkyboxUniforms[SKYBOX_WORLDAROUNDME], 1);
		glEnableVertexAttribArray(0); // Position
		glEnableVertexAttribArray(2); // Normal
		Skybox_Transitional_Transform = skybox_transform.getModel();
		glUniformMatrix4fv(SkyboxUniforms[SKYBOX_MODEL2WORLD], 1, GL_FALSE, &Skybox_Transitional_Transform[0][0]);
		m_skybox_Mesh->drawGeneric();
		glDepthMask(GL_TRUE); // We would like depth testing to be done again.
	}

	// This is where we would render any objects which don't use the mainshader
	if (customRenderingAfterSkyboxBeforeMainShader != nullptr)
		customRenderingAfterSkyboxBeforeMainShader(meshmask, CurrentRenderTarget, RenderTarget_Transparent, CurrentRenderCamera, doFrameBufferChecks,
												   backgroundColor, fogRangeDescriptor);

	/*
			MAINSHADER RENDERING
	*/
	MainShader->bind(); // Bind the shader!

	// Runs whenever the window is resized or a shader is reassigned, so that we
	// only need to get uniform locations once. It's not optimized fully yet...
	if (HasntRunYet) {

		HasntRunYet = false;
		MainShaderUniforms[MAINSHADER_DIFFUSE] = MainShader->getUniformLocation("diffuse");					  // Literal texture unit
		MainShaderUniforms[MAINSHADER_EMISSIVITY] = MainShader->getUniformLocation("emissivity");			  // Emissivity value
		MainShaderUniforms[MAINSHADER_WORLD2CAMERA] = MainShader->getUniformLocation("World2Camera");		  // World --> NDC
		MainShaderUniforms[MAINSHADER_MODEL2WORLD] = MainShader->getUniformLocation("Model2World");			  // Model --> World
		MainShaderUniforms[MAINSHADER_SPECREFLECTIVITY] = MainShader->getUniformLocation("specreflectivity"); // Specular reflectivity
		MainShaderUniforms[MAINSHADER_SPECDAMP] = MainShader->getUniformLocation("specdamp");				  // Specular dampening
		MainShaderUniforms[MAINSHADER_DIFFUSIVITY] = MainShader->getUniformLocation("diffusivity");			  // Diffusivity
		MainShaderUniforms[MAINSHADER_RENDERFLAGS] = MainShader->getUniformLocation("renderflags");			  // Renderflags. Going to be used again b/c we're
																											  // going to use per-vertex colors again!
		MainShaderUniforms[MAINSHADER_WORLDAROUNDME] = MainShader->getUniformLocation("worldaroundme");
		MainShaderUniforms[MAINSHADER_ENABLE_CUBEMAP_REFLECTIONS] = MainShader->getUniformLocation("enableCubeMapReflections");
		MainShaderUniforms[MAINSHADER_ENABLE_CUBEMAP_DIFFUSION] = MainShader->getUniformLocation("enableCubeMapDiffusivity");
		MainShaderUniforms[MAINSHADER_TEXOFFSET] = MainShader->getUniformLocation("texOffset");
		MainShaderUniforms[MAINSHADER_CAMERAPOS] = MainShader->getUniformLocation("CameraPos");
		MainShaderUniforms[MAINSHADER_ENABLE_TRANSPARENCY] = MainShader->getUniformLocation("EnableTransparency");
		MainShaderUniforms[MAINSHADER_NUM_POINTLIGHTS] = MainShader->getUniformLocation("numpointlights");
		MainShaderUniforms[MAINSHADER_NUM_DIRLIGHTS] = MainShader->getUniformLocation("numdirlights");
		MainShaderUniforms[MAINSHADER_NUM_AMBLIGHTS] = MainShader->getUniformLocation("numamblights");
		MainShaderUniforms[MAINSHADER_NUM_CAMLIGHTS] = MainShader->getUniformLocation("numcamlights");
		MainShaderUniforms[MAINSHADER_IS_INSTANCED] = MainShader->getUniformLocation("is_instanced");
		MainShaderUniforms[MAINSHADER_CAMERATEX1] = MainShader->getUniformLocation("CameraTex1");
		MainShaderUniforms[MAINSHADER_CAMERATEX2] = MainShader->getUniformLocation("CameraTex2");
		MainShaderUniforms[MAINSHADER_CAMERATEX3] = MainShader->getUniformLocation("CameraTex3");
		MainShaderUniforms[MAINSHADER_CAMERATEX4] = MainShader->getUniformLocation("CameraTex4");
		MainShaderUniforms[MAINSHADER_CAMERATEX5] = MainShader->getUniformLocation("CameraTex5");
		MainShaderUniforms[MAINSHADER_CAMERATEX6] = MainShader->getUniformLocation("CameraTex6");
		MainShaderUniforms[MAINSHADER_CAMERATEX7] = MainShader->getUniformLocation("CameraTex7");
		MainShaderUniforms[MAINSHADER_CAMERATEX8] = MainShader->getUniformLocation("CameraTex8");
		MainShaderUniforms[MAINSHADER_CAMERATEX9] = MainShader->getUniformLocation("CameraTex9");
		MainShaderUniforms[MAINSHADER_CAMERATEX10] = MainShader->getUniformLocation("CameraTex10");
		MainShaderUniforms[MAINSHADER_BONE_UBO_LOCATION] = MainShader->getUniformBlockIndex("bone_data");
		//~ std::cout << "\nBONE UBO LOCATION: " <<
		// MainShaderUniforms[MAINSHADER_BONE_UBO_LOCATION];
		MainShaderUniforms[MAINSHADER_HAS_BONES] = MainShader->getUniformLocation("has_bones");

		MainShaderUniforms[MAINSHADER_SKYBOX_CUBEMAP] = MainShader->getUniformLocation("SkyboxCubemap");
		MainShaderUniforms[MAINSHADER_BACKGROUND_COLOR] = MainShader->getUniformLocation("backgroundColor");
		MainShaderUniforms[MAINSHADER_FOG_RANGE] = MainShader->getUniformLocation("fogRange");
	}
	// Custom Bindings
	if (customMainShaderBinds != nullptr)
		customMainShaderBinds(meshmask, CurrentRenderTarget, RenderTarget_Transparent, CurrentRenderCamera, doFrameBufferChecks, backgroundColor,
							  fogRangeDescriptor); // TODO: Why aren't we passing the shader?

	if (CurrentRenderCamera == nullptr)
		glUniform3f(MainShaderUniforms[MAINSHADER_CAMERAPOS], SceneCamera->pos.x, SceneCamera->pos.y, SceneCamera->pos.z);
	else
		glUniform3f(MainShaderUniforms[MAINSHADER_CAMERAPOS], CurrentRenderCamera->pos.x, CurrentRenderCamera->pos.y, CurrentRenderCamera->pos.z);

	//~ communism = glGetError();	 // Ensure there are no errors listed before we
	//~ // start.
	//~ if (communism != GL_NO_ERROR) // if communism has made an error (which is pretty typical)
	//~ {
	//~ std::cout << "\n OpenGL reports an ERROR! (POS 4)";
	//~ if (communism == GL_INVALID_ENUM)
	//~ std::cout << "\n Invalid enum.";
	//~ if (communism == GL_INVALID_OPERATION)
	//~ std::cout << "\n Invalid operation.";
	//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
	//~ std::cout << "\n Invalid Framebuffer Operation.";
	//~ if (communism == GL_OUT_OF_MEMORY) {
	//~ std::cout << "\n Out of memory. You've really done it now. I'm so angry, "
	//~ "i'm "
	//~ "going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
	//~ std::abort();
	//~ }
	//~ }

	glUniform1i(MainShaderUniforms[MAINSHADER_SKYBOX_CUBEMAP], 2);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX1], 3);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX2], 4);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX3], 5);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX4], 6);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX5], 7);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX6], 8);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX7], 9);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX8], 10);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX9], 11);
	glUniform1i(MainShaderUniforms[MAINSHADER_CAMERATEX10], 12);
	if (SkyBoxCubemap)
		SkyBoxCubemap->bind(2);
	// glm::vec4 tempBackgroundColor = glm::vec4(0,0,0,0);
	// glm::vec2 tempFogRange = glm::vec2(500,800);
	glUniform4f(MainShaderUniforms[MAINSHADER_BACKGROUND_COLOR], backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
	glUniform2f(MainShaderUniforms[MAINSHADER_FOG_RANGE], fogRangeDescriptor.x, fogRangeDescriptor.y);

	glUniform1i(MainShaderUniforms[MAINSHADER_DIFFUSE], 0);

	// Do this regardless of the presence of Skyboxcubemap
	glUniform1i(MainShaderUniforms[MAINSHADER_WORLDAROUNDME],
				1); // Cubemap unit 1 is reserved for the cubemap representing
					// the world around the object, for reflections.

	//~ communism = glGetError();	 // Ensure there are no errors listed before we
	//~ // start.
	//~ if (communism != GL_NO_ERROR) // if communism has made an error (which is pretty typical)
	//~ {
	//~ std::cout << "\n OpenGL reports an ERROR! (POS 5)";
	//~ if (communism == GL_INVALID_ENUM)
	//~ std::cout << "\n Invalid enum.";
	//~ if (communism == GL_INVALID_OPERATION)
	//~ std::cout << "\n Invalid operation.";
	//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
	//~ std::cout << "\n Invalid Framebuffer Operation.";
	//~ if (communism == GL_OUT_OF_MEMORY) {
	//~ std::cout << "\n Out of memory. You've really done it now. I'm so angry, "
	//~ "i'm "
	//~ "going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
	//~ std::abort();
	//~ }
	//~ }

	// TODO: Change camera reference for sorting lights to the current render
	// target camera (if applicable)

	if (CurrentRenderCamera == nullptr) {
		glUniformMatrix4fv(MainShaderUniforms[MAINSHADER_WORLD2CAMERA], 1, GL_FALSE, &SceneRenderCameraMatrix[0][0]);
	} else {
		glUniformMatrix4fv(MainShaderUniforms[MAINSHADER_WORLD2CAMERA], 1, GL_FALSE, &(CurrentRenderCamera->getViewProjection()[0][0]));
	}

	// Make the code significantly prettier by packing all the light binding
	// stuff into a function
	if (CurrentRenderCamera)
		organizeUBOforUpload(CurrentRenderCamera);
	else
		organizeUBOforUpload(SceneCamera);

	//~ communism = glGetError();	 // Ensure there are no errors listed before we
	//~ // start.
	//~ if (communism != GL_NO_ERROR) // if communism has made an error (which is pretty typical)
	//~ {
	//~ std::cout << "\n OpenGL reports an ERROR! (POS 6)";
	//~ if (communism == GL_INVALID_ENUM)
	//~ std::cout << "\n Invalid enum.";
	//~ if (communism == GL_INVALID_OPERATION)
	//~ std::cout << "\n Invalid operation.";
	//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
	//~ std::cout << "\n Invalid Framebuffer Operation.";
	//~ if (communism == GL_OUT_OF_MEMORY) {
	//~ std::cout << "\n Out of memory. You've really done it now. I'm so angry, "
	//~ "i'm "
	//~ "going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
	//~ std::abort();
	//~ }
	//~ }

	// glEnable(GL_CULL_FACE);
	// Now that we have the shader stuff set up, let's get to rendering!
	glEnableVertexAttribArray(0); // Position
	glEnableVertexAttribArray(1); // Texture
	glEnableVertexAttribArray(2); // Normal
	glEnableVertexAttribArray(3); // Color
	// Render Opaque Objects. Note we configured depth masking and blending for
	// this pass much earlier on.
	if (Meshes.size() > 0)						   // if there are any
		for (size_t i = 0; i < Meshes.size(); i++) // for all of them
			if (Meshes[i])						   // don't call methods on nullptrs
			{
				// unsigned int flagerinos = Meshes[i]->getFlags(); //Set flags
				// glUniform1ui(MainShaderUniforms[MAINSHADER_RENDERFLAGS],
				// flagerinos);
				// //Set flags on GPU
				Meshes[i]->drawInstancesPhong(MainShaderUniforms[MAINSHADER_MODEL2WORLD], // Model->World
																						  // transformation
																						  // matrix
											  MainShaderUniforms[MAINSHADER_RENDERFLAGS],
											  MainShaderUniforms[MAINSHADER_SPECREFLECTIVITY], // Specular reflective
																							   // material component
											  MainShaderUniforms[MAINSHADER_SPECDAMP],		   // Specular dampening
																							   // material component
											  MainShaderUniforms[MAINSHADER_DIFFUSIVITY],	  // Diffusivity. Reaction
																							   // to diffuse light.
											  MainShaderUniforms[MAINSHADER_EMISSIVITY], MainShaderUniforms[MAINSHADER_ENABLE_CUBEMAP_REFLECTIONS],
											  MainShaderUniforms[MAINSHADER_ENABLE_CUBEMAP_DIFFUSION], MainShaderUniforms[MAINSHADER_ENABLE_TRANSPARENCY],
											  MainShaderUniforms[MAINSHADER_IS_INSTANCED], MainShaderUniforms[MAINSHADER_BONE_UBO_LOCATION],
											  BoneDataUBOBindPoint, MainShaderUniforms[MAINSHADER_HAS_BONES], 
											  MainShaderUniforms[MAINSHADER_TEXOFFSET],
											  MainShader,
											  false,										   // NOT transparent
											  (CurrentRenderTarget != nullptr) ? true : false, // is it to render target?
											  meshmask,
											  true, // Phong?
											  true, // Do not use maps
											  false // Force non-instanced
				);
			}
	//~ // Prep for rendering Transparent Objects
	//~ communism = glGetError();	 // Ensure there are no errors listed before we
	//~ // start.
	//~ if (communism != GL_NO_ERROR) // if communism has made an error (which is pretty typical)
	//~ {
	//~ std::cout << "\n OpenGL reports an ERROR! (POS 7)";
	//~ if (communism == GL_INVALID_ENUM)
	//~ std::cout << "\n Invalid enum.";
	//~ if (communism == GL_INVALID_OPERATION)
	//~ std::cout << "\n Invalid operation.";
	//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
	//~ std::cout << "\n Invalid Framebuffer Operation.";
	//~ if (communism == GL_OUT_OF_MEMORY) {
	//~ std::cout << "\n Out of memory. You've really done it now. I'm so angry, "
	//~ "i'm "
	//~ "going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
	//~ std::abort();
	//~ }
	//~ }
	// IF TEST FOR IF WE HAVE THE COMPOSITION SHADER (DON'T ATTEMPT TO DO
	// TRANSPARENCY UNLESS WE HAVE IT)
	if (CompositionShader && doTransparency && Render_Transparent) {

		glDepthMask(GL_FALSE); // Transparent objects aren't depth tested against each
							   // other but are against opaque objects in the scene.
		glDisable(GL_CULL_FACE);
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		if (TransparencyFBO != nullptr) // JUST TO MAKE SURE...
			TransparencyFBO->bindRenderTarget();
		static GLenum DrawBuffers0[1] = {GL_COLOR_ATTACHMENT0};
		static GLenum DrawBuffers1[1] = {GL_COLOR_ATTACHMENT1};
		// GLenum DrawBuffers2[2] = {GL_COLOR_ATTACHMENT0,
		// GL_COLOR_ATTACHMENT1};
		glDrawBuffers(1, DrawBuffers0);
		FBO::clearTexture(0, 0, 0, 0);
		glDrawBuffers(1, DrawBuffers1);
		FBO::clearTexture(1, 1, 1, 1);
		TransparencyFBO->bindDrawBuffers(); // Do the right thing!

		glEnable(GL_BLEND);
		glBlendFunci(0, GL_ONE, GL_ONE);				  // Additive blending
		glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA); // Uh... I don't know

		// glBlendFunc(GL_ZERO, GL_SRC_COLOR); //Multiplicative Blending
		// Render Transparent Objects
		if (Meshes.size() > 0)						   // if there are any
			for (size_t i = 0; i < Meshes.size(); i++) // for all of them
				if (Meshes[i])						   // don't call methods on nullptrs
				{
					unsigned int flagerinos = Meshes[i]->getFlags(); // Set flags
					glUniform1ui(MainShaderUniforms[MAINSHADER_RENDERFLAGS],
								 flagerinos);												  // Set flags on GPU
					Meshes[i]->drawInstancesPhong(MainShaderUniforms[MAINSHADER_MODEL2WORLD], // Model->World
																							  // transformation
																							  // matrix
												  MainShaderUniforms[MAINSHADER_RENDERFLAGS],
												  MainShaderUniforms[MAINSHADER_SPECREFLECTIVITY], // Specular
																								   // reflective
																								   // material component
												  MainShaderUniforms[MAINSHADER_SPECDAMP],		   // Specular dampening
																								   // material component
												  MainShaderUniforms[MAINSHADER_DIFFUSIVITY],	  // Diffusivity.
																								   // Reaction to diffuse
																								   // light.
												  MainShaderUniforms[MAINSHADER_EMISSIVITY], MainShaderUniforms[MAINSHADER_ENABLE_CUBEMAP_REFLECTIONS],
												  MainShaderUniforms[MAINSHADER_ENABLE_CUBEMAP_DIFFUSION], MainShaderUniforms[MAINSHADER_ENABLE_TRANSPARENCY],
												  MainShaderUniforms[MAINSHADER_IS_INSTANCED], MainShaderUniforms[MAINSHADER_BONE_UBO_LOCATION],
												  BoneDataUBOBindPoint, MainShaderUniforms[MAINSHADER_HAS_BONES],
												  MainShaderUniforms[MAINSHADER_TEXOFFSET],
												  MainShader,
												  true,											   // Transparent.
												  (CurrentRenderTarget != nullptr) ? true : false, // is it to render target?
												  meshmask, true, true, false);
				}
		glDisableVertexAttribArray(0); // Position. NOTE: don't disable if we
									   // want to do screenquads later.
		glDisableVertexAttribArray(1); // Texture
		glDisableVertexAttribArray(2); // Normal
		glDisableVertexAttribArray(3); // Color

		//~ communism = glGetError();	 // Ensure there are no errors listed before we start.
		//~ if (communism != GL_NO_ERROR) // if communism has made an error (which
		//~ // is pretty typical)
		//~ {
		//~ std::cout << "\n OpenGL reports an ERROR! (POS 8)";
		//~ if (communism == GL_INVALID_ENUM)
		//~ std::cout << "\n Invalid enum.";
		//~ if (communism == GL_INVALID_OPERATION)
		//~ std::cout << "\n Invalid operation.";
		//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
		//~ std::cout << "\n Invalid Framebuffer Operation.";
		//~ if (communism == GL_OUT_OF_MEMORY) {
		//~ std::cout << "\n Out of memory. You've really done it now. I'm "
		//~ "so angry, i'm "
		//~ "going to close the program. ARE YOU HAPPY NOW, "
		//~ "DAVE?!?!";
		//~ std::abort();
		//~ }
		//~ }

		// This is the part where we composite to the screen with the
		// compositionshader
		if (customRenderingAfterTransparentObjectRendering)
			customRenderingAfterTransparentObjectRendering(meshmask, CurrentRenderTarget, RenderTarget_Transparent, CurrentRenderCamera, doFrameBufferChecks,
														   backgroundColor, fogRangeDescriptor);
		// INTEL GPU FIX
		if (doFrameBufferChecks)
			while (true) {
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
					break;
			}
		CompositionShader->bind();
		if (HasntRunYet_CompositionShader) {
			/*
			enum{
							OIT_COMP_1,
							OIT_COMP_2,
							OIT_NUM_OIT_COMP_UNIFORMS
					};
					GLuint CompositionShaderUniforms[OIT_NUM_OIT_COMP_UNIFORMS];
			*/
			CompositionShaderUniforms[OIT_COMP_1] = CompositionShader->getUniformLocation("diffuse");
			CompositionShaderUniforms[OIT_COMP_2] = CompositionShader->getUniformLocation("diffuse2");
			HasntRunYet_CompositionShader = false;
		}
		glUniform1i(CompositionShaderUniforms[OIT_COMP_1], 0);
		glUniform1i(CompositionShaderUniforms[OIT_COMP_2], 1);
		MainFBO->bindRenderTarget();
		TransparencyFBO->bindasTexture(0, 0);
		TransparencyFBO->bindasTexture(1, 1);
		glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		screenquadtoFBO(CompositionShader);

		//~ communism = glGetError();	 // Ensure there are no errors listed before we start.
		//~ if (communism != GL_NO_ERROR) // if communism has made an error (which
		//~ // is pretty typical)
		//~ {
		//~ std::cout << "\n OpenGL reports an ERROR! (POS 9)";
		//~ if (communism == GL_INVALID_ENUM)
		//~ std::cout << "\n Invalid enum.";
		//~ if (communism == GL_INVALID_OPERATION)
		//~ std::cout << "\n Invalid operation.";
		//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
		//~ std::cout << "\n Invalid Framebuffer Operation.";
		//~ if (communism == GL_OUT_OF_MEMORY) {
		//~ std::cout << "\n Out of memory. You've really done it now. I'm "
		//~ "so angry, i'm "
		//~ "going to close the program. ARE YOU HAPPY NOW, "
		//~ "DAVE?!?!";
		//~ std::abort();
		//~ }
		//~ }

	} else {
		// Do additive blending
		if (Render_Transparent) {
			glDepthMask(GL_FALSE); // Transparent objects aren't depth tested
								   // against each other but are against opaque
								   // objects in the scene.
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive
			glDisable(GL_CULL_FACE);
			//~ glBlendFunc(GL_ZERO, GL_SRC_COLOR); //Multiplicative Blending

			// Render Transparent Objects
			if (Meshes.size() > 0)						   // if there are any
				for (size_t i = 0; i < Meshes.size(); i++) // for all of them
					if (Meshes[i])						   // don't call methods on nullptrs
					{
						unsigned int flagerinos = Meshes[i]->getFlags(); // Set flags
						glUniform1ui(MainShaderUniforms[MAINSHADER_RENDERFLAGS],
									 flagerinos);												  // Set flags on GPU
						Meshes[i]->drawInstancesPhong(MainShaderUniforms[MAINSHADER_MODEL2WORLD], // Model->World
																								  // transformation
																								  // matrix
													  MainShaderUniforms[MAINSHADER_RENDERFLAGS],
													  MainShaderUniforms[MAINSHADER_SPECREFLECTIVITY], // Specular
																									   // reflective
																									   // material
																									   // component
													  MainShaderUniforms[MAINSHADER_SPECDAMP],		   // Specular dampening
																									   // material component
													  MainShaderUniforms[MAINSHADER_DIFFUSIVITY],	  // Diffusivity.
																									   // Reaction to
																									   // diffuse light.
													  MainShaderUniforms[MAINSHADER_EMISSIVITY], MainShaderUniforms[MAINSHADER_ENABLE_CUBEMAP_REFLECTIONS],
													  MainShaderUniforms[MAINSHADER_ENABLE_CUBEMAP_DIFFUSION],
													  MainShaderUniforms[MAINSHADER_ENABLE_TRANSPARENCY], MainShaderUniforms[MAINSHADER_IS_INSTANCED],
													  MainShaderUniforms[MAINSHADER_BONE_UBO_LOCATION], BoneDataUBOBindPoint,
													  MainShaderUniforms[MAINSHADER_HAS_BONES],
													  MainShaderUniforms[MAINSHADER_TEXOFFSET],
													  MainShader,
													  true,											   // Transparent.
													  (CurrentRenderTarget != nullptr) ? true : false, // is it to render target?
													  meshmask, true, true, false);
					}
		}

		if (customRenderingAfterTransparentObjectRendering)
			customRenderingAfterTransparentObjectRendering(meshmask, CurrentRenderTarget, RenderTarget_Transparent, CurrentRenderCamera, doFrameBufferChecks,
														   backgroundColor, fogRangeDescriptor);

		glDisableVertexAttribArray(0); // Position
		glDisableVertexAttribArray(1); // Texture
		glDisableVertexAttribArray(2); // Normal
		glDisableVertexAttribArray(3); // Color

		//~ communism = glGetError();	 // Ensure there are no errors listed before we start.
		//~ if (communism != GL_NO_ERROR) // if communism has made an error (which
		//~ // is pretty typical)
		//~ {
		//~ std::cout << "\n OpenGL reports an ERROR! (POS 10)";
		//~ if (communism == GL_INVALID_ENUM)
		//~ std::cout << "\n Invalid enum.";
		//~ if (communism == GL_INVALID_OPERATION)
		//~ std::cout << "\n Invalid operation.";
		//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
		//~ std::cout << "\n Invalid Framebuffer Operation.";
		//~ if (communism == GL_OUT_OF_MEMORY) {
		//~ std::cout << "\n Out of memory. You've really done it now. I'm "
		//~ "so angry, i'm "
		//~ "going to close the program. ARE YOU HAPPY NOW, "
		//~ "DAVE?!?!";
		//~ std::abort();
		//~ }
		//~ }
	}

	if (CurrentRenderTarget == nullptr) // This part is just for displaying the FBO to the screen...
										// Not used for the FBO stuff.
	{
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		ShowTextureShader->bind();
		// INTEL GPU FIX
		if (doFrameBufferChecks)
			while (true) {
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
					break;
			}
		FBO::unBindRenderTarget(width, height);
		FBO::clearTexture(0, 0, 0, 0);
		FboArray[FORWARD_BUFFER1]->bindasTexture(0, 0);
		// TransparencyFBO->BindasTexture(0,0);
		screenquadtoFBO(ShowTextureShader);

		//~ communism = glGetError();	 // Ensure there are no errors listed before we start.
		//~ if (communism != GL_NO_ERROR) // if communism has made an error (which
		//~ // is pretty typical)
		//~ {
		//~ std::cout << "\n OpenGL reports an ERROR! (POS 11)";
		//~ if (communism == GL_INVALID_ENUM)
		//~ std::cout << "\n Invalid enum.";
		//~ if (communism == GL_INVALID_OPERATION)
		//~ std::cout << "\n Invalid operation.";
		//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
		//~ std::cout << "\n Invalid Framebuffer Operation.";
		//~ if (communism == GL_OUT_OF_MEMORY) {
		//~ std::cout << "\n Out of memory. You've really done it now. I'm "
		//~ "so angry, i'm "
		//~ "going to close the program. ARE YOU HAPPY NOW, "
		//~ "DAVE?!?!";
		//~ std::abort();
		//~ }
		//~ }
	}
	if (CurrentRenderTarget == nullptr) {
		nextRenderPipelineCallWillBeAfterVsync = true;
	} else {
		if (doFrameBufferChecks)
			while (true) {
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
					break;
			}
		nextRenderPipelineCallWillBeAfterVsync = false;
	}

	//~ communism = glGetError();	 // Ensure there are no errors listed before we
	//~ // start.
	//~ if (communism != GL_NO_ERROR) // if communism has made an error (which is pretty typical)
	//~ {
	//~ std::cout << "\n OpenGL reports an ERROR! (POS 12)";
	//~ if (communism == GL_INVALID_ENUM)
	//~ std::cout << "\n Invalid enum.";
	//~ if (communism == GL_INVALID_OPERATION)
	//~ std::cout << "\n Invalid operation.";
	//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
	//~ std::cout << "\n Invalid Framebuffer Operation.";
	//~ if (communism == GL_OUT_OF_MEMORY) {
	//~ std::cout << "\n Out of memory. You've really done it now. I'm so angry, "
	//~ "i'm "
	//~ "going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
	//~ std::abort();
	//~ }
	//~ }
} // eof drawPipeline

void GkScene::organizeUBOforUpload(Camera* SceneCam) {
	// Do all the stuff to sort the lights out and place them in their spots

	GLenum communism;
	float* temp_ptr;
	// std::cout << "\nSTARTED UBO SETUP" << std::endl;

	// Setting up the UBO
	/*
	 *
	unsigned char LightingDataUBOData[16000];//maximum of 16k in size to be
	compatible with old systems GLuint LightingDataUBO = 0; //handle for the
	lighting data UBO bool haveCreatedLightingUBO = false; //Have we created it?
	determines whether or not we destroy it and whether or not we have to make
	it again
	 *
	 * */
	if (!haveCreatedLightingUBO) {
		glGetError();
		for (int i = 0; i < 16000; i++) {
			LightingDataUBOData[i] = 0;
		}
		glGenBuffers(1, &LightingDataUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, LightingDataUBO); // Work with this UBO
		glBufferData(GL_UNIFORM_BUFFER, 8500, LightingDataUBOData,
					 GL_DYNAMIC_DRAW); // Use dynamic draw
		// from now on we will use glBufferSubData to update the VBO
		haveCreatedLightingUBO = true;
		glGetError();
	}
	if (!haveFoundLocationOfLightingUBO) {
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, LightingDataUBO);
		LightingDataUBOLocation = MainShader->getUniformBlockIndex("light_data");
		// MainShaderUniforms[MAINSHADER_BONE_UBO_LOCATION] =
		// MainShader->getUniformBlockIndex("bone_data"); ~ std::cout <<
		// "\nLIGHTING
		// DATA UBO LOCATION: " << LightingDataUBOLocation;
		MainShader->uniformBlockBinding(LightingDataUBOLocation, 0);
		haveFoundLocationOfLightingUBO = true;
	}

	//~ if (communism != GL_NO_ERROR) //if communism has made an error (which is
	// pretty typical) ~ { ~ std::cout<<"\n OpenGL reports an ERROR! (AFTER
	// LIGHTING UBO CREATION)"; ~ if (communism == GL_INVALID_ENUM) ~
	// std::cout<<"\n Invalid enum.";
	//~ if (communism == GL_INVALID_OPERATION)
	//~ std::cout<<"\n Invalid operation.";
	//~ if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
	//~ std::cout <<"\n Invalid Framebuffer Operation.";
	//~ if (communism == GL_OUT_OF_MEMORY)
	//~ {
	//~ std::cout <<"\n Out of memory. You've really done it now. I'm so angry,
	// i'm going to close the program. ARE YOU HAPPY NOW, DAVE?!?!"; ~
	// std::abort();
	//~ }
	//~ glGetError(); //clear it
	//~ }

	if (DirectionalLights.size() > 0) {
		int numDirLightsActive = 0;
		for (int i = 0; i < 2 && i < DirectionalLights.size(); i++)
			if (DirectionalLights[i] && DirectionalLights[i]->shouldRender) {
				// access by taking i, multiplying by 2, adding 4 * max point
				// lights, and adding an offset (0 for direction, 1 for
				// color...) m_LightUniformHandles
				// glUniform3f(m_LightUniformHandles[i * 2 + 4 * 32 + 0],
				// DirectionalLights[i]->myDirection.x,DirectionalLights[i]->myDirection.y,
				// DirectionalLights[i]->myDirection.z); //STARTS AT 0
				temp_ptr = (float*)(&(LightingDataUBOData[0 + i * 16])); // dirlight direction
																		 // (STARTS AT 0, VEC4S)
				memcpy(temp_ptr, &(DirectionalLights[i]->myDirection[0]),
					   12); // copy 3 floats of size 4 each

				// glUniform3f(m_LightUniformHandles[i * 2 + 4 * 32 + 1],
				// DirectionalLights[i]->myColor.x,DirectionalLights[i]->myColor.y,
				// DirectionalLights[i]->myColor.z);
				temp_ptr = (float*)(&(LightingDataUBOData[32 + i * 16])); // dirlight color
				memcpy(temp_ptr, &(DirectionalLights[i]->myColor[0]),
					   12); // copy 3 floats of size 4 each
				// Bind the Light Clipping Volumes

				// FORMAT OF m_LightClippingVolumeUniformHandles
				// Per Light:
				// 0 - 3 AABBp1-4 (4 vec3s)
				// 4-7 Spheres 0-3 (4 vec4s)
				// 8 whitelist or Whitelist UINT
				temp_ptr = (float*)(&(LightingDataUBOData[7872 + i * 16])); // dirlight whitelist toggle
				*((GLuint*)(&(LightingDataUBOData[7872 + i * 16]))) = DirectionalLights[i]->whitelist ? 0 : 1;

				// glUniform4f(m_LightClippingVolumeUniformHandles[32*9 + i*9 +
				// 4], DirectionalLights[i]->sphere1.x,
				// DirectionalLights[i]->sphere1.y,
				// DirectionalLights[i]->sphere1.z,
				// DirectionalLights[i]->sphere1.w);
				// //Sphere 1
				temp_ptr = (float*)(&(LightingDataUBOData[64 + i * 16])); // dirlight sphere1
				//~ memcpy(temp_ptr, &(DirectionalLights[i]->sphere1[0]),
				//~ 16); // copy 4 floats of size 4 each
				// glUniform4f(m_LightClippingVolumeUniformHandles[32*9 + i*9 +
				// 5], DirectionalLights[i]->sphere2.x,
				// DirectionalLights[i]->sphere2.y,
				// DirectionalLights[i]->sphere2.z,
				// DirectionalLights[i]->sphere2.w);
				// //Sphere 2
				temp_ptr = (float*)(&(LightingDataUBOData[32 * 3 + i * 16])); // dirlight sphere2
				//~ memcpy(temp_ptr, &(DirectionalLights[i]->sphere2[0]),
				//~ 16); // copy 4 floats of size 4 each
				// glUniform4f(m_LightClippingVolumeUniformHandles[32*9 + i*9 +
				// 6], DirectionalLights[i]->sphere3.x,
				// DirectionalLights[i]->sphere3.y,
				// DirectionalLights[i]->sphere3.z,
				// DirectionalLights[i]->sphere3.w);
				// //Sphere 3
				temp_ptr = (float*)(&(LightingDataUBOData[32 * 4 + i * 16])); // dirlight sphere3
				//~ memcpy(temp_ptr, &(DirectionalLights[i]->sphere3[0]),
				//~ 16); // copy 4 floats of size 4 each
				// glUniform4f(m_LightClippingVolumeUniformHandles[32*9 + i*9 +
				// 7], DirectionalLights[i]->sphere4.x,
				// DirectionalLights[i]->sphere4.y,
				// DirectionalLights[i]->sphere4.z,
				// DirectionalLights[i]->sphere4.w);
				// //Sphere 4
				temp_ptr = (float*)(&(LightingDataUBOData[32 * 5 + i * 16])); // dirlight sphere4
				//~ memcpy(temp_ptr, &(DirectionalLights[i]->sphere4[0]),
				//~ 16); // copy 4 floats of size 4 each

				// glUniform3f(m_LightClippingVolumeUniformHandles[32*9 + i*9 ],
				// DirectionalLights[i]->AABBp1.x,
				// DirectionalLights[i]->AABBp1.y,
				// DirectionalLights[i]->AABBp1.z); //AABBp1
				temp_ptr = (float*)(&(LightingDataUBOData[32 * 6 + i * 16])); // dirlight AABBp1
				//~ memcpy(temp_ptr, &(DirectionalLights[i]->AABBp1[0]),
				//~ 12); // copy 3 floats of size 4 each
				// glUniform3f(m_LightClippingVolumeUniformHandles[32*9 + i*9 +
				// 1], DirectionalLights[i]->AABBp2.x,
				// DirectionalLights[i]->AABBp2.y,
				// DirectionalLights[i]->AABBp2.z); //AABBp2
				temp_ptr = (float*)(&(LightingDataUBOData[32 * 7 + i * 16])); // dirlight AABBp2
				//~ memcpy(temp_ptr, &(DirectionalLights[i]->AABBp2[0]),
				//~ 12); // copy 3 floats of size 4 each
				// glUniform3f(m_LightClippingVolumeUniformHandles[32*9 + i*9 +
				// 2], DirectionalLights[i]->AABBp3.x,
				// DirectionalLights[i]->AABBp3.y,
				// DirectionalLights[i]->AABBp3.z); //AABBp3
				temp_ptr = (float*)(&(LightingDataUBOData[32 * 8 + i * 16])); // dirlight AABBp3
				//~ memcpy(temp_ptr, &(DirectionalLights[i]->AABBp3[0]),
				//~ 12); // copy 3 floats of size 4 each
				// glUniform3f(m_LightClippingVolumeUniformHandles[32*9 + i*9 +
				// 3], DirectionalLights[i]->AABBp4.x,
				// DirectionalLights[i]->AABBp4.y,
				// DirectionalLights[i]->AABBp4.z); //AABBp4
				temp_ptr = (float*)(&(LightingDataUBOData[32 * 9 + i * 16])); // dirlight AABBp4
				//~ memcpy(temp_ptr, &(DirectionalLights[i]->AABBp4[0]),
				//~ 12); // copy 3 floats of size 4 each

				numDirLightsActive++;
				// glUniform1ui(m_LightClippingVolumeUniformHandles[32*9 + i*9 +
				// 8], DirectionalLights[i]->whitelist?1:0); ~ if (i == 1 || i
				// ==
				// DirectionalLights.size()-1) ~ { WTF?!?!
				//~ }
			}
		// glUniform1i(MainShaderUniforms[MAINSHADER_NUM_DIRLIGHTS],
		// numDirLightsActive);
		*((int*)(&(LightingDataUBOData[8468]))) = numDirLightsActive; // num dir lights
	} else {
		// glUniform1i(MainShaderUniforms[MAINSHADER_NUM_DIRLIGHTS], 0);
		// Assign the value in the UBO data cpu-side
		*((int*)(&(LightingDataUBOData[8468]))) = 0; // num dir lights
	}

	// Do point lights
	if (SimplePointLights.size() > 0) {
		const int MAX_PL = 16;				 // If you set it higher than 32, you break the
											 // program. Don't break the program.
		PointLight* LightsThisFrame[MAX_PL]; // Array of pointers
		// Grab the first 32
		size_t howmanypointlightshavewedone = 0;
		long long maxindex = -1;
		for (size_t i = 0; i < SimplePointLights.size() && howmanypointlightshavewedone < MAX_PL; i++) {
			if (SimplePointLights[i]->shouldRender) {
				LightsThisFrame[howmanypointlightshavewedone] = SimplePointLights[i];
				howmanypointlightshavewedone++;
				maxindex = i;
			}
		}
		// maxindex++;
		// Compare all of the rest of them
		if (maxindex > -1 && maxindex < SimplePointLights.size()) {
			// Loop through all lights
			for (size_t i = 0; i < SimplePointLights.size(); i++)
				SimplePointLights[i]->dist2 = glm::length2(SimplePointLights[i]->myPos - SceneCam->pos);
			for (size_t i = maxindex + 1; i < SimplePointLights.size(); i++) // for all of them
			{
				if (i < SimplePointLights.size() && SimplePointLights[i]->shouldRender) // don't bother checking if it
																						// shouldn't be rendered.
				{
					// Will never be a nullptr
					PointLight* FarthestLight = LightsThisFrame[0];
					size_t indexofFarthestLight = 0;
					for (size_t j = 1; j < MAX_PL; j++) // We assume that 0 is the farthest by default, we
														// don't need to check it against itself
					{
						if (FarthestLight->dist2 < LightsThisFrame[j]->dist2) // FarthestLight is closer
						{
							FarthestLight = LightsThisFrame[j]; // Swap!
							// break; //I'm hoping this breaks the inner for
							// loop only. If it doesn't- set j to some massively
							// huge value instead
							indexofFarthestLight = j;
						}
					}
					// FarthestLight
					//~ if (glm::length2(SimplePointLights[i]->myPos -
					// SceneCam->pos) < glm::length2(FarthestLight->myPos -
					// SceneCam->pos)) //FarthestLight is farther away than the
					// light
					if (SimplePointLights[i]->dist2 < FarthestLight->dist2) // FarthestLight is farther away
																			// than the light
					{
						LightsThisFrame[indexofFarthestLight] = SimplePointLights[i];
					}
				}
			}
		}
		// Bind the lights

		// glUniform1i(MainShaderUniforms[MAINSHADER_NUM_POINTLIGHTS],
		// howmanypointlightshavewedone);
		*((int*)(&(LightingDataUBOData[8464]))) = howmanypointlightshavewedone;
		// if(maxindex > -1)
		for (size_t i = 0; i < MAX_PL && i < howmanypointlightshavewedone; i++) {

			// glUniform3f(m_LightUniformHandles[i*4],
			// LightsThisFrame[i]->pos.x, LightsThisFrame[i]->pos.y,
			// LightsThisFrame[i]->pos.z);//pos
			temp_ptr = (float*)(&(LightingDataUBOData[320 + i * 16])); // pointlight position
			memcpy(temp_ptr, &(LightsThisFrame[i]->myPos[0]),
				   12); // copy 3 floats of size 4 each
			// glUniform3f(m_LightUniformHandles[i*4+1],
			// LightsThisFrame[i]->myColor.x, LightsThisFrame[i]->myColor.y,
			// LightsThisFrame[i]->myColor.z);//color
			temp_ptr = (float*)(&(LightingDataUBOData[16 * 32 * 1 + 320 + i * 16])); // pointlight color
			memcpy(temp_ptr, &(LightsThisFrame[i]->myColor[0]),
				   12); // copy 3 floats of size 4 each
			// glUniform1f(m_LightUniformHandles[i*4+2],
			// LightsThisFrame[i]->range);//range
			temp_ptr = (float*)(&(LightingDataUBOData[6560 + i * 16])); // pointlight range
			memcpy(temp_ptr, &(LightsThisFrame[i]->range),
				   4); // copy 1 floats of size 4 each
			// glUniform1f(m_LightUniformHandles[i*4+3],
			// LightsThisFrame[i]->dropoff);//dropoff
			temp_ptr = (float*)(&(LightingDataUBOData[7072 + i * 16])); // pointlight dropoff
			memcpy(temp_ptr, &(LightsThisFrame[i]->dropoff),
				   4); // copy 1 floats of size 4 each
		}
	} else {
		// glUniform1i(MainShaderUniforms[MAINSHADER_NUM_POINTLIGHTS], 0);
		*((int*)(&(LightingDataUBOData[8464]))) = 0;
	}

	// Ambient Lights
	if (AmbientLights.size() > 0) {
		AmbientLight* LightsThisFrame[3]; // Array of pointers
		// Grab the first 3
		size_t howmanyAmbientlightshavewedone = 0;
		long long maxindex = -1;
		for (size_t i = 0; i < AmbientLights.size() && howmanyAmbientlightshavewedone < 3; i++) {
			if (AmbientLights[i]->shouldRender) {
				LightsThisFrame[howmanyAmbientlightshavewedone] = AmbientLights[i];
				howmanyAmbientlightshavewedone++;
				maxindex = i;
			}
		}
		maxindex++;
		// Compare all of the rest of them
		if (maxindex > -1 && maxindex != AmbientLights.size() - 1) {
			for (size_t i = maxindex; i < AmbientLights.size(); i++) // for all
																	 // of them
				AmbientLights[i]->dist2 = glm::length2(AmbientLights[i]->myPos - SceneCam->pos);
			for (size_t i = maxindex; i < AmbientLights.size(); i++) // for all of them
			{
				if (AmbientLights[i]->shouldRender) // don't bother checking if
													// it shouldn't be rendered.
				{
					AmbientLight* FarthestLight = LightsThisFrame[0];
					size_t indexofFarthestLight = 0;
					for (size_t j = 1; j < 3; j++) // We don't need to look at 0, because we assume
												   // it's the farthest by default
					{
						if ((FarthestLight->dist2) < (LightsThisFrame[j]->dist2)) // if this light is farther away
																				  // than the furthest light...
						{
							FarthestLight = LightsThisFrame[j]; // Then it is the farthest
																// light...
							indexofFarthestLight = j;
						}
					}
					if ((FarthestLight->dist2) > (AmbientLights[i]->dist2)) // This one is closer
						LightsThisFrame[indexofFarthestLight] = AmbientLights[i];
				}
			}
		}
		// Bind the lights
		// glUniform1i(MainShaderUniforms[MAINSHADER_NUM_AMBLIGHTS],
		// howmanyAmbientlightshavewedone);
		*((int*)(&(LightingDataUBOData[8472]))) = howmanyAmbientlightshavewedone;
		// if(maxindex > -1)
		for (size_t i = 0; i < 3 && i < howmanyAmbientlightshavewedone; i++) {
			// glUniform3f(m_LightUniformHandles[4 * 32 + 2 * 2 + i*3],
			// LightsThisFrame[i]->myPos.x, LightsThisFrame[i]->myPos.y,
			// LightsThisFrame[i]->myPos.z);//pos
			temp_ptr = (float*)(&(LightingDataUBOData[5440 + i * 16])); // Ambient light position
			memcpy(temp_ptr, &(LightsThisFrame[i]->myPos[0]),
				   12); // copy 3 floats of size 4 each
			// glUniform3f(m_LightUniformHandles[4 * 32 + 2 * 2 + i*3 +1],
			// LightsThisFrame[i]->myColor.x, LightsThisFrame[i]->myColor.y,
			// LightsThisFrame[i]->myColor.z);//color
			temp_ptr = (float*)(&(LightingDataUBOData[16 * 3 * 1 + 5440 + i * 16])); // Ambient light color
			memcpy(temp_ptr, &(LightsThisFrame[i]->myColor[0]),
				   12); // copy 3 floats of size 4 each

			// glUniform1f(m_LightUniformHandles[4 * 32 + 2 * 2 + i*3 +2],
			// LightsThisFrame[i]->myRange);//range
			temp_ptr = (float*)(&(LightingDataUBOData[7584 + i * 16])); // Ambient light color
			memcpy(temp_ptr, &(LightsThisFrame[i]->range), 4);
			temp_ptr = (float*)(&(LightingDataUBOData[8416 + i * 16])); // Ambient light whitelist var
			*((GLuint*)(temp_ptr)) = LightsThisFrame[i]->whitelist ? 1 : 0;
		}
		// std::cout << "\n DEBUGGING POINTLIGHT BINDINGS: MAXINDEX " <<
		// maxindex << " howmanypointlightshavewedone " <<
		// howmanypointlightshavewedone << "!" ;
	} else {
		// glUniform1i(MainShaderUniforms[MAINSHADER_NUM_AMBLIGHTS], 0);
		*((int*)(&(LightingDataUBOData[8472]))) = 0;
	}

	// Camera Lights
	if (CameraLights.size() > 0) {
		int camLightsRegistered = 0;
		size_t currindex = 0;
		// Replace with a system that sorts by distance later
		while (currindex < CameraLights.size() && camLightsRegistered < 5) {
			if (CameraLights[currindex] && CameraLights[currindex]->shouldRender) {
				const int i = camLightsRegistered; // BAM!
				glm::mat4 m_matrix;				   // done
				glm::vec3 m_camerapos;			   // done
				glm::vec3 m_color;				   // done
				GLfloat m_SolidColorToggle;		   // done
				GLfloat m_range;				   // done
				GLfloat m_shadows;				   // done
				glm::vec2 m_radii;				   // done
				glm::vec2 m_dim;
				CameraLights[currindex]->bindToUniformBufferCameraLight(&m_matrix, &m_camerapos, &m_color, &m_SolidColorToggle, &m_range, &m_shadows, &m_radii,
																		&m_dim, 3 + camLightsRegistered);
				// BINDING INFO

				// CAMERAPOS
				temp_ptr = (float*)(&(LightingDataUBOData[5920 + 16 * i])); // Cameralight Position (4 floats
																			// (padded), 4 bytes per float)
				memcpy(temp_ptr, &(m_camerapos[0]),
					   12); // copy 3 floats of size 4 each
				// COLOR
				temp_ptr = (float*)(&(LightingDataUBOData[16 * 5 * 1 + 5920 + 16 * i])); // Cameralight Color    (4
																						 // floats (padded), 4 bytes
																						 // per float)
				memcpy(temp_ptr, &(m_color[0]),
					   12); // copy 3 floats of size 4 each
				// RADII
				temp_ptr = (float*)(&(LightingDataUBOData[6400 + 16 * i])); // Cameralight RADII
																			// (4 floats (padded),
																			// 4 bytes per float)
				memcpy(temp_ptr, &(m_radii[0]),
					   8); // copy 2 floats of size 4 each
				// DIM
				temp_ptr = (float*)(&(LightingDataUBOData[6480 + 16 * i])); // Cameralight RADII
																			// (4 floats (padded),
																			// 4 bytes per float)
				memcpy(temp_ptr, &(m_dim[0]),
					   8); // copy 2 floats of size 4 each
				// SOLID COLOR TOGGLE
				temp_ptr = (float*)(&(LightingDataUBOData[7632 + 16 * i])); // Cameralight SOLID COLOR (4 floats
																			// (padded), 4 bytes per float)
				memcpy(temp_ptr, &(m_SolidColorToggle),
					   4); // copy 1 floats of size 4 each
				// RANGE
				temp_ptr = (float*)(&(LightingDataUBOData[7712 + 16 * i])); // Cameralight RANGE
																			// (4 floats (padded),
																			// 4 bytes per float)
				memcpy(temp_ptr, &(m_range), 4);							// copy 1 floats of size 4 each
				// SHADOWS
				temp_ptr = (float*)(&(LightingDataUBOData[7792 + 16 * i])); // Cameralight SHADOWS
																			// (4 floats (padded),
																			// 4 bytes per float)
				memcpy(temp_ptr, &(m_shadows),
					   4); // copy 1 floats of size 4 each
				// MATRIX
				temp_ptr = (float*)(&(LightingDataUBOData[16 * 5 * 2 + 5920 + 64 * i])); // Cameralight Matrix   (16
																						 // floats, 4 bytes per float)
				memcpy(temp_ptr, &(m_matrix[0][0]), 64);								 // copy 16 floats of
																						 // size 4 each
				camLightsRegistered++;
			}

			currindex++;
		}
		// glUniform1i(MainShaderUniforms[MAINSHADER_NUM_CAMLIGHTS],
		// camLightsRegistered);
		*((int*)(&(LightingDataUBOData[8476]))) = camLightsRegistered;
	} else {
		// glUniform1i(MainShaderUniforms[MAINSHADER_NUM_CAMLIGHTS], 0);
		*((int*)(&(LightingDataUBOData[8476]))) = 0;
	}

	// std::cout << "\nNO PROBLEMS WITH THE NEW UBO INFO" << std::endl;
	glGetError();

	// Update the buffer
	glBindBuffer(GL_UNIFORM_BUFFER, LightingDataUBO); // Work with this UBO
	glBufferSubData(GL_UNIFORM_BUFFER, 0, 8500,
					LightingDataUBOData); // Nothing more to do
	communism = glGetError();			  // Ensure there are no errors listed before we
										  // start.
	if (communism != GL_NO_ERROR)		  // communism is a mistake
	{
		std::cout << "\n OpenGL reports an ERROR! (AFTER EVERYTHING)";
		if (communism == GL_INVALID_ENUM)
			std::cout << "\n Invalid enum.";
		if (communism == GL_INVALID_OPERATION)
			std::cout << "\n Invalid operation.";
		if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
			std::cout << "\n Invalid Framebuffer Operation.";
		if (communism == GL_OUT_OF_MEMORY) {
			std::cout << "\n Out of memory. You've really done it now. I'm so angry, "
						 "i'm "
						 "going to close the program. ARE YOU HAPPY NOW, DAVE?!?!";
			std::abort();
		}
	}
}

// Constructor and Destructor
GkScene::GkScene(unsigned int newwidth, unsigned int newheight, float approxFactor) {
	// NOTE: we will be glEnabling and glDisabling alot in the draw function
	// when we get to stuff like transparency.
	glEnable(GL_CULL_FACE);				 // Enable culling faces
	glEnable(GL_DEPTH_TEST);			 // test fragment depth when rendering.
	glCullFace(GL_BACK);				 // cull faces with clockwise winding
	glEnable(GL_ARB_conservative_depth); // conservative depth.
	// glEnable(GL_EXT_conservative_depth); //Also needed?
	glGetError(); // the arb conservative depth thing (I think) creates an
				  // error, so i'm just voiding it with this...
	// Does the FBO stuff and of course sets the width and height
	resizeSceneViewport(newwidth, newheight,
						approxFactor); // setup the viewport and do the FBO dimensions

	if (m_screenquad_Mesh == nullptr) {
		// Set up the positions
		IndexedModel screenquad_IndexedModel;
		screenquad_IndexedModel.positions.push_back(glm::vec3(-1, -1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(-1, 1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(1, 1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(1, -1, 0));
		// Set up the index
		// First Triangle
		screenquad_IndexedModel.indices.push_back(0);
		screenquad_IndexedModel.indices.push_back(2);
		screenquad_IndexedModel.indices.push_back(3);
		// Second triangle
		screenquad_IndexedModel.indices.push_back(0);
		screenquad_IndexedModel.indices.push_back(1);
		screenquad_IndexedModel.indices.push_back(2);
		// Put in some bullshit for the texcoords and normals
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		// We also need bullshit for the normals
		screenquad_IndexedModel.normals.push_back(glm::vec3(1, -1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(2, 1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(3, 1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(4, -1, 0));

		// std::cout<<"\n We got the indexed model done";
		// Make the mesh
		m_screenquad_Mesh = new Mesh(screenquad_IndexedModel, false, true, true);
		// std::cout<<"\n we got the mesh made";
		// Make the camera needed to render the screenquad
		//~ if(ScreenquadCamera){
		//~ delete ScreenquadCamera;
		//~ ScreenquadCamera = nullptr;
		//~ }
		Camera ScreenquadCamera = Camera();
		ScreenquadCamera.buildOrthogonal(-1, 1, -1, 1, 0, 1);
		ScreenquadCamera.pos = glm::vec3(0, 0, 0);
		ScreenquadCamera.forward = glm::vec3(0, 0, 1);
		ScreenquadCamera.up = glm::vec3(0, 1, 0);
		Screenquad_CameraMatrix = ScreenquadCamera.getViewProjection();
	}

	if (m_skybox_Mesh == nullptr) // if this has not been instantiated
	{
		// std::cout << "\n Making the skybox mesh";
		const GLfloat canada[] = {// Naming convention???
								  // 0-5
								  -1, 1, -1, -1, -1, -1, 1, -1, -1,

								  1, -1, -1, 1, 1, -1, -1, 1, -1,
								  // 6-11
								  -1, -1, 1, -1, -1, -1, -1, 1, -1,

								  -1, 1, -1, -1, 1, 1, -1, -1, 1,
								  // 12-17
								  1, -1, -1, 1, -1, 1, 1, 1, 1,

								  1, 1, 1, 1, 1, -1, 1, -1, -1,
								  // 18-23
								  -1, -1, 1, -1, 1, 1, 1, 1, 1,

								  1, 1, 1, 1, -1, 1, -1, -1, 1,
								  // 24-29
								  -1, 1, -1, 1, 1, -1, 1, 1, 1,

								  1, 1, 1, -1, 1, 1, -1, 1, -1,
								  // 30-35
								  -1, -1, -1, -1, -1, 1, 1, -1, -1,

								  1, -1, -1, -1, -1, 1, 1, -1, 1}; // 36 vertices.
		for (unsigned int i = 0; i < 36; i++) {
			skyboxModel.positions.push_back(glm::vec3(canada[i * 3], canada[i * 3 + 1], canada[i * 3 + 2]));
			skyboxModel.indices.push_back(i);
			skyboxModel.texCoords.push_back(glm::vec2(0, 0));
			skyboxModel.colors.push_back(glm::vec3(0, 0, 0));
			skyboxModel.normals.push_back(glm::vec3(canada[i * 3], canada[i * 3 + 1], canada[i * 3 + 2]));
		}
		// std::cout << "\nPushed back all vert data.";
		skyboxModel.myFileName = "/aux/DEBUG_FILENAME_SKYBOX_MESH";
		m_skybox_Mesh = new Mesh(skyboxModel, false, true, true);
	}
}

GkScene::~GkScene() {

	for (auto* item : FboArray) // for each
		delete item;
	FboArray.clear();

	if (m_screenquad_Mesh)
		delete m_screenquad_Mesh; // Calls the destructor... or so I hear...
	if (m_skybox_Mesh)
		delete m_skybox_Mesh;
	if (haveCreatedLightingUBO)
		glDeleteBuffers(1, &LightingDataUBO);
}

}; // namespace gekRender
