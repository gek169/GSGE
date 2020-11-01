#include "GkEngine.h"

namespace GSGE {
using namespace gekRender;



#if defined(_WIN32)
#include <dir.h>
int MakeDir(const char* fn) { return (mkdir(fn) != 0); }
#else
#include <sys/stat.h>
#include <sys/types.h>
int MakeDir(const char* fn) { return (mkdir(fn, 0770) == -1); }
#endif

// GKOBJECT FILE FORMAT PARSER HELPER
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
btTransform g2b_transform(glm::mat4 trans) {
	btTransform result;
	result.setFromOpenGLMatrix((btScalar*)(&(trans[0][0])));
	return result;
}
glm::mat4 b2g_transform(btTransform input) {
	glm::mat4 result;
	float bruh[16];
	input.getOpenGLMatrix(bruh);
	result = glm::make_mat4(bruh);
	return result;
}

IndexedModel retrieveIndexedModelFromTriMesh(btTriangleMesh* trimesh) {
	IndexedModel retval;
	// We will assume it was created using true, true

	btIndexedMesh& mushy = trimesh->getIndexedMeshArray()[0]; // typedef btAlignedObjectArray< btIndexedMesh > 	IndexedMeshArray
	size_t numVerts = mushy.m_numVertices;
	size_t numTris = mushy.m_numTriangles;
	size_t numIndices = numTris * 3;
	PHY_ScalarType indexType = mushy.m_indexType;
	PHY_ScalarType vertexType = mushy.m_vertexType;
	if (indexType != PHY_INTEGER)
		return getErrorShape("Error, tri mesh was apparently made using Short indices");
	btVector3* vertArray = (btVector3*)mushy.m_vertexBase;
	int* indArray = (int*)mushy.m_triangleIndexBase;
	for (size_t i = 0; i < numIndices; i++) {
		retval.indices.push_back(retval.positions.size());
		glm::vec3 pos = b2g_vec3(vertArray[indArray[i]]);
		retval.positions.push_back(pos);
	}
	retval.validate(); // fills in texcoords and normals for exporting to OBJ
	return retval;
	// Vertex Stride is sizeof(btVector3)
	// Index stride is sizeof(int) * 3 (per triangle, of course)
}
btTriangleMesh* makeTriangleMesh(IndexedModel input_model) {
	// Makes a triangle mesh. This satisfies the Striding Mesh Interface
	// of btBvhTriangleMeshShape by the way.
	// Maximum of 2 million tris.
	btTriangleMesh* TriangleMesh = new btTriangleMesh(true, true); // 32 bit indices, 4 component.
	btVector3 p1;
	btVector3 p2;
	btVector3 p3;
	for (int i = 0; i < input_model.indices.size(); i++)
		switch (i % 3) { // master level C coding right here
		case 0:
			p1 = btVector3(input_model.positions[input_model.indices[i]].x, input_model.positions[input_model.indices[i]].y,
						   input_model.positions[input_model.indices[i]].z);
			break;
		case 1:
			p2 = btVector3(input_model.positions[input_model.indices[i]].x, input_model.positions[input_model.indices[i]].y,
						   input_model.positions[input_model.indices[i]].z);
			break;
		case 2:
			p3 = btVector3(input_model.positions[input_model.indices[i]].x, input_model.positions[input_model.indices[i]].y,
						   input_model.positions[input_model.indices[i]].z);
			TriangleMesh->addTriangle(p1, p2, p3);
			break;
		}
	// Quick test here of our function.
	//~ IndexedModel test = retrieveIndexedModelFromTriMesh(TriangleMesh);
	//~ std::cout << "\nTri mesh converted back to IndexedModel. Verts: " << test.positions.size() <<
	//~ " Indices: " << test.indices.size() <<
	//~ " Tris: " << test.indices.size()/3 << std::endl;
	return TriangleMesh;
}
btConvexHullShape* makeConvexHullShape(IndexedModel input_model) { // Makes a convex hull
	btConvexHullShape* returnval = new btConvexHullShape();
	for (int i = 0; i < input_model.positions.size(); i++)
		returnval->addPoint(btVector3(input_model.positions[i].x, input_model.positions[i].y, input_model.positions[i].z));
	returnval->optimizeConvexHull();
	return returnval;
}
//~ btConvexHullShape* getFrustumShape(Camera* cam){
	//~ btConvexHullShape* retval = nullptr;
	//~ if (!cam) return retval;
	//~ retval = new btConvexHullShape();
	//~ auto Frust = extractFrustum(cam->getViewProjection());
	//~ auto points = getFrustCorners(Frust);
	//~ for(auto& p:points)
		//~ retval->addPoint(g2b_vec3(glm::vec3(p)));
	//~ return retval;
//~ }

// https://pybullet.org/Bullet/BulletFull/btSoftBodyHelpers_8cpp_source.html on
// line 1019 We need to write a function to take an indexed model and convert it
// into a soft body, with params. I think it'll be really useful.

GkEngine::GkEngine() {
	WBOIT_ENABLED = true;
	myWorldType = BTDISCRETEWORLD;
	Resources.myEngine = (void*)this;
	init(640, 480, 1.0, false, false, "GSGE Program");
}
GkEngine::GkEngine(uint window_width, uint window_height, float scale, bool resizeable, bool fullscreen, const char* title, bool softbodies, bool _WBOIT,
				   uint samples) {
	WBOIT_ENABLED = _WBOIT;
	Resources.myEngine = (void*)this;
	init(window_width, window_height, scale, resizeable, fullscreen, title, softbodies);
}
void GkEngine::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	if (activeControlInterface)
		activeControlInterface->scroll_callback(window, xoffset, yoffset);
}
void GkEngine::window_size_callback(GLFWwindow* window, int width, int height) {
	//~ std::cout << "\n\nWindow resized!!!" << std::endl;
	if (height == 0 || width == 0)
		return;
	if (theScene)
		theScene->resizeSceneViewport(width, height, RENDERING_SCALE);
	//~ WIDTH = width;
	//~ HEIGHT = height;
	if (AutoRebuildCameraProjectionMatrix)
		MainCamera.buildPerspective(MainCamFOV, ((float)width) / ((float)height), MainCamMinClipPlane, MainCamMaxClipPlane);

	if (myFont)
		myFont->resize(width, height, UI_SCALE_FACTOR);
	for (auto*& sw : subWindows)
		if (sw)
			sw->setScreenSize(width, height, UI_SCALE_FACTOR);
}
void GkEngine::addSubWindow(GkUIWindow* b) {
	if (!b)
		return;
	for (auto*& i : subWindows)
		if (!i) {
			i = b;
			return;
		}
	subWindows.push_back(b);
}
void GkEngine::removeSubWindow(GkUIWindow* b) {
	for (auto*& i : subWindows)
		if (i == b) {
			i = nullptr;
		}
}

void GkEngine::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (activeControlInterface)
		activeControlInterface->key_callback(window, key, scancode, action, mods);
	return;
}

void GkEngine::char_callback(GLFWwindow* window, unsigned int codepoint) {
	if (activeControlInterface)
		activeControlInterface->char_callback(window, codepoint);
}
void GkEngine::joystickAxes_callback(int jid, const float* axes, int naxes){
	if(activeControlInterface)
		activeControlInterface->joystickAxes_callback(jid,axes,naxes);
}
void GkEngine::joystickButtons_callback(int jid, const unsigned char* buttons, int nbuttons){
	if(activeControlInterface)
		activeControlInterface->joystickButtons_callback(jid,buttons,nbuttons);
}
void GkEngine::init(uint window_width, uint window_height, float scale, bool resizeable, bool fullscreen, const char* title, bool softbodies, uint samples) {
	if (myDevice) // Has been constructed before
		return;
	myDevice = new IODevice();
	myDevice->initGLFW();
	myDevice->pushWindowCreationHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	myDevice->pushWindowCreationHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	if (samples > 0) {
		glfwWindowHint(GLFW_SAMPLES, samples);
		//~ std::cout << "\nEnabled MSAA" << std::endl;
	}
	if (resizeable) {
		myDevice->pushWindowCreationHint(GLFW_RESIZABLE, GLFW_TRUE);
	}
	if (fullscreen)
		myDevice->addFullScreenWindow(window_width, window_height, title);
	else
		myDevice->addWindow(window_width, window_height, title);
	myDevice->setContext(0); // Binds GL context of window 0 to current thread
	// myDevice->setWindowSizeCallback(0,this->window_size_callback);//Self
	// explanatory
	myDevice->initGL3W(); // Initialize the OpenGL extension wrangler.
	myDevice->fastInitOpenAL();
	glEnable(GL_CULL_FACE);  // Enable culling faces
	glEnable(GL_DEPTH_TEST); // test fragment depth when rendering
	glCullFace(GL_BACK);	 // cull faces with clockwise winding
	theScene = new GkScene(window_width, window_height, scale);
	RENDERING_SCALE = scale;
	UI_SCALE_FACTOR = scale;
	MainCamera = Camera(glm::vec3(0, 50, -10),						  // World Pos
						70.0f,										  // FOV
						((float)window_width / (float)window_height), // Aspect
						1.0f,										  // Znear
						1000.0f,									  // Zfar
						glm::vec3(0.0f, 0.0f, 1.0f),				  // forward
						glm::vec3(0.0f, 1.0f, 0.0f));				  // Up
	theScene->setSceneCamera(&MainCamera);
	// Attempt to load shaders
	if (WBOIT_ENABLED)
		MainShad = new Shader("shaders/FORWARD_MAINSHADER_UBO");
	else
		MainShad = new Shader("shaders/FORWARD_MAINSHADER_UBO_ADDITIVE");
	MainShaderShadows = new Shader("shaders/FORWARD_MAINSHADER_SHADOWS");
	DisplayTexture = new Shader("shaders/SHOWTEX");
	SkyboxShad = new Shader("shaders/Skybox");

	theScene->setSkyboxShader(SkyboxShad);
	theScene->setMainShader(MainShad);
	theScene->ShadowOpaqueMainShader = MainShaderShadows;
	theScene->ShowTextureShader = DisplayTexture;
	if (WBOIT_ENABLED) {
		WBOITCompShader = new Shader("shaders/WBOIT_COMPOSITION_SHADER");
		ParticleShader = new Shader("shaders/PARTICLE_WBOIT");
		theScene->setWBOITCompositionShader(WBOITCompShader);
	} else {
		ParticleShader = new Shader("shaders/PARTICLE_ADDITIVE");
	}
	Resources.theScene = theScene;
}

void GkEngine::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (activeControlInterface)
		activeControlInterface->mouse_button_callback(window, button, action, mods);
	return;
}
void GkEngine::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	updateCursorInfo(xpos, ypos);
	if (activeControlInterface)
		activeControlInterface->cursor_position_callback(window, xpos, ypos, ((double)xpos) / ((double)theScene->getWidth()),
														 ((double)ypos) / ((double)theScene->getHeight()), getCursorDelta(), getNormalizedCursorDelta());
	return;
}
// initialize everything related to your game.
// These functions don't have to be used
void GkEngine::init_game() {}
void GkEngine::tick_game() {}
void GkEngine::AutoExecutor(bool needContextSetting) {
	// If you don't switch between windows? You don't need context
	// setting.
	glfwSwapInterval(1);
	soundHandler.construct(64, 3); // Sounds and streams.
	init_game();
	
	while (!shouldQuit_AutoExecutor) {
		bullet_thread.Lock();
		UI_thread.Lock();
		ECS_Halts();
		tick_game(); // Where the magic happens.
		// Sync!
	
		resetContactInfo(); // Clears contact information in GkObjects from last time
		updateContactInfo(); // Updates contact information in GkObjects
		// Window 0 is managed by AutoExecutor.
		if (!paused)
			ECS_Midframes();
		if (!paused)
			PlayColSounds();
		Rendering_Sync(); 
		//~ performCulling();
		ECS_Splits();
		
		bullet_thread.Step();
		if (myFont)
			myFont->swapBuffers();
		UI_thread.Step();
		//Everything after this point is on the Rendering thread.
		//That just so happens to be the main thread.
		renderShadowMaps(3); // Renders shadow maps for all shadowed camera lights registered to the engine.
		theScene->drawPipeline(5, nullptr, nullptr, nullptr, false, glm::vec4(0, 0, 0.2, 0), glm::vec2(700, 900), true);
		if (myFont)
			myFont->pushChangestoTexture();
		if (myFont)
			myFont->draw(true);

		myDevice->swapBuffers(0);
		if (myDevice->shouldClose(0))
		shouldQuit_AutoExecutor = true;
		if (needContextSetting)
			myDevice->setContext(0);
		//~ myDevice->pollevents();
		//~ myDevice->swapBuffers(0);
		//~ shouldQuit_AutoExecutor = myDevice->shouldClose(0);
	}
	std::cout << "\nAutoExecutor Finished. Exiting..." << std::endl;
}
void GkEngine::renderShadowMaps(int meshmask) {
	for (auto*& cl : Cam_Lights)
		if (cl && cl->isShadowed && cl->myShadowMappingFBO && cl->shouldRender) {
			theScene->drawShadowPipeline(meshmask, (FBO*)cl->myShadowMappingFBO, &(cl->myCamera));
		}
}

void GkEngine::Load(std::string filename) {
	std::ifstream fakefile;
	fakefile.open(filename);
	if (!fakefile.is_open()) {
		std::cout << "\nLoad Failed, Can't open file" << std::endl;
		return;
	}
	//~ std::string((std::istreambuf_iterator<char>(fakefile)), (std::istreambuf_iterator<char>()));
	std::stringstream file(
		std::string(
			(std::istreambuf_iterator<char>(fakefile)), 
			(std::istreambuf_iterator<char>())
		)
	);
	fakefile.close();
	std::string line;
	uint lineCount = 0;
	uint linesSoFar = 0;
	{
		std::stringstream lfile(file.str());
		for (lineCount = 0; lfile.good(); lineCount++)
			std::getline(lfile, line);
		 //~ lfile.close();
	}
	//~ file.open(filename);

	std::vector<std::string> tokens;
	std::getline(file, line);
	std::string savedir = line;
	if(myVoxelWorld)
		myVoxelWorld->Load(savedir);
	double lastPercentage = 0.0;
	const double minPercentageDiff = 10.0;
	while (file.good()) {
		std::getline(file, line);
		linesSoFar++;
		
		double percent = 100.0 * (double)linesSoFar / (double)lineCount;
		if(percent - lastPercentage > minPercentageDiff)
		{
			displayLoadingScreen(percent);
			lastPercentage = percent;
		}

		if (line.length() < 4 || (line[0] == '#'))
			continue;
		tokens = SplitString(line, '|');
		if (GKMapLineInterpreter(savedir, filename, file, line, tokens)) {
			continue;
			/**
			 * GkMapFileText += "\nMAINCAM_PERSPECTIVE|";
	GkMapFileText += std::to_string(MainCamFOV) + "|";
	GkMapFileText += std::to_string(MainCamMinClipPlane) + "|";
	GkMapFileText += std::to_string(MainCamMaxClipPlane) + "\n";
			 * */
		} else if (tkCmd(tokens[0], "MAINCAM_PERSPECTIVE")) {
			if(tokens.size() < 4) return;
			MainCamFOV = GkAtof(tokens[1].c_str());
			MainCamMinClipPlane = GkAtof(tokens[2].c_str());
			MainCamMaxClipPlane = GkAtof(tokens[3].c_str());
			MainCamera.buildPerspective(MainCamFOV, ((float)theScene->getWidth()) / ((float)theScene->getHeight()), MainCamMinClipPlane, MainCamMaxClipPlane);
		} else if (tkCmd(tokens[0], "MAINCAMERA")) {
			if(tokens.size() < 10) return;
			glm::vec3 pos, forw, up;
			pos.x = GkAtof(tokens[1].c_str());
			pos.y = GkAtof(tokens[2].c_str());
			pos.z = GkAtof(tokens[3].c_str());
			forw.x = GkAtof(tokens[4].c_str());
			forw.y = GkAtof(tokens[5].c_str());
			forw.z = GkAtof(tokens[6].c_str());
			up.x = GkAtof(tokens[7].c_str());
			up.y = GkAtof(tokens[8].c_str());
			up.z = GkAtof(tokens[9].c_str());
			MainCamera.pos = pos;
			MainCamera.forward = forw;
			MainCamera.up = up;
		} else if (tkCmd(tokens[0], "SKYBOX")) {
			if (tokens.size() > 6)
				theScene->setSkyBoxCubemap(Resources.getCubeMap(tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6]));
		} else if (tkCmd(tokens[0], "POINTLIGHT")) {
			if (tokens.size() < 10)
				continue;
			glm::vec3 pos(0);
			glm::vec3 col(0);
			float range = 0;
			float dropoff = 0;
			int shouldRender = 0;
			pos.x = GkAtof(tokens[1].c_str());
			pos.y = GkAtof(tokens[2].c_str());
			pos.z = GkAtof(tokens[3].c_str());

			col.x = GkAtof(tokens[4].c_str());
			col.y = GkAtof(tokens[5].c_str());
			col.z = GkAtof(tokens[6].c_str());

			range = GkAtof(tokens[7].c_str());
			dropoff = GkAtof(tokens[8].c_str());

			shouldRender = GkAtoi(tokens[9].c_str());
			Point_Lights.push_back(new PointLight());
			auto& pl = *(Point_Lights.back());
			pl.myPos = pos;
			pl.myColor = col;
			pl.range = range;
			pl.dropoff = dropoff;
			pl.shouldRender = shouldRender;
			theScene->registerPointLight(Point_Lights.back());
		} else if (tkCmd(tokens[0], "AMBLIGHT")) {
			if (tokens.size() < 9)
				continue;
			glm::vec3 pos(0);
			glm::vec3 col(0);
			float range = 0;
			int shouldRender = 0;
			pos.x = GkAtof(tokens[1].c_str());
			pos.y = GkAtof(tokens[2].c_str());
			pos.z = GkAtof(tokens[3].c_str());

			col.x = GkAtof(tokens[4].c_str());
			col.y = GkAtof(tokens[5].c_str());
			col.z = GkAtof(tokens[6].c_str());

			range = GkAtof(tokens[7].c_str());

			shouldRender = GkAtoi(tokens[8].c_str());
			Amb_Lights.push_back(new AmbientLight());
			auto& pl = *(Amb_Lights.back());
			pl.myPos = pos;
			pl.myColor = col;
			pl.range = range;
			pl.shouldRender = shouldRender;
			theScene->registerAmbLight(Amb_Lights.back());
		} else if (tkCmd(tokens[0], "CAMLIGHT")) {
			if (tokens.size() < 38)
				continue;
			glm::vec3 pos(0);
			glm::vec3 forward(0);
			glm::vec3 up(0);
			float jafar;
			float janear;
			glm::mat4 proj(1.0);
			glm::vec3 col(0);
			glm::vec2 radii(0);
			glm::vec2 dimensions(0);
			float range;
			int isShadowed;
			int shouldRender;
			// then TEX or FBO or neither
			std::string texname = "";
			uint fbowidth = 0;
			uint fboheight = 0;
			pos.x = GkAtof(tokens[1].c_str());
			pos.y = GkAtof(tokens[2].c_str());
			pos.z = GkAtof(tokens[3].c_str());

			forward.x = GkAtof(tokens[4].c_str());
			forward.y = GkAtof(tokens[5].c_str());
			forward.z = GkAtof(tokens[6].c_str());

			up.x = GkAtof(tokens[7].c_str());
			up.y = GkAtof(tokens[8].c_str());
			up.z = GkAtof(tokens[9].c_str());

			jafar = GkAtof(tokens[10].c_str());
			janear = GkAtof(tokens[11].c_str());
			for (uint i = 0; i < 16; i++)
				proj[i / 4][i % 4] = GkAtof(tokens[12 + i].c_str());
			col.x = GkAtof(tokens[28].c_str());
			col.y = GkAtof(tokens[29].c_str());
			col.z = GkAtof(tokens[30].c_str());

			radii.x = GkAtof(tokens[31].c_str());
			radii.y = GkAtof(tokens[32].c_str());

			dimensions.x = GkAtof(tokens[33].c_str());
			dimensions.y = GkAtof(tokens[34].c_str());

			range = GkAtof(tokens[35].c_str());

			//~ isShadowed = GkAtoi(tokens[36].c_str());
			shouldRender = GkAtoi(tokens[37].c_str());

			if (tokens.size() > 38 && tokens[38] == "TEX") {
				texname = tokens[39];
				SafeTexture b(Resources.getTexture(texname, false, GL_LINEAR, GL_REPEAT, 0.0));
				if (!b.DaddyO)
					continue;
				Cam_Lights.push_back(new CameraLight());
				auto& cl = *(Cam_Lights.back());
				cl.solidColorToggle = 0.0f;
				// Done for all
				cl.isShadowed = false;
				cl.shouldRender = shouldRender;
				cl.range = range;
				cl.radii = radii;
				cl.Dimensions = dimensions;
				cl.myColor = col;
				cl.myCamera.pos = pos;
				cl.myCamera.forward = forward;
				cl.myCamera.up = up;
				cl.myCamera.jafar = jafar;
				cl.myCamera.janear = janear;
				cl.myCamera.projection = proj;
				cl.Tex2Project = b;
				theScene->registerCamLight(Cam_Lights.back());
			} else if (tokens.size() > 38 && tokens[38] == "FBO") {
				fbowidth = GkAtoui(tokens[39].c_str());
				fboheight = GkAtoui(tokens[40].c_str());
				if (fbowidth < 1 || fboheight < 1)
					continue;
				Cam_Light_Shadowmap_FBOs.push_back(new FBO(fbowidth, fboheight, 1, GL_RGBA32F));
				Cam_Lights.push_back(new CameraLight());
				auto& cl = *(Cam_Lights.back());
				cl.solidColorToggle = 1.0f;
				// Done for all
				cl.isShadowed = true;
				cl.shouldRender = shouldRender;
				cl.range = range;
				cl.radii = radii;
				cl.Dimensions = dimensions;
				cl.myColor = col;
				cl.myCamera.pos = pos;
				cl.myCamera.forward = forward;
				cl.myCamera.up = up;
				cl.myCamera.jafar = jafar;
				cl.myCamera.janear = janear;
				cl.myCamera.projection = proj;
				cl.myShadowMappingFBO = (void*)Cam_Light_Shadowmap_FBOs.back();
				cl.Tex2Project = Cam_Light_Shadowmap_FBOs.back()->getTex(0);
				theScene->registerCamLight(Cam_Lights.back());
			} else { // Build normal cameralight using its solid color and no shadowmaps.
				Cam_Lights.push_back(new CameraLight());
				auto& cl = *(Cam_Lights.back());
				cl.solidColorToggle = 1.0f;
				// Done for all
				cl.isShadowed = false;
				cl.shouldRender = shouldRender;
				cl.range = range;
				cl.radii = radii;
				cl.Dimensions = dimensions;
				cl.myColor = col;
				cl.myCamera.pos = pos;
				cl.myCamera.forward = forward;
				cl.myCamera.up = up;
				cl.myCamera.jafar = jafar;
				cl.myCamera.janear = janear;
				cl.myCamera.projection = proj;
				theScene->registerCamLight(Cam_Lights.back());
			}

		} else if (tkCmd(tokens[0], "PARTICLERENDERER")) {
			//~ std::cout << "\nParticleRenderer line, numtokens is " << tokens.size();
			if (tokens.size() < 31)
				continue;
			std::string texname;
			uint maxp = 1;
			glm::mat2x3 box;
			glm::mat2x3 vi_limits;
			glm::vec3 accel;
			glm::vec3 initcol;
			glm::vec3 fincol;
			float init_transp;
			float final_transp;
			float partSize = 1;
			float maxage;
			int coloristint;
			int autoadd;
			int forcenoimplicit;

			texname = tokens[1];
			maxp = GkAtoui(tokens[2].c_str());
			box[0].x = GkAtof(tokens[3].c_str());
			box[1].x = GkAtof(tokens[4].c_str());
			box[0].y = GkAtof(tokens[5].c_str());
			box[1].y = GkAtof(tokens[6].c_str());
			box[0].z = GkAtof(tokens[7].c_str());
			box[1].z = GkAtof(tokens[8].c_str());
			vi_limits[0].x = GkAtof(tokens[9].c_str());
			vi_limits[1].x = GkAtof(tokens[10].c_str());
			vi_limits[0].y = GkAtof(tokens[11].c_str());
			vi_limits[1].y = GkAtof(tokens[12].c_str());
			vi_limits[0].z = GkAtof(tokens[13].c_str());
			vi_limits[1].z = GkAtof(tokens[14].c_str());
			accel.x = GkAtof(tokens[15].c_str());
			accel.y = GkAtof(tokens[16].c_str());
			accel.z = GkAtof(tokens[17].c_str());
			initcol.x = GkAtof(tokens[18].c_str());
			initcol.y = GkAtof(tokens[19].c_str());
			initcol.z = GkAtof(tokens[20].c_str());
			fincol.x = GkAtof(tokens[21].c_str());
			fincol.y = GkAtof(tokens[22].c_str());
			fincol.z = GkAtof(tokens[23].c_str());
			init_transp = GkAtof(tokens[24].c_str());
			final_transp = GkAtof(tokens[25].c_str());
			partSize = GkAtof(tokens[26].c_str());
			maxage = GkAtof(tokens[27].c_str());
			coloristint = GkAtoi(tokens[28].c_str());
			autoadd = GkAtoi(tokens[29].c_str());
			forcenoimplicit = GkAtoi(tokens[30].c_str());
			//~ std::cout << "\nMAKING PARTICLE RENDERER!!!" << std::endl;
			ParticleRenderers.push_back(new ParticleRenderer());
			auto& pr = *(ParticleRenderers.back());
			pr.init(ParticleShader, SafeTexture(Resources.getTexture(texname, true, GL_NEAREST, GL_REPEAT, 0.0f)), maxp);
			pr.minX = box[0].x;
			pr.maxX = box[1].x;
			pr.minY = box[0].y;
			pr.maxY = box[1].y;
			pr.minZ = box[0].z;
			pr.maxZ = box[1].z;
			pr.minVX = vi_limits[0].x;
			pr.maxVX = vi_limits[1].x;
			pr.minVY = vi_limits[0].y;
			pr.maxVY = vi_limits[1].y;
			pr.minVZ = vi_limits[0].z;
			pr.maxVZ = vi_limits[1].z;
			pr.initColor = initcol;
			pr.finalColor = fincol;
			pr.initTransparency = init_transp;
			pr.finalTransparency = final_transp;
			pr.particleSize = partSize;
			pr.max_age = maxage;
			pr.colorIsTint = coloristint;
			pr.AutoAddParticles = autoadd;
			pr.forceNoImplicitSync = forcenoimplicit;
		} else if (tkCmd(tokens[0], "VFILE")) {
			if(tokens.size() < 2) continue;
			std::string vfilename = tokens[1];
			if(vfilename == "") continue;
			std::string filecontents = "";
			std::getline(file, line);
			while(!tkCmd(line, "ENDVFILE") && file.good()){
				filecontents += line + "\n";
				std::getline(file, line);
				linesSoFar++;
			}
			Resources.TextFiles[vfilename] = filecontents;
		} else if (tkCmd(tokens[0], "OBJECT")) {
			if (tokens.size() < 8)
				continue;
			std::string gsgefile = tokens[1];
			std::string name = tokens[2];
			int isTemplated;
			glm::vec3 createScale;
			size_t id;
			isTemplated = GkAtoi(tokens[3].c_str());
			createScale.x = GkAtof(tokens[4].c_str());
			createScale.y = GkAtof(tokens[5].c_str());
			createScale.z = GkAtof(tokens[6].c_str());
			id = GkAtoui(tokens[7].c_str());
			//~ if(isTemplated) std::cout << "\nMaking a templated object!" << std::endl;
			addObject(gsgefile, isTemplated, name, Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), createScale), id);
		} else if (tkCmd(tokens[0], "STARTECPROPERTIES")) {
			if (tokens.size() > 2) {
				// Retrieve id and index.
				size_t id = GkAtoui(tokens[1].c_str());
				size_t ind = GkAtoui(tokens[2].c_str());
				GkObject* obj = getObject(id);
				if (obj && obj->OwnedEntityComponents.size() > ind && obj->OwnedEntityComponents[ind]) {
					obj->OwnedEntityComponents[ind]->LoadGkMap(file, line, tokens, savedir, filename);
				}
			}
		} else if (tkCmd(tokens[0], "SETRIGIDBODYTRANSFORM")) {
			if (tokens.size() < 19)
				continue;
			size_t id;
			size_t ind;
			glm::mat4 m;
			id = GkAtoui(tokens[1].c_str());
			ind = GkAtoui(tokens[2].c_str());
			//~ std::cout << "\nID on transform line is " << id << std::endl;
			//~ std::cout << "\nIndex on transform line is " << ind << std::endl;
			for (uint i = 0; i < 16; i++)
				m[i / 4][i % 4] = GkAtof(tokens[3 + i].c_str());

			btTransform q = g2b_transform(m);
			GkObject* b = getObject(id);
			//~ if(!b) std::cout << "\nCan't get GkObject." << std::endl;
			if (b && b->RigidBodies.size() > ind && b->RigidBodies[ind]) {
				//~ const btVector3 zeroes(0,0,0);
				b->RigidBodies[ind]->setCenterOfMassTransform(q);
				b->RigidBodies[ind]->getMotionState()->setWorldTransform(q);
			}
		} else if (tkCmd(tokens[0], "SETRIGIDBODYVEL")) {
			if (tokens.size() < 9)
				continue;
			size_t id;
			size_t ind;
			btVector3 linv;
			btVector3 angv;
			id = GkAtoui(tokens[1].c_str());
			ind = GkAtoui(tokens[2].c_str());
			linv.setX(GkAtof(tokens[3].c_str()));
			linv.setY(GkAtof(tokens[4].c_str()));
			linv.setZ(GkAtof(tokens[5].c_str()));
			angv.setX(GkAtof(tokens[6].c_str()));
			angv.setY(GkAtof(tokens[7].c_str()));
			angv.setZ(GkAtof(tokens[8].c_str()));
			GkObject* b = getObject(id);
			//~ if(!b) std::cout << "\nCan't get GkObject." << std::endl;
			if (b && b->RigidBodies.size() > ind && b->RigidBodies[ind]) {
				b->RigidBodies[ind]->setLinearVelocity(linv);
				b->RigidBodies[ind]->setAngularVelocity(angv);
			}
			/*
			 if(RigidBodies[i]->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT){
				retval += "RIGIDBODYKINEMATIC|";
				retval += std::to_string((unsigned long long)ID);
				retval += "|";
				retval += std::to_string(i) + "\n";
			}
			 *
			 * */
		} else if (tkCmd(tokens[0], "RIGIDBODYKINEMATIC")) {
			if (tokens.size() < 3)
				continue;
			auto id = GkAtoui(tokens[1].c_str());
			auto ind = GkAtoui(tokens[2].c_str());
			GkObject* b = getObject(id);
			if (b && b->RigidBodies.size() > ind && b->RigidBodies[ind] && b->RigidBodies[ind]->getBroadphaseHandle()) {
				int group = b->RigidBodies[ind]->getBroadphaseHandle()->m_collisionFilterGroup;
				int mask = b->RigidBodies[ind]->getBroadphaseHandle()->m_collisionFilterMask;
				world->removeRigidBody(b->RigidBodies[ind]);
				b->RigidBodies[ind]->setCollisionFlags(b->RigidBodies[ind]->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				world->addRigidBody(b->RigidBodies[ind], group, mask);
			}
		} else if (tkCmd(tokens[0], "SETRIGIDBODYMASS")) {
			if (tokens.size() < 7)
				continue;
			size_t id;
			size_t ind;
			float mass;
			btVector3 inertia;
			id = GkAtoui(tokens[1].c_str());
			ind = GkAtoui(tokens[2].c_str());
			//~ std::cout << "\nID on mass line is " << id << std::endl;
			//~ std::cout << "\nIndex on mass line is " << ind << std::endl;
			mass = GkAtof(tokens[3].c_str());
			inertia.setX(GkAtof(tokens[4].c_str()));
			inertia.setY(GkAtof(tokens[5].c_str()));
			inertia.setZ(GkAtof(tokens[6].c_str()));
			GkObject* b = getObject(id);
			//~ if(!b) std::cout << "\nCan't get GkObject." << std::endl;
			if (b && b->RigidBodies.size() > ind && b->RigidBodies[ind] && b->RigidBodies[ind]->getBroadphaseHandle()) {
				//~ std::cout << "\nSETTING RIGID BODY MASS!" << std::endl;

				int group = b->RigidBodies[ind]->getBroadphaseHandle()->m_collisionFilterGroup;
				int mask = b->RigidBodies[ind]->getBroadphaseHandle()->m_collisionFilterMask;
				world->removeRigidBody(b->RigidBodies[ind]);
				b->RigidBodies[ind]->setMassProps(mass, inertia);
				world->addRigidBody(b->RigidBodies[ind], group, mask);
			}
		} else if (tkCmd(tokens[0], "SETRIGIDBODYFRICTION")) {
			if (tokens.size() < 4)
				continue;
			size_t id;
			size_t ind;
			float frict;
			id = GkAtoui(tokens[1].c_str());
			ind = GkAtoui(tokens[2].c_str());
			frict = GkAtof(tokens[3].c_str());
			GkObject* b = getObject(id);
			//~ if(!b) std::cout << "\nCan't get GkObject." << std::endl;
			if (b && b->RigidBodies.size() > ind && b->RigidBodies[ind])
				b->RigidBodies[ind]->setFriction(frict);
		} else if (tkCmd(tokens[0], "SETRIGIDBODYRESTITUTION")) {
			if (tokens.size() < 4)
				continue;
			size_t id;
			size_t ind;
			float rest;
			id = GkAtoui(tokens[1].c_str());
			ind = GkAtoui(tokens[2].c_str());
			rest = GkAtof(tokens[3].c_str());
			GkObject* b = getObject(id);
			if (b && b->RigidBodies.size() > ind && b->RigidBodies[ind])
				b->RigidBodies[ind]->setRestitution(rest);

		} else if (tkCmd(tokens[0], "CREATEWELD")) {
			std::vector<size_t> ids;
			// TODO: write a CreateWeld function that takes in a vector of GkObject*,
			// removes them from the discretedynamicsworld,
			// and then makes a GkObject that syncs the subobjects
			// to the child shapes of the Welded object.
		} else if (tkCmd(tokens[0], "FIXEDCONSTRAINT")) {
			if (tokens.size() < 39)
				continue;
			size_t idA;
			size_t indA;
			size_t idB;
			size_t indB;
			glm::mat4 tia;
			glm::mat4 tib;
			float threshhold;
			int ncollide;
			btTransform btia, btib;
			idA = GkAtoui(tokens[1].c_str());
			indA = GkAtoui(tokens[2].c_str());
			idB = GkAtoui(tokens[3].c_str());
			indB = GkAtoui(tokens[4].c_str());
			for (uint i = 0; i < 16; i++)
				tia[i / 4][i % 4] = GkAtof(tokens[5 + i].c_str());
			for (uint i = 0; i < 16; i++)
				tib[i / 4][i % 4] = GkAtof(tokens[21 + i].c_str());
			threshhold = GkAtof(tokens[37].c_str());
			ncollide = GkAtoi(tokens[38].c_str());
			btia = g2b_transform(tia);
			btib = g2b_transform(tib);
			GkObject* AO = getObject(idA);
			GkObject* BO = getObject(idB);

			if (!AO || !BO || AO->RigidBodies.size() <= indA || !AO->RigidBodies[indA] || BO->RigidBodies.size() <= indB || !BO->RigidBodies[indB])
				continue;
			btRigidBody* RgdA = AO->RigidBodies[indA];
			btRigidBody* RgdB = BO->RigidBodies[indB];
			Constraints.push_back(new btFixedConstraint(*RgdA, *RgdB, btia, btib));
			Constraints.back()->setBreakingImpulseThreshold(threshhold);
			world->addConstraint(Constraints.back(), (bool)ncollide);
		} else if (tkCmd(tokens[0], "POINT2POINTCONSTRAINT")) {
			if (tokens.size() < 13)
				continue;
			size_t idA;
			size_t indA;
			size_t idB;
			size_t indB;
			btVector3 pia;
			btVector3 pib;
			float threshhold;
			int ncollide;
			idA = GkAtoui(tokens[1].c_str());
			indA = GkAtoui(tokens[2].c_str());
			idB = GkAtoui(tokens[3].c_str());
			indB = GkAtoui(tokens[4].c_str());

			pia.setX(GkAtof(tokens[5].c_str()));
			pia.setY(GkAtof(tokens[6].c_str()));
			pia.setZ(GkAtof(tokens[7].c_str()));

			pib.setX(GkAtof(tokens[8].c_str()));
			pib.setY(GkAtof(tokens[9].c_str()));
			pib.setZ(GkAtof(tokens[10].c_str()));
			threshhold = GkAtof(tokens[11].c_str());
			ncollide = GkAtoi(tokens[12].c_str());
			GkObject* AO = getObject(idA);
			GkObject* BO = getObject(idB);

			if (!AO || !BO || AO->RigidBodies.size() <= indA || !AO->RigidBodies[indA] || BO->RigidBodies.size() <= indB || !BO->RigidBodies[indB])
				continue;
			btRigidBody* RgdA = AO->RigidBodies[indA];
			btRigidBody* RgdB = BO->RigidBodies[indB];
			Constraints.push_back(new btPoint2PointConstraint(*RgdA, *RgdB, pia, pib));
			world->addConstraint(Constraints.back(), (bool)ncollide);
		}
	} // Eof while loop over lines in a file
}
void GkEngine::Save(std::string savedir, std::string filename) {
	guaranteeUniqueIDs();
	std::string GkMapFileText = "";
	GkMapFileText += savedir + "\n";
	int retval_makedir = MakeDir(savedir.c_str());
	std::ofstream ofile;
	ofile.open(filename, std::ios::trunc);
	if (!ofile.is_open()) {
		std::cout << "\nSave Failed, Can't open output file" << std::endl;
		return;
	} // failed to open outfile
	
	// Serialize the global skybox
	CubeMap* skybox = theScene->getSkyBoxCubemap();
	if (myVoxelWorld)
		myVoxelWorld->Save(savedir);
	if (skybox) {
		GkMapFileText += "SKYBOX|" + skybox->MyName + "\n";
	}
	// Serialize Camera
	GkMapFileText += "\nMAINCAMERA|";
	GkMapFileText += std::to_string(MainCamera.pos.x) + "|";
	GkMapFileText += std::to_string(MainCamera.pos.y) + "|";
	GkMapFileText += std::to_string(MainCamera.pos.z) + "|";

	GkMapFileText += std::to_string(MainCamera.forward.x) + "|";
	GkMapFileText += std::to_string(MainCamera.forward.y) + "|";
	GkMapFileText += std::to_string(MainCamera.forward.z) + "|";

	GkMapFileText += std::to_string(MainCamera.up.x) + "|";
	GkMapFileText += std::to_string(MainCamera.up.y) + "|";
	GkMapFileText += std::to_string(MainCamera.up.z) + "\n";
	GkMapFileText += "\nMAINCAM_PERSPECTIVE|";
	GkMapFileText += std::to_string(MainCamFOV) + "|";
	GkMapFileText += std::to_string(MainCamMinClipPlane) + "|";
	GkMapFileText += std::to_string(MainCamMaxClipPlane) + "\n";
	// Serialize Point Lights
	for (auto*& pl : Point_Lights)
		if (pl) {
			GkMapFileText += "\nPOINTLIGHT|" + std::to_string(pl->myPos.x) + "|" + std::to_string(pl->myPos.y) + "|" + std::to_string(pl->myPos.z) + "|" +
							 std::to_string(pl->myColor.x) + "|" + std::to_string(pl->myColor.y) + "|" + std::to_string(pl->myColor.z) + "|" +
							 std::to_string(pl->range) + "|" + std::to_string(pl->dropoff) + "|" + std::to_string((int)(pl->shouldRender)) + "\n";
		}
	// Serialize AmbientLights
	for (auto*& al : Amb_Lights)
		if (al) {
			GkMapFileText += "\nAMBLIGHT|" + std::to_string(al->myPos.x) + "|" + std::to_string(al->myPos.y) + "|" + std::to_string(al->myPos.z) + "|" +
							 std::to_string(al->myColor.x) + "|" + std::to_string(al->myColor.y) + "|" + std::to_string(al->myColor.z) + "|" +
							 std::to_string(al->range) + "|" + std::to_string((int)(al->shouldRender));
		}
	// Serialize CameraLights
	for (auto*& cl : Cam_Lights)
		if (cl) {
			FBO* myFBO = (FBO*)cl->myShadowMappingFBO;
			Texture* myProjText = cl->Tex2Project.DaddyO;
			//~ if(myFBO && cl->isShadowed){ //This is a shadowmapped Cameralight.
			//~ GkMapFileText += "\nSHADOWEDCAMLIGHT|";//Build shadowed camera light line.
			//~ } else if (myProjText) {
			//~ GkMapFileText += "\nTEXTUREDCAMLIGHT|";//Build textured camera light line.
			//~ } else {

			//~ }
			GkMapFileText += "\nCAMLIGHT|"; // Build untextured, solid-color camera light line
			// Provide the Camera details before anything
			GkMapFileText += std::to_string(cl->myCamera.pos.x) + "|";
			GkMapFileText += std::to_string(cl->myCamera.pos.y) + "|";
			GkMapFileText += std::to_string(cl->myCamera.pos.z) + "|";

			GkMapFileText += std::to_string(cl->myCamera.forward.x) + "|";
			GkMapFileText += std::to_string(cl->myCamera.forward.y) + "|";
			GkMapFileText += std::to_string(cl->myCamera.forward.z) + "|";

			GkMapFileText += std::to_string(cl->myCamera.up.x) + "|";
			GkMapFileText += std::to_string(cl->myCamera.up.y) + "|";
			GkMapFileText += std::to_string(cl->myCamera.up.z) + "|";

			GkMapFileText += std::to_string(cl->myCamera.jafar) + "|";
			GkMapFileText += std::to_string(cl->myCamera.janear) + "|";
			// Export projection matrix
			for (uint i = 0; i < 16; i++)
				GkMapFileText += std::to_string(cl->myCamera.projection[i / 4][i % 4]) + "|";
			// Export Light's own settings.
			GkMapFileText += std::to_string(cl->myColor.x) + "|";
			GkMapFileText += std::to_string(cl->myColor.y) + "|";
			GkMapFileText += std::to_string(cl->myColor.z) + "|";

			GkMapFileText += std::to_string(cl->radii.x) + "|";
			GkMapFileText += std::to_string(cl->radii.y) + "|";
			GkMapFileText += std::to_string(cl->Dimensions.x) + "|";
			GkMapFileText += std::to_string(cl->Dimensions.y) + "|";

			GkMapFileText += std::to_string(cl->range) + "|";

			GkMapFileText += std::to_string((int)(cl->isShadowed)) + "|";
			GkMapFileText += std::to_string((int)(cl->shouldRender)) + "|";

			if (myProjText) {
				GkMapFileText += "TEX|";
				GkMapFileText += myProjText->MyName + "\n";
			} else if (myFBO && cl->isShadowed) {
				GkMapFileText += "FBO|";
				GkMapFileText += std::to_string(myFBO->getWidth()) + "|";
				GkMapFileText += std::to_string(myFBO->getHeight()) + "\n";
				// We will assume that the internal type is GL_RGBA32F
			} else {
				GkMapFileText += "\n";
			}
		}
	// Serialize ParticleRenderers
	for (auto*& item : ParticleRenderers)
		if (item) {
			if (!item->myTexture.DaddyO)
				continue; // Doesn't use a loaded texture. Can't serialize.
			GkMapFileText += "\nPARTICLERENDERER|";

			GkMapFileText += item->myTexture.DaddyO->MyName + "|";
			GkMapFileText += std::to_string(item->particleData.size() / 7) + "|";

			GkMapFileText += std::to_string(item->minX) + "|";
			GkMapFileText += std::to_string(item->maxX) + "|";
			GkMapFileText += std::to_string(item->minY) + "|";
			GkMapFileText += std::to_string(item->maxY) + "|";
			GkMapFileText += std::to_string(item->minZ) + "|";
			GkMapFileText += std::to_string(item->maxZ) + "|";

			GkMapFileText += std::to_string(item->minVX) + "|";
			GkMapFileText += std::to_string(item->maxVX) + "|";
			GkMapFileText += std::to_string(item->minVY) + "|";
			GkMapFileText += std::to_string(item->maxVY) + "|";
			GkMapFileText += std::to_string(item->minVZ) + "|";
			GkMapFileText += std::to_string(item->maxVZ) + "|";

			GkMapFileText += std::to_string(item->Acceleration.x) + "|";
			GkMapFileText += std::to_string(item->Acceleration.y) + "|";
			GkMapFileText += std::to_string(item->Acceleration.z) + "|";

			GkMapFileText += std::to_string(item->initColor.x) + "|";
			GkMapFileText += std::to_string(item->initColor.y) + "|";
			GkMapFileText += std::to_string(item->initColor.z) + "|";
			GkMapFileText += std::to_string(item->finalColor.x) + "|";
			GkMapFileText += std::to_string(item->finalColor.y) + "|";
			GkMapFileText += std::to_string(item->finalColor.z) + "|";

			GkMapFileText += std::to_string(item->initTransparency) + "|";
			GkMapFileText += std::to_string(item->finalTransparency) + "|";

			GkMapFileText += std::to_string(item->particleSize) + "|";
			GkMapFileText += std::to_string(item->max_age) + "|";
			GkMapFileText += std::to_string((int)(item->colorIsTint)) + "|";
			GkMapFileText += std::to_string((int)(item->AutoAddParticles)) + "|";
			GkMapFileText += std::to_string((int)(item->forceNoImplicitSync)) + "|";
			GkMapFileText += "\n";
		}
	// Write out Vfiles for templates.
	for (auto it = Resources.TemplateObjects.begin(); it != Resources.TemplateObjects.end(); it++){
		if(!it->second) continue;
		GkObject* tmplt = it->second;
		if(tmplt->requiresExporting){
			GkMapFileText += "\nVFILE|" + tmplt->GkObjectFile + "\n" + tmplt->ExportString + "ENDVFILE\n";
			//Export a virtual file. Hooks into Resources, effectively creating a placeholder GSGE file.
			//~ retval += "\nVFILE|" + GkObjectFile;
			//~ retval += "\n" + ExportString + "ENDVFILE\n";
		}
	}
	// Serialize GkObjects
	for (auto*& item : Objects)
		if (item && !(item->donotsave)) {
			if (!item->requiresExporting) {
				GkMapFileText += "\n" + item->Serialize(savedir) + "\n";
			} else {
				GkMapFileText += "\n" + GkObjectExporterHook(item, savedir) + "\n";
				GkMapFileText += "\n" + item->Serialize(savedir) + "\n";
			}
		}
	// Serialize all Constraints
	for (btTypedConstraint*& item : Constraints)
		if (item) {
			if (item->getConstraintType() == FIXED_CONSTRAINT_TYPE) {
				glm::mat4 m;
				btFixedConstraint* constraint = (btFixedConstraint*)item;
				btTransform tia = constraint->getFrameOffsetA();
				btTransform tib = constraint->getFrameOffsetB();
				float threshhold = constraint->getBreakingImpulseThreshold();
				btRigidBody* objA = &(constraint->getRigidBodyA());
				btRigidBody* objB = &(constraint->getRigidBodyB());
				if (!objA->getUserPointer() || !objB->getUserPointer())
					continue;
				GkObject* gkA = (GkObject*)((GkCollisionInfo*)objA->getUserPointer())->myGkObject;
				GkObject* gkB = (GkObject*)((GkCollisionInfo*)objB->getUserPointer())->myGkObject;
				if (!gkA || !gkB)
					continue;
				size_t idA = gkA->getID();
				size_t idB = gkB->getID();
				size_t indA = 0;
				size_t indB = 0;
				for (size_t i = 0; i < gkA->RigidBodies.size(); i++)
					if (gkA->RigidBodies[i] == objA)
						indA = i;
				for (size_t i = 0; i < gkB->RigidBodies.size(); i++)
					if (gkB->RigidBodies[i] == objB)
						indB = i;
				GkMapFileText += "\nFIXEDCONSTRAINT|";
				GkMapFileText += std::to_string(idA) + "|";
				GkMapFileText += std::to_string(indA) + "|";
				GkMapFileText += std::to_string(idB) + "|";
				GkMapFileText += std::to_string(indB) + "|";
				m = (b2g_transform(tia));
				for (uint i = 0; i < 16; i++)
					GkMapFileText += std::to_string(m[i / 4][i % 4]) + "|";
				m = (b2g_transform(tib));
				for (uint i = 0; i < 16; i++)
					GkMapFileText += std::to_string(m[i / 4][i % 4]) + "|";
				GkMapFileText += std::to_string(threshhold) + "|";
				// Determine if we have nocollide
				bool ncollide = false;
				for (int i = 0; i < objA->getNumConstraintRefs(); i++)
					if (objA->getConstraintRef(i) == item)
						ncollide = true;
				GkMapFileText += std::to_string((int)(ncollide));
				GkMapFileText += "\n";
			} else if (item->getConstraintType() == POINT2POINT_CONSTRAINT_TYPE) {
				btPoint2PointConstraint* constraint = (btPoint2PointConstraint*)item;
				float threshhold = constraint->getBreakingImpulseThreshold();
				btVector3 pia = constraint->getPivotInA();
				btVector3 pib = constraint->getPivotInB();
				btRigidBody* objA = &(constraint->getRigidBodyA());
				btRigidBody* objB = &(constraint->getRigidBodyB());
				if (!objA->getUserPointer() || !objB->getUserPointer())
					continue;
				GkObject* gkA = (GkObject*)((GkCollisionInfo*)objA->getUserPointer())->myGkObject;
				GkObject* gkB = (GkObject*)((GkCollisionInfo*)objB->getUserPointer())->myGkObject;
				if (!gkA || !gkB)
					continue;
				size_t idA = gkA->getID();
				size_t idB = gkB->getID();
				size_t indA = 0;
				size_t indB = 0;
				for (size_t i = 0; i < gkA->RigidBodies.size(); i++)
					if (gkA->RigidBodies[i] == objA)
						indA = i;
				for (size_t i = 0; i < gkB->RigidBodies.size(); i++)
					if (gkB->RigidBodies[i] == objB)
						indB = i;
				GkMapFileText += "\nPOINT2POINTCONSTRAINT|";
				GkMapFileText += std::to_string(idA) + "|";
				GkMapFileText += std::to_string(indA) + "|";
				GkMapFileText += std::to_string(idB) + "|";
				GkMapFileText += std::to_string(indB) + "|";

				GkMapFileText += std::to_string(pia.x()) + "|";
				GkMapFileText += std::to_string(pia.y()) + "|";
				GkMapFileText += std::to_string(pia.z()) + "|";

				GkMapFileText += std::to_string(pib.x()) + "|";
				GkMapFileText += std::to_string(pib.y()) + "|";
				GkMapFileText += std::to_string(pib.z()) + "|";
				GkMapFileText += std::to_string(threshhold) + "|";
				// Determine if we have nocollide
				bool ncollide = false;
				for (int i = 0; i < objA->getNumConstraintRefs(); i++)
					if (objA->getConstraintRef(i) == item)
						ncollide = true;
				GkMapFileText += std::to_string((int)(ncollide));
				GkMapFileText += "\n";
			}
		}
	GkMapFileText += GkMapSaveHook(savedir, filename, ofile);
	ofile << GkMapFileText;
	ofile.close();
}
int GkEngine::getKey(int key) { return myDevice->getKey(0, key); }						 // https://www.glfw.org/docs/latest/group__keys.html
int GkEngine::getMouseButton(int button) { return myDevice->getMouseButton(0, button); } // https://www.glfw.org/docs/latest/group__buttons.html
// world->stepSimulation(t) passthrough
void GkEngine::tickPhysics(double t) {
	if (world)
		world->stepSimulation(t);
}
void GkEngine::registerObject(GkObject* obj, std::string name, size_t id){
	name_compat(name);
	// Register the object to the scene here.
	GkObject* retval = obj;
	if (retval) {
		retval->donotsave = true; //Do not attempt to save this object.
		retval->name = name;
		if (id == std::numeric_limits<size_t>::max()) {
			while (getObject(id_ticker))
				id_ticker++;
			retval->setID(id_ticker++);
		} else if (id < std::numeric_limits<size_t>::max()) {
			while (getObject(id)) // Ensure the id is unique.
				id++;
			retval->setID(id);
		}
		bool foundNullPtr = false;
		for (size_t i = 0; i < Objects.size(); i++)
			if ((Objects[i]) == nullptr) {
				foundNullPtr = true;
				//~ std::cout << "\nFOUND NULLPTR TO PLACE OBJECT AT!!!" <<
				// std::endl;
				Objects[i] = retval;
				break;
			}
		if (!foundNullPtr) {
			Objects.push_back(retval);
			//~ std::cout << "\nPushing back." << std::endl;
		}
		retval->Register(theScene, world, myWorldType);
	}
}
void GkEngine::deregisterObject(GkObject* obj){
	if (!obj || Objects.size() < 1)
		return;
	//~ std::cout << "\nENTERED removeObject!" << std::endl;
	if (Constraints.size())
		for (uint constraint = Constraints.size() - 1; constraint >= 0; constraint--)
			if (Constraints[constraint])
				for (auto* body : obj->RigidBodies) // Loop through all constraints and rigid
													// bodies
				{
					//~ std::cout << "\nENTERED the constraint loop!" <<
					// std::endl;
					if (&(Constraints[constraint]->getRigidBodyA()) == body || &(Constraints[constraint]->getRigidBodyB()) == body) // If this constraint has
																																	// this rigid body in it...
					{

						world->removeConstraint(Constraints[constraint]); // Remove from our world!
						delete (Constraints[constraint]);				  // Delete from our
																		  // universe!
						Constraints[constraint] = nullptr;
					}
				}
	for (size_t i = Objects.size() - 1; i < Objects.size() && i >= 0; i--)
		if (Objects[i] == obj) {
			//~ std::cout << "\nFound our man. He's: " << Objects[i] <<
			// std::endl;
			Objects[i] = nullptr;
			//~ Objects.erase(Objects.begin() + i);
			//~ std::cout << "He's null now. PROOF:" << Objects[i] << std::endl;
		}
	obj->deRegister(theScene, world, myWorldType);
}

GkObject* GkEngine::addObject(std::string filename, bool templated, std::string name, Transform init_transform, size_t id) {

	GkObject* retval = Resources.getAnObject(filename, templated, init_transform);
	name_compat(name);
	// Register the object to the scene here.
	if (retval) {
		retval->name = name;
		if (id == std::numeric_limits<size_t>::max()) {
			while (getObject(id_ticker))
				id_ticker++;
			retval->setID(id_ticker++);
		} else if (id < std::numeric_limits<size_t>::max()) {
			while (getObject(id)) // Ensure the id is unique.
				id++;
			retval->setID(id);
		}
		bool foundNullPtr = false;
		for (size_t i = 0; i < Objects.size(); i++)
			if ((Objects[i]) == nullptr) {
				foundNullPtr = true;
				//~ std::cout << "\nFOUND NULLPTR TO PLACE OBJECT AT!!!" <<
				// std::endl;
				Objects[i] = retval;
				break;
			}
		if (!foundNullPtr) {
			Objects.push_back(retval);
			//~ std::cout << "\nPushing back." << std::endl;
		}
		retval->Register(theScene, world, myWorldType);
	}
	return retval;
}
void GkEngine::addConstraint(btTypedConstraint* constraint, bool ncollide) {
	for (size_t i = 0; i < Constraints.size(); i++) {
		if (Constraints[i] == constraint)
			return;
	}
	world->addConstraint(constraint, ncollide);
	for (size_t i = 0; i < Constraints.size(); i++) {
		if (Constraints[i] == nullptr) {
			Constraints[i] = constraint;
			return;
		}
	}
	Constraints.push_back(constraint);
}
void GkEngine::removeConstraint(btTypedConstraint* constraint) {
	for (size_t i = 0; i < Constraints.size(); i++) {

		if (Constraints[i] == constraint)
			Constraints[i] = (btTypedConstraint*)nullptr;
	}
	world->removeConstraint(constraint);
}
GkObject* GkEngine::getObject(std::string name) {
	for (uint i = 0; i < Objects.size(); i++)
		if (Objects[i] && Objects[i]->name == name)
			return Objects[i];
	return nullptr;
} // by name
GkObject* GkEngine::getObject(size_t ID) {
	for (size_t i = 0; i < Objects.size(); i++)
		if (Objects[i] && Objects[i]->ID == ID)
			return Objects[i];
	return nullptr;
} // by ID
void GkEngine::guaranteeUniqueIDs() {
	for (GkObject*& item : Objects)
		if (item)
			while (getObject(item->getID()) != item)
				item->setID(item->getID() + 1);
}
void GkEngine::removeObjectsMarkedForDeletion() {
	for (size_t i = Objects.size(); i > 0; i--) {
		if (Objects[i - 1] && Objects[i - 1]->markedForDeletion) {
			removeObject(Objects[i - 1]);
			i++;
		}
	}
}

void GkEngine::removeObject(GkObject* obj) {
	if (!obj || Objects.size() < 1)
		return;
	//~ std::cout << "\nENTERED removeObject!" << std::endl;
	if (Constraints.size())
		for (uint constraint = Constraints.size() - 1; constraint >= 0; constraint--)
			if (Constraints[constraint])
				for (auto* body : obj->RigidBodies) // Loop through all constraints and rigid
													// bodies
				{
					//~ std::cout << "\nENTERED the constraint loop!" <<
					// std::endl;
					if (&(Constraints[constraint]->getRigidBodyA()) == body || &(Constraints[constraint]->getRigidBodyB()) == body) // If this constraint has
																																	// this rigid body in it...
					{

						world->removeConstraint(Constraints[constraint]); // Remove from our world!
						delete (Constraints[constraint]);				  // Delete from our
																		  // universe!
						Constraints[constraint] = nullptr;
					}
				}
	//TODO: Remove all weld objects from the world which contain this object. And if this is a weld object,
	//restore all child objects to being in the world.
	for (size_t i = Objects.size() - 1; i < Objects.size() && i >= 0; i--)
		if (Objects[i] == obj) {
			//~ std::cout << "\nFound our man. He's: " << Objects[i] <<
			// std::endl;
			Objects[i] = nullptr;
			//~ Objects.erase(Objects.begin() + i);
			//~ std::cout << "He's null now. PROOF:" << Objects[i] << std::endl;
		}
	
	obj->deRegister(theScene, world, myWorldType);
	//~ std::cout << "\nDeleting..." << std::endl;
	delete obj; // so we can say delete removeObject(obj); and it's super slick
				// and shit
				//~ std::cout << "Finished Removing Object." << std::endl;
} // by pointer

void GkEngine::PurgeUnusedTemplates() {} // TODO: implement
// Tells all GkObjects registered to synchronize their rendered objects with
// their BtCollisionObjects
void GkEngine::performCulling(){
	//~ btConvexHullShape* b = getFrustumShape(&MainCamera);
	//~ if(!b) return;
	
	//~ btAABB camaabb;
	btTransform t; t.setIdentity();
	//~ b->getAabb(t, camaabb.m_min, camaabb.m_max);
	//~ delete b;
	std::vector<glm::vec4> frust = extractFrustum(MainCamera.getViewProjection());
	for(auto* obj: Objects)
		if(obj && !obj->disableCulling)
		{
			bool hasIntersection = false;
			for(auto* rigidBody: obj->RigidBodies)
				if(rigidBody)
				{
					rigidBody->getMotionState()->getWorldTransform(t);
					btAABB aabb;
					rigidBody->getCollisionShape()->getAabb(t, aabb.m_min, aabb.m_max);
					//~ hasIntersection = hasIntersection || (camaabb.has_collision(rbaabb));
					//Test the planes against the points of the AABB
					bool inside = true;
					for(glm::vec4& plane: frust){
						float d = std::max(aabb.m_min.x() * plane.x, aabb.m_max.x() * plane.x)
							  + std::max(aabb.m_min.y() * plane.y, aabb.m_max.y() * plane.y)
							  + std::max(aabb.m_min.z() * plane.z, aabb.m_max.z() * plane.z)
							  + plane.w;
						inside = inside && d > 0;
						if(hasIntersection)break;
					}
					hasIntersection = hasIntersection || inside;
					if(hasIntersection)break;
				}
			if(hasIntersection || obj->disableCulling){
				for(auto* mi: obj->MeshInstances)
					mi->culled = false;
			} else {
				for(auto* mi: obj->MeshInstances)
					mi->culled = true;
			}
		}
	if(myVoxelWorld)
		myVoxelWorld->FrustumCull(frust);
	
}
void GkVoxelWorld::FrustumCull(std::vector<glm::vec4>& frust){
	btTransform t;
	for(auto* chunk: chunks)
		if(chunk && chunk->myRigidBody)
		{
			auto* rigidBody = chunk->myRigidBody;
			t = rigidBody->getCenterOfMassTransform();
			btAABB aabb;
			rigidBody->getCollisionShape()->getAabb(t, aabb.m_min, aabb.m_max);
			
			bool inside = true;
			for(glm::vec4& plane: frust){
				float d = std::max(aabb.m_min.x() * plane.x, aabb.m_max.x() * plane.x)
					  + std::max(aabb.m_min.y() * plane.y, aabb.m_max.y() * plane.y)
					  + std::max(aabb.m_min.z() * plane.z, aabb.m_max.z() * plane.z)
					  + plane.w;
				inside = inside && d > 0;
			}
			if(inside){
				chunk->myMeshInstance.culled = false;
			} else {
				chunk->myMeshInstance.culled = true;
			}
		}
}

void GkEngine::resetContactInfo() {
	for (auto* item : Objects)
		if (item)
			item->ColInfo.reset();
}
void GkEngine::updateContactInfo() {
	if (!world) // There is nothing to do.
		return;
	int numManifolds = world->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) {
		btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
		btCollisionObject* obA = (btCollisionObject*)(contactManifold->getBody0());
		btCollisionObject* obB = (btCollisionObject*)(contactManifold->getBody1());
		if (!obA || !obB || obA == obB)
			continue; // Bad contact. Don't know how that could happen.
		GkCollisionInfo* obAColInfo = (GkCollisionInfo*)obA->getUserPointer();
		GkCollisionInfo* obBColInfo = (GkCollisionInfo*)obB->getUserPointer();
		if (!obAColInfo || !obBColInfo) // User is idiot
		{
			continue;
		}
		if (obAColInfo == obBColInfo) // Part of the same object. Don't process.
		{
			continue;
		}
		GkbtContactInformation forA;
		GkbtContactInformation forB;
		forA.body = obA;
		forB.body = obB;
		forA.other = obB;
		forB.other = obA;
		forA.bodyInternalType = obA->getInternalType();
		forB.bodyInternalType = obB->getInternalType();
		forA.otherInternalType = obB->getInternalType();
		forB.otherInternalType = obA->getInternalType();
		if (obAColInfo->myGkObject)
			forB.otherID = ((GkObject*)(obAColInfo->myGkObject))->getID();
		if (obBColInfo->myGkObject)
			forA.otherID = ((GkObject*)(obBColInfo->myGkObject))->getID();
		int numContacts = contactManifold->getNumContacts();
		btVector3 maxptA;
		btVector3 maxptB;
		float totalImpulse;
		float maxImpulse = 0;
		bool foundOne = false;
		for (int j = 0; j < numContacts; j++) {
			btManifoldPoint& pt = contactManifold->getContactPoint(j);

			btVector3 ptA = pt.getPositionWorldOnA();
			btVector3 ptB = pt.getPositionWorldOnB();
			float impulse = pt.getAppliedImpulse();
			if (pt.getDistance() < 0.f) {

				totalImpulse += impulse;
				if (!foundOne) {
					maxptA = ptA;
					maxptB = ptB;
					maxImpulse = impulse;
					foundOne = true;
				}
				if (impulse > maxImpulse) {
					maxptA = ptA;
					maxptB = ptB;
					maxImpulse = impulse;
				}
			}
		} // EOF looping through contacts in a manifold
		if (foundOne) {
			forA.force = maxImpulse;
			forB.force = maxImpulse;
			forA.Worldspace_Location = (maxptA);
			forB.Worldspace_Location = (maxptB);
			obAColInfo->addContact(forA);
			obBColInfo->addContact(forB);
		}
	} // EOF looping through manifolds
}
void GkEngine::PlayColSounds() {
	for (auto* item : Objects)
		if (item)
			if (!soundHandler.isNull)
				item->PlayColSounds(&soundHandler);
}
void GkEngine::Rendering_Sync() {
	// First sync the AL listener.
	soundHandler.syncALListener(&MainCamera);
	
	// Normal Object
	for (auto* item : Objects)
		if (item) {
			item->Sync();
		}
}

// ParticleRenderers
void GkEngine::tickParticleRenderers(float t) {
	for (auto*& item : ParticleRenderers)
		if (item) {
			item->tickTime(t);
			if (item->AutoAddParticles)
				item->addParticle();
		}
}
void GkEngine::resetTimeParticleRenderers() {
	for (auto*& item : ParticleRenderers)
		if (item)
			item->resetTime();
}
void GkEngine::DrawAllParticles(Camera* RenderCamera) {
	if (!RenderCamera)
		RenderCamera = &MainCamera;
	for (auto*& item : ParticleRenderers)
		if (item)
			item->DrawParticles(RenderCamera);
}
void GkEngine::addParticleRenderer(ParticleRenderer* p) {
	if (!p)
		return;
	for (auto*& p_ : ParticleRenderers)
		if (p == p_)
			return;
	ParticleRenderers.push_back(p);
}
void GkEngine::removeParticleRenderer(ParticleRenderer* p) {
	for (auto*& p_ : ParticleRenderers)
		if (p == p_)
			p_ = nullptr;
}

void GkEngine::ECS_Midframes() {
	for (GkEntityComponentProcessor*& ECSP : ECSProcessors)
		if (ECSP)
			ECSP->MidFrameProcess();
}
void GkEngine::ECS_Splits() {
	for (GkEntityComponentProcessor*& ECSP : ECSProcessors)
		if (ECSP)
			ECSP->StartSplitProcessing();
}
void GkEngine::ECS_Halts() {
	for (GkEntityComponentProcessor*& ECSP : ECSProcessors)
		if (ECSP)
			ECSP->ConfirmProcessingHalted();
}
void GkEngine::registerECSProcessor(GkEntityComponentProcessor* b) {
	if (!b)
		return;
	b->myEngine = (void*)this;
	for (GkEntityComponentProcessor*& ECSP : ECSProcessors)
		if (!ECSP) {
			ECSP = b;
			return;
		}
	ECSProcessors.push_back(b);
}
void GkEngine::deregisterECSProcessor(GkEntityComponentProcessor* b) {
	if (!b)
		return;
	b->myEngine = nullptr;
	for (GkEntityComponentProcessor*& ECSP : ECSProcessors)
		if (ECSP == b) {
			ECSP = nullptr;
		}
}
void GkEngine::updateCursorInfo(double _x, double _y) {
	oldmousexy[0] = currentmousexy[0];
	oldmousexy[1] = currentmousexy[1];
	//~ myDevice->getCursorPosition(0, &(currentmousexy[0]),
	//&(currentmousexy[1]));
	currentmousexy[0] = _x;
	currentmousexy[1] = _y;
}
glm::vec2 GkEngine::getCursorDelta() { return glm::vec2(currentmousexy[0] - oldmousexy[0], currentmousexy[1] - oldmousexy[1]); }
glm::vec2 GkEngine::getNormalizedCursorDelta() {
	return glm::vec2((currentmousexy[0] - oldmousexy[0]) / theScene->getWidth(), (currentmousexy[1] - oldmousexy[1]) / theScene->getHeight());
}
glm::vec2 GkEngine::getNormalizedCursorPos() { return glm::vec2((currentmousexy[0]) / theScene->getWidth(), (currentmousexy[1]) / theScene->getHeight()); }
GkScene* GkEngine::getScene() { return theScene; }
Camera* GkEngine::getCamera() { return &MainCamera; }
btDiscreteDynamicsWorld* GkEngine::getWorld() { return world; }
IODevice* GkEngine::getDevice() { return myDevice; }
GkSoundHandler* GkEngine::getSoundHandler() { return &soundHandler; }
BMPFontRenderer* GkEngine::getFont() { return myFont; }
GkEngine::~GkEngine() { destroy_GkEngine(); }
void GkEngine::destroy_GkEngine() {
	//~ std::cout << "\nReached destroy_GkEngine!" << std::endl;
	/*
	std::vector<gekRender::PointLight*> Point_Lights;
	std::vector<gekRender::DirectionalLight*> Dir_Lights;
	std::vector<gekRender::AmbientLight*> Amb_Lights;
	std::vector<gekRender::CameraLight*> Cam_Lights;
	 * */
	ECS_Halts();
	if(myVoxelWorld)
		delete myVoxelWorld;
	myVoxelWorld = nullptr;
	for (auto* item : Objects)
		if (item)
			item->deRegister(theScene, world, myWorldType);
	if (myFont)
		delete myFont;
	myFont = nullptr;
	for (GkEntityComponentProcessor*& ECSP : ECSProcessors)
		if (ECSP)
			delete ECSP;
	ECSProcessors.clear();
	for (auto* item : ParticleRenderers)
		if (item)
			delete item;
	ParticleRenderers.clear();
	if (ParticleShader)
		delete ParticleShader;
	ParticleShader = nullptr;
	for (auto* item : Constraints)
		if (item)
			delete item;
	Constraints.clear();
	for (auto* item : Point_Lights)
		if (item)
			delete item;
	Point_Lights.clear();
	//~ for (auto* item : Dir_Lights)
		//~ if (item)
			//~ delete item;
	//~ Dir_Lights.clear();
	for (auto* item : Amb_Lights)
		if (item)
			delete item;
	Amb_Lights.clear();
	for (auto* item : Cam_Light_Shadowmap_FBOs)
		if (item)
			delete item;
	Cam_Light_Shadowmap_FBOs.clear();
	for (auto* item : Cam_Lights)
		if (item)
			delete item;
	Cam_Lights.clear();

	while (Objects.size() > 0) {
		if (Objects.front())
			delete Objects.front();
		Objects.erase(Objects.begin());
	}
	Resources.UnloadAll();
	// First, delete the scene
	if (theScene)
		delete theScene;
	// Delete all shaders
	if (MainShad)
		delete MainShad;
	if (MainShaderShadows)
		delete MainShaderShadows;
	if (DisplayTexture)
		delete DisplayTexture;
	if (SkyboxShad)
		delete SkyboxShad;
	if (WBOITCompShader)
		delete WBOITCompShader;
	// Delete all Bullet Physics constructs
	if (world)
		delete world;
	if (dispatcher)
		delete dispatcher;
	if (collisionConfig)
		delete collisionConfig;
	if (broadphase)
		delete broadphase;
	if (solver)
		delete solver;
	if (softbodysolver)
		delete softbodysolver;
	// Loop through all GkObjects, set their properly deregistered flag
	// And delete them

	// Delete the device last
	if (myDevice)
		delete myDevice;
}
void GkEngine::setCursorLock(bool state){
	if(state)
		getDevice()->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	else
		getDevice()->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

btDiscreteDynamicsWorld* GkEngine::makeWorld(
	// And you answer questions about what you want
	glm::vec3 GravityForce, // What's the magnitude of Gravity in this world?
	bool SoftBodyPhysics	// Do we care about soft body physics?
) {							// Note: Just because we're making the world does not mean we're not setting
	// the world.
	if (world || dispatcher || collisionConfig || broadphase || solver || softbodysolver) {
		std::cout << "\nERROR! Stuff for world creation already exists!";
		return nullptr;
	}
	if (SoftBodyPhysics) { // Make a btSoftRigidDynamicsWorld
		myWorldType = BTSOFTRIGIDWORLD;
		collisionConfig = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfig);
		solver = new btSequentialImpulseConstraintSolver();
		broadphase = new btDbvtBroadphase();
		softbodysolver = new btDefaultSoftBodySolver();
		world = new btSoftRigidDynamicsWorld(dispatcher, broadphase, solver, collisionConfig, softbodysolver);

	} else { // Make a btDiscreteDynamicsWorld
		myWorldType = BTDISCRETEWORLD;
		collisionConfig = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfig);
		solver = new btSequentialImpulseConstraintSolver();
		broadphase = new btDbvtBroadphase();
		world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
	}
	world->setGravity(btVector3(GravityForce.x, GravityForce.y, GravityForce.z));
	Resources.myWorldType = myWorldType;
	Resources.world = world;
	return world;
}

std::string GkEngine::GkObjectExporterHook(GkObject* me, std::string savedir){ return ""; }
//~ std::string GkEngine::GkObjectExporterHook(GkObject* me, std::string savedir){
	//~ if(!me)
		//~ return "";
	//~ //Do not allow non-independent GkObjects to export.
	//~ if(me->myAssetMode != INDEPENDENT)
		//~ return "";
	//~ std::string GSGEstring = "";
	//~ //The GSGE file to write to.
	//~ std::string ofilename = savedir + "GKOBJ_" + std::to_string(me->getID()) + ".gsge";
	//~ //Write the outfile name to the object to be saved.
	//~ me->GkObjectFile = ofilename;
	//~ //Beginning of file names for custom shapes and whatnot.
	//~ std::string baseofilename = savedir + "GKOBJ_" + std::to_string(me->getID()) + "_";
	//~ //FIRST EXPORT SYNCMODE
	//~ GSGEstring += "\nSYNCMODE|";
	//~ if(me->mySyncMode == SYNC_ANIMATED)	GSGEstring += "ANIMATED\n";
	//~ else if(me->mySyncMode == SYNC_ONE_TO_ONE)	GSGEstring += "ONE_TO_ONE\n";
	//~ GSGEstring += "NONE\n";
	
	//~ if(me->colSoundFileName != ""){
		//~ GSGEstring += "\nCOLSOUND|" + me->colSoundFileName + "\n";
		//~ GSGEstring += "\nLOUDNESS|" + std::to_string(me->ColInfo.colSoundLoudness) + "\n";
		//~ GSGEstring += "\nMINCOLSOUNDIMPULSE|" + std::to_string(me->ColInfo.colSoundMinImpulse) + "\n";
		//~ GSGEstring += "\nMINPITCH|" + std::to_string(me->ColInfo.colSoundMinPitch) + "\n";
		//~ GSGEstring += "\nMAXPITCH|" + std::to_string(me->ColInfo.colSoundMaxPitch) + "\n";
		//~ GSGEstring += "\nMINGAIN|" + std::to_string(me->ColInfo.colSoundMinGain) + "\n";
		//~ GSGEstring += "\nMAXGAIN|" + std::to_string(me->ColInfo.colSoundMaxGain) + "\n";
		//~ GSGEstring += "\nMAXDISTANCE|" + std::to_string(me->ColInfo.colSoundMaxDistance) + "\n";
		//~ GSGEstring += "\nROLLOFF|" + std::to_string(me->ColInfo.colSoundRolloffFactor) + "\n";
	//~ } else {
		//~ GSGEstring += "\nDISABLECOLSOUNDS\n";
	//~ }
	//~ //TODO export Meshes, then collision shapes, then meshinstances, then rigid bodies, then constraints.
	//~ for(size_t i = 0; i < me->OwnedMeshes.size(); i++) if(me->OwnedMeshes[i]){
		//~ Mesh*& Meshi = me->OwnedMeshes[i];
		//~ IndexedModel b = Meshi->getShape();
		//~ std::string obj_filename = baseofilename + "MESH_"+std::to_string(i) + ".obj";
		//~ std::ofstream bruh; bruh.open(obj_filename); bruh << b.exportToString(true); bruh.close();
		//~ GSGEstring += "\nMESH|" + obj_filename + "|1|" + std::to_string((int)Meshi->is_static) + "|" + std::to_string((int)Meshi->is_instanced) + "|0\n";
		//~ GSGEstring += "\nPHONG_I_DIFF|" + std::to_string(Meshi->InstancedMaterial.diffusivity) + "\n";
		//~ GSGEstring += "\nPHONG_I_CUBE_REF|" + std::to_string((int)Meshi->instanced_enable_cubemap_reflections) + "\n";
		//~ GSGEstring += "\nPHONG_I_SPECR|" + std::to_string(Meshi->InstancedMaterial.specreflectivity) + "\n";
		//~ GSGEstring += "\nPHONG_I_SPECD|" + std::to_string(Meshi->InstancedMaterial.specdamp) + "\n";
		//~ GSGEstring += "\nPHONG_I_AMB|" + std::to_string(Meshi->InstancedMaterial.ambient) + "\n";
		//~ GSGEstring += "\nPHONG_I_EMISS|" + std::to_string(Meshi->InstancedMaterial.emissivity) + "\n";
		//~ for(size_t j = 0; j < Meshi->getTextureVectorPtr()->size(); j++){
			//~ SafeTexture& safetex = (*(Meshi->getTextureVectorPtr()))[j];
			//~ if(safetex.DaddyO){
				//~ GSGEstring += "\nTEXTURE|" + safetex.DaddyO->MyName + "|" + std::to_string((int)safetex.amITransparent()) + "|LINEAR|REPEAT|4.0\n";
			//~ }
		//~ }
		//~ for(size_t j = 0; j < Meshi->getCubeMapVectorPtr()->size(); j++){
			//~ if((*Meshi->getCubeMapVectorPtr())[j])
			//~ {
				//~ CubeMap& cm = *((*(Meshi->getCubeMapVectorPtr()))[j]);
				//~ GSGEstring += "\nCUBEMAP|" + cm.MyName + "\n";
			//~ }
			
		//~ }
	//~ }
	//~ //Meshes involves exporting all the textures too.
	//~ std::ofstream b; b.open(ofilename);
	//~ if(b.is_open()){
		//~ b << GSGEstring;
		//~ b.close();
		//~ return me->Serialize(savedir);
	//~ } 
	//~ return "";
//~ }

// GkObject Functions~~~~~~~~
GkObject::GkObject() {
	isNull = true;
	ColInfo.myGkObject = (void*)this;
	//~ ColInfo.RigidBodiesVector = &RigidBodies; // all pointers are the same size, so it should work.
}
void GkObject::addOwnedEntityComponent(GkEntityComponent* new_owned) {
	for (auto*& item : OwnedEntityComponents) // Avoid duplicate additions.
		if (item == new_owned) {
			return;
		}
	OwnedEntityComponents.push_back(new_owned);
}
GkObject::~GkObject() { destruct(); }
size_t GkObject::getID() { return ID; }
void GkObject::setID(size_t _ID) { ID = _ID; }

bool GkObject::isSingleBodyProp() { // Can this object be welded?
	return ((mySyncMode == SYNC_ANIMATED || mySyncMode == SYNC_ONE_TO_ONE) && (RigidBodies.size() == 1) && (RigidBodies[0]));
}
std::string GkObject::Serialize(std::string savepath) {
	std::string retval = "\n";
	if (donotsave)
		return retval;
	if (myAssetMode != TEMPLATED && requiresExporting) {
		//Export a virtual file. Hooks into Resources, effectively creating a placeholder GSGE file.
		retval += "\nVFILE|" + GkObjectFile;
		retval += "\n" + ExportString + "ENDVFILE\n";
	}
	
	retval += "OBJECT|";
	retval += GkObjectFile;
	retval += "|";
	retval += name;
	retval += "|";
	retval += std::to_string((int)(myAssetMode == TEMPLATED));
	retval += "|"; // Only the creation scale is exported.
	retval += std::to_string(creationScale.x);
	retval += "|";
	retval += std::to_string(creationScale.y);
	retval += "|";
	retval += std::to_string(creationScale.z);
	retval += "|";
	retval += std::to_string((unsigned long long)ID);
	retval += "\n";
	btTransform t;
	btVector3 v;
	for (size_t i = 0; i < RigidBodies.size(); i++)
		if (RigidBodies[i]) {
			//~ RigidBodies[i]->getMotionState()->getWorldTransform(t);
			t = RigidBodies[i]->getCenterOfMassTransform();
			Transform bigT;
			bigT.setModel(b2g_transform(t));
			glm::mat4 m = bigT.getModel();
			glm::vec3 pos = bigT.getPos();
			glm::vec3 rot = bigT.getRot();
			retval += "\nSETRIGIDBODYTRANSFORM|";
			retval += std::to_string((unsigned long long)ID);
			retval += "|";
			retval += std::to_string(i);
			retval += "|";
			for (uint i = 0; i < 16; i++)
				retval += std::to_string(m[i / 4][i % 4]) + "|";
			retval += "\n";
			// Now, export the linear and angular velocity.
			v = RigidBodies[i]->getLinearVelocity();
			pos = b2g_vec3(v);
			v = RigidBodies[i]->getAngularVelocity();
			rot = b2g_vec3(v);
			retval += "SETRIGIDBODYVEL|";
			retval += std::to_string((unsigned long long)ID);
			retval += "|";
			retval += std::to_string(i);
			retval += "|";
			retval += std::to_string(pos.x);
			retval += "|";
			retval += std::to_string(pos.y);
			retval += "|";
			retval += std::to_string(pos.z);
			retval += "|";
			retval += std::to_string(rot.x);
			retval += "|";
			retval += std::to_string(rot.y);
			retval += "|";
			retval += std::to_string(rot.z);
			retval += "\n";
			{
				retval += "SETRIGIDBODYMASS|";
				retval += std::to_string((unsigned long long)ID);
				retval += "|";
				retval += std::to_string(i) + "|";
				retval += std::to_string(1 / (RigidBodies[i]->getInvMass())) + "|";
				btVector3 b = RigidBodies[i]->getLocalInertia();
				retval += std::to_string(b.x()) + "|";
				retval += std::to_string(b.y()) + "|";
				retval += std::to_string(b.z()) + "\n";
			}
			if (RigidBodies[i]->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT) {
				retval += "RIGIDBODYKINEMATIC|";
				retval += std::to_string((unsigned long long)ID);
				retval += "|";
				retval += std::to_string(i) + "\n";
			}
			retval += "SETRIGIDBODYFRICTION|" + std::to_string((unsigned long long)ID) + "|" + std::to_string(i) + "|" +
					  std::to_string(RigidBodies[i]->getFriction()) + "\n";
			retval += "SETRIGIDBODYRESTITUTION|" + std::to_string((unsigned long long)ID) + "|" + std::to_string(i) + "|" +
					  std::to_string(RigidBodies[i]->getRestitution()) + "\n";
		}
	for (auto*& item : OwnedEntityComponents)
		if (item)
			retval += item->SerializeGkMap(savepath);
	return retval;
}

void GkObject::PlayColSounds(GkSoundHandler* soundHandler) {
	if (ColInfo.colsoundbuffer == NO_BUFFER || ColInfo.disableColSounds || ColInfo.noColSoundsThisFrame)
		return;
	for (uint i = 0; i < GK_MAX_CONTACTS && i < ColInfo.numContacts; i++)
		if (!ColInfo.isContactOld(i) && (ColInfo.Contacts[i].force > ColInfo.colSoundMinImpulse)) {
			// TODO: make above isContactOld check obsolete.
			//~ GkSoundHandler::playSound(ALuint soundbuffer, glm::vec3 loc, glm::vec3 vel, ALuint source, float gain, float pitch, float mingain, float
			// maxgain, ~ float rolloff_factor, float max_distance, bool looping)
			auto& contact = ColInfo.Contacts[i];
			// Don't play collision sounds for ghost objects.
			if (contact.otherInternalType == btCollisionObject::CO_GHOST_OBJECT || contact.bodyInternalType == btCollisionObject::CO_GHOST_OBJECT)
				continue;

			soundHandler->playSound(ColInfo.colsoundbuffer, b2g_vec3(contact.Worldspace_Location), glm::vec3(0, 0, 0), NO_SOURCE,
									(ColInfo.colSoundLoudness * contact.force), RandomFloat(ColInfo.colSoundMinPitch, ColInfo.colSoundMaxPitch),
									ColInfo.colSoundMinGain, ColInfo.colSoundMaxGain, ColInfo.colSoundRolloffFactor, ColInfo.colSoundMaxDistance, false);
		}
}
void GkObject::Sync() {

	// there is nothing to synchronize.
	if (RigidBodies.size() == 0 || MeshInstances.size() == 0 || mySyncMode == NO_SYNC)
		return;
	btTransform baseTransform;
	if (mySyncMode == SYNC_ANIMATED) {
		RigidBodies[0]->getMotionState()->getWorldTransform(baseTransform);
		glm::mat4 actual_transform = b2g_transform(baseTransform);
		for (uint i = 0; i < MeshInstances.size(); i++) {
			glm::mat4 LocalspaceTransform = glm::mat4(1);
			if (RelativeTransforms.size() > i)
				LocalspaceTransform = RelativeTransforms[i];
			//~ if(name == "SomeNiceFloor")
			//~ std::cout << "\nScaling: " << LocalspaceTransform[0][0];
			MeshInstances[i]->myTransform.setModel(actual_transform * LocalspaceTransform);
			if (Hitboxes.size() > i && Hitboxes[i]) // Set the world transform of the hitbox
			{
				btTransform bt = g2b_transform(actual_transform * LocalspaceTransform);
				Hitboxes[i]->setCenterOfMassTransform(bt);
			}
		}
	} else if (mySyncMode == SYNC_ONE_TO_ONE) {
		for (uint i = 0; i < MeshInstances.size(); i++) {
			// Determine which RigidBody it should sync to.
			uint syncindex = i % RigidBodies.size();
			RigidBodies[syncindex]->getMotionState()->getWorldTransform(baseTransform);
			glm::mat4 actual_transform = b2g_transform(baseTransform);
			glm::mat4 LocalspaceTransform = glm::mat4(1);
			if (RelativeTransforms.size() > i)
				LocalspaceTransform = RelativeTransforms[i];
			//~ std::cout << "\nSYNCING MESHINSTANCE TO RIGIDBOYD IN ONE TO
			// ONE..." << std::endl;
			MeshInstances[i]->myTransform.setModel(actual_transform * LocalspaceTransform);
			if (Hitboxes.size() > i && Hitboxes[i]) // Set the world transform of the hitbox
			{
				btTransform bt = g2b_transform(actual_transform * LocalspaceTransform);
				Hitboxes[i]->setCenterOfMassTransform(bt);
			}
		}
	} else if (mySyncMode == SYNC_SOFTBODY) {
		// TODO: figure out how the hell this will work, because quite frankly?
		// I don't understand how.
	}
}
void GkObject::ApplyTransformToAnimGroup(std::string anim_name, glm::mat4 transform) {

	if (!Animations || !Animations->count(anim_name) || (*Animations)[anim_name].Frames.size() < 1)
		return;
	std::vector<glm::mat4>& frame = (*Animations)[anim_name].Frames[0]; // Setting a reference.
	while (RelativeTransforms.size() < frame.size())					// In case we don't have any relative transforms or
																		// somethin
		RelativeTransforms.push_back(glm::mat4(1));
	for (uint i = 0; i < frame.size(); i++) {
		if (frame[i][0][0] != std::numeric_limits<float>::max()) // if this isn't an infinity
																 // "placeholder" animation
																 // matrix.
			RelativeTransforms[i] = transform * glm::inverse(frame[i]) * RelativeTransforms[i];
	}
}
void GkObject::setAnimation(std::string anim_name, float t) {
	if (Animations) {
		setAnimation((*Animations)[anim_name], t);
	} else {
		RelativeTransforms = std::vector<glm::mat4>();
		return;
	}
}
void GkObject::setAnimation(GkAnim& anim, float t) {
	if (anim.Frames.size() < 1 || anim.timing == 0) {
		return;
	}
	//~ while (anim.getDuration() < t)
	//~ t -= anim.getDuration();
	//~ while (t < 0)
	//~ t += anim.getDuration();

	// Pick our frame to set.
	long long int frame_to_set = ((long long int)(t / anim.timing));
	while (frame_to_set < 0)
		frame_to_set += anim.Frames.size();

	frame_to_set %= anim.Frames.size();
	std::vector<glm::mat4>& frame = anim.Frames[frame_to_set];

	while (RelativeTransforms.size() < frame.size()) // In case we don't have any relative transforms or
													 // somethin
		RelativeTransforms.push_back(glm::mat4(1));

	// RelativeTransforms = anim[frame_to_set]; //Old code
	// New code (allows for partial object animation)
	for (uint i = 0; i < frame.size(); i++) {
		if (frame[i][0][0] != std::numeric_limits<float>::max()) // if this isn't an infinity
																 // "placeholder" animation
																 // matrix.
		{
			RelativeTransforms[i] = frame[i];
		}
	}
}
void GkObject::clearAnimation() {
	RelativeTransforms = std::vector<glm::mat4>();
	return;
}

void GkObject::Register(GkScene* theScene, btDiscreteDynamicsWorld* world, WorldType myWorldType) {
	if (!partialDeregistered)
		for (auto* item : OwnedMeshes)
			theScene->registerMesh(item);
	for (auto* item : RigidBodies) {
		//~ std::cout << "\nADDING RIGID BODY TO WORLD!" << std::endl;
		if(!item->getBroadphaseHandle())
			world->addRigidBody(item, COL_NORMAL | COL_HITSCAN, COL_NORMAL | COL_TRIGGER | COL_CHARACTERS);
		// Member of NORMAL, collides with NORMAL only.
	}
	for (auto* item : Constraints)
		if (item)
			world->addConstraint(item, false); // Nocollide is the boolean.
	for (auto* item : Hitboxes)
		if (item)
			world->addRigidBody(item, COL_HITSCAN, COL_HITRAY);
	haveProperlyDeRegistered = false;
	partialDeregistered = false;
}
void GkObject::deRegister(GkScene* theScene, btDiscreteDynamicsWorld* world, WorldType myWorldType, bool partial) {
	// First deregister Meshinstances from shared meshes
	//~ std::cout << "\nDeregistering..." << std::endl;
	//~ std::cout << "\nMy name is:" << name << " and I'm deregistering." <<
	// std::endl;

	if (!partial) {
		for (auto* item : OwnedEntityComponents) {
			if (item) {
				item->deregisterFromProcessor();
			}
		}
		for (int i = 0; i < MeshInstances.size(); i++) // Deregister Meshinstances
		{
			//~ for(int j = 0; j < SharedMeshes.size(); j++)
			for (auto* SharedMesh : SharedMeshes)
				SharedMesh->deregisterInstance(MeshInstances[i]);
		}
		SharedMeshes.clear();
		// Second, deregister Owned Meshes
		for (int j = 0; j < OwnedMeshes.size(); j++)
			if (OwnedMeshes[j] != nullptr)
				theScene->deregisterMesh(OwnedMeshes[j]);
	}

	// Third, deregister Rigid bodies
	for (int i = 0; i < RigidBodies.size(); i++)
		if (RigidBodies[i])
			world->removeRigidBody(RigidBodies[i]);
	// Same for ghost objects
	for (int i = 0; i < Hitboxes.size(); i++)
		if (Hitboxes[i])
			world->removeRigidBody(Hitboxes[i]);
	// Same for constraints
	for (int i = 0; i < Constraints.size(); i++)
		if (Constraints[i])
			world->removeConstraint(Constraints[i]);
	if (!partial)
		haveProperlyDeRegistered = true;
	partialDeregistered = partial;
	//~ std::cout << "\nFinished Deregistering." << std::endl;
}

// FUNCTIONS WHICH WORK ON AN OBJECT WITH ONLY A SINGLE RIGID BODY
void GkObject::makeCharacterController() { // Turn this GkObject into a
										   // character controller.
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return;
	btVector3 ang(0, 0, 0);
	RigidBodies[0]->setAngularFactor(ang);
}
void GkObject::makeKinematic(btDiscreteDynamicsWorld* world) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr || !RigidBodies[0]->getBroadphaseHandle())
		return;
	//~ btVector3 inertia(0, 0, 0);
	// get the group and mask first.
	int group = RigidBodies[0]->getBroadphaseHandle()->m_collisionFilterGroup;
	int mask = RigidBodies[0]->getBroadphaseHandle()->m_collisionFilterMask;
	world->removeRigidBody(RigidBodies[0]);
	//~ RigidBodies[0]->setMassProps(0, inertia);
	RigidBodies[0]->setCollisionFlags(RigidBodies[0]->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	world->addRigidBody(RigidBodies[0], group, mask);
}
void GkObject::makeDynamic(btDiscreteDynamicsWorld* world) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr || !RigidBodies[0]->getBroadphaseHandle())
		return;
	int group = RigidBodies[0]->getBroadphaseHandle()->m_collisionFilterGroup;
	int mask = RigidBodies[0]->getBroadphaseHandle()->m_collisionFilterMask;

	world->removeRigidBody(RigidBodies[0]);
	RigidBodies[0]->setCollisionFlags(RigidBodies[0]->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
	world->addRigidBody(RigidBodies[0], group, mask);
}
void GkObject::setRot(glm::vec3 rot) { setRot(glm::quat(rot)); }
void GkObject::setRot(glm::quat rot) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return;
	Transform trans;
	btTransform t;
	auto* Body = RigidBodies[0];
	Body->getMotionState()->getWorldTransform(t);
	trans.setModel(b2g_transform(t));
	trans.setRotQuat(rot);
	t = g2b_transform(trans.getModel());
	Body->setCenterOfMassTransform(t);
	Body->getMotionState()->setWorldTransform(t);
}
void GkObject::setPos(glm::vec3 pos, bool resetForces, bool resetLinVel, bool resetAngVel) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return;
	auto* Body = RigidBodies[0];
	Transform trans;
	btTransform t;
	const btVector3 zeroes(0, 0, 0);
	Body->getMotionState()->getWorldTransform(t);
	trans.setModel(b2g_transform(t));
	trans.setPos(pos);
	t = g2b_transform(trans.getModel());
	Body->setCenterOfMassTransform(t);
	Body->getMotionState()->setWorldTransform(t);
	if (resetLinVel)
		Body->setLinearVelocity(zeroes);
	if (resetAngVel)
		Body->setAngularVelocity(zeroes);
	if (resetForces)
		Body->clearForces();
}
void GkObject::setVel(glm::vec3 vel) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return;
	auto* Body = RigidBodies[0];
	btVector3 v = g2b_vec3(vel);
	Body->setLinearVelocity(v);
}
void GkObject::addVel(glm::vec3 vel) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return;
	auto* Body = RigidBodies[0];
	btVector3 v = Body->getLinearVelocity() + g2b_vec3(vel);
	Body->setLinearVelocity(v);
}
glm::vec3 GkObject::getVel() {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return glm::vec3(0);
	auto* Body = RigidBodies[0];
	
	return b2g_vec3(Body->getLinearVelocity());
}
glm::vec3 GkObject::getAngVel() {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return glm::vec3(0);
	auto* Body = RigidBodies[0];
	
	return b2g_vec3(Body->getAngularVelocity());
}
void GkObject::setAngVel(glm::vec3 vel) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return;
	auto* Body = RigidBodies[0];
	btVector3 v = g2b_vec3(vel);
	Body->setAngularVelocity(v);
}
void GkObject::setWorldTransform(Transform trans, bool apply_to_body, bool apply_to_motionstate, bool lockScale) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return;
	if (lockScale)
		trans.setScale(glm::vec3(1, 1, 1)); // Avoid trouble.
	btTransform t = g2b_transform(trans.getModel());
	auto* Body = RigidBodies[0];
	if (apply_to_body)
		Body->setWorldTransform(t);
	if (apply_to_motionstate)
		Body->getMotionState()->setWorldTransform(t);
}
Transform GkObject::getWorldTransform() {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr)
		return glm::mat4();
	btTransform t;
	RigidBodies[0]->getMotionState()->getWorldTransform(t);
	return Transform(b2g_transform(t));
}
glm::mat3x3 GkObject::applyCharacterControllerSpring(
	// THE SMART WAY TO DO CHARACTER CONTROLLERS!
	// Returns the distance from the closest ray hit, or -1 if there is no hit.
	glm::vec3 RelPos,		// Position in local space relative to the
	glm::vec3 RelDirection, // Direction from RelPos in local space.
	float k_pull,			// k value to use when the spring is pulling.
	float k_push,			// k value to use when the spring is pushing.
	float natural_length,
	float max_length, // Max Distance spring can "snap on" to things.
	btDiscreteDynamicsWorld* world, glm::vec3 dirfilter, float reactionFactor) {
	if (RigidBodies.size() < 1 || RigidBodies[0] == nullptr || !world || max_length <= 0 || natural_length <= 0) // Fail conditions
		return glm::mat3x3(-1, 0, 0, 
							0, 0, 0,
							0, 0, 0); //-1, force 0, friction 0, velocity 0 0 0
	//~ std::cout << "\nPassed the fail tests... " << std::endl;
	auto* Body = RigidBodies[0];
	Body->activate(true);
	btTransform t;
	Body->getMotionState()->getWorldTransform(t);
	Transform buddy;
	buddy.setModel(b2g_transform(t));
	RelPos += buddy.getPos(); // HA
	RelDirection = glm::normalize(RelDirection);
	RelDirection = max_length * RelDirection;
	// Do a ray test
	btVector3 p = g2b_vec3(RelPos);
	btVector3 d = g2b_vec3(glm::vec3(RelDirection.x, RelDirection.y, RelDirection.z));
	d += p;
	RelDirection = glm::normalize(RelDirection);
	btCollisionWorld::ClosestRayResultCallback RayCallback(p, d);
	// Avoid colliding with the player.
	RayCallback.m_collisionFilterGroup = COL_NORMAL;
	RayCallback.m_collisionFilterMask = COL_NORMAL;
	world->rayTest(p, d, RayCallback);

	if (!RayCallback.hasHit()) // nothing to do.
		return glm::mat3x3(glm::vec3(-1, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 0));
	//~ std::cout << "\nHIT! Distance: " << RayCallback.m_closestHitFraction *
	// max_length << std::endl;
	float x = RayCallback.m_closestHitFraction * max_length - natural_length;
	//~ float x = natural_length - RayCallback.m_closestHitFraction *
	// max_length;
	float kx = ((x < 0) ? k_push : k_pull) * x;
	//~ float kx = ((x<0)?k_pull:k_push) * x;
	RelDirection = RelDirection * dirfilter;
	RelDirection = glm::normalize(RelDirection);
	d = g2b_vec3(kx * RelDirection);
	Body->applyCentralImpulse(d);
	d *= -1 * reactionFactor; // Equal and opposite... sort of
	// Equal and opposite force on the body we're standing on....
	float GF = 0.5; // If we can't find the ground's friction... just make it 0.5
	glm::vec3 GV(0, 0, 0);
	glm::vec3 GN(0, 0, 0);
	btVector3 hitPoint = RayCallback.m_hitPointWorld;
	if (RayCallback.m_collisionObject && btRigidBody::upcast(RayCallback.m_collisionObject)) {
		//~ std::cout << "\nWe have a HIT!" << std::endl;
		(btRigidBody::upcast(RayCallback.m_collisionObject))->getMotionState()->getWorldTransform(t);
		(btRigidBody::upcast(RayCallback.m_collisionObject))->activate(true);
		btVector3 p_body = t.getOrigin();
		p -= p_body;
		hitPoint -= p_body;
		((btRigidBody*)RayCallback.m_collisionObject)->applyImpulse(d, p);
		GF = ((btRigidBody*)RayCallback.m_collisionObject)->getFriction();
		GN = b2g_vec3(RayCallback.m_hitNormalWorld);
		GV = b2g_vec3(((btRigidBody*)RayCallback.m_collisionObject)->getVelocityInLocalPoint(hitPoint));
	} else {
		std::cout << "\nWe have a hit but the upcast failed? Bruh moment rn" << std::endl;
	}
	return glm::mat3x3(
		glm::vec3(
			 RayCallback.m_closestHitFraction * max_length, // Distance
			 kx,											// Force
			 GF												// Ground Friction
		),
		GV, // Ground velocity
		GN //Ground Normal
	);
}


void GkObject::destruct() {
	if (!haveProperlyDeRegistered)
		std::cout << "\nWARNING! Haven't properly deregistered from GkEngine!"
				  << " EXPECT BAD THINGS TO HAPPEN" << std::endl;
	//~ for(int i = 0; i < MeshInstances.size(); i++) //Deregister Meshinstances
	//~ {
	//~ for(int j = 0; j < SharedMeshes.size(); j++)
	//~ SharedMeshes[j]->deregisterInstance(&MeshInstances[i]);
	//~ }
	//~ std::cout << "\nBefore OwnedShapes" << std::endl;
	//~ std::cout << "\nDeleting Owned Entity Components..." << std::endl;
	//~ std::cout << "\nThere are: " << OwnedEntityComponents.size() << " of them" << std::endl;
	for (auto*& item : OwnedEntityComponents) {
		if (item) {
			delete item;
			item = nullptr;
		}
	}
	//~ std::cout << "\nDone Deleting Owned Entity Components..." << std::endl;
	OwnedEntityComponents.clear();
	for (auto*& item : OwnedShapes) {
		// Test if this is a btBvhTriangleMeshShape
		if (item->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE) {
			//~ std::cout << "\nProperly deleting meshinterface..." << std::endl;
			delete ((btBvhTriangleMeshShape*)item)->getMeshInterface();
		}
		if (item)
			delete item;
	}
	//~ std::cout << "\nBefore OwnedMeshes" << std::endl;
	for (auto*& item : OwnedMeshes) {
		if (item)
			delete item;
	}
	for (auto*& item : OwnedTextures)
		delete item;
	OwnedTextures.clear();
	//~ std::cout << "\nBefore RigidBodies" << std::endl;
	for (auto*& item : RigidBodies) {
		if (item) {
			delete item->getMotionState(); // the bullet manual says to do this.
			delete item;
		}
	}
	for (auto*& item : Hitboxes) {
		if (item) {
			delete item->getMotionState(); // the bullet manual says to do this.
			delete item;
		}
	}
	//~ std::cout << "\nBefore Constraints" << std::endl;
	for (auto*& item : Constraints) {
		if (item)
			delete item;
	}
	//~ std::cout << "\nBefore MIs" << std::endl;
	for (auto*& item : MeshInstances) {
		if (item)
			delete item;
	}
	//~ std::cout << "\nBefore Anims" << std::endl;
	if (shouldDeleteAnimations && Animations) // if Animations exists and is an owned resource.
		delete Animations;
	OwnedMeshes.clear();
	SharedMeshes.clear();
	SharedTextures.clear();
	SharedCubeMaps.clear();
	MeshInstances.clear();
	OwnedShapes.clear();
	SharedShapes.clear();
	RigidBodies.clear();
	Hitboxes.clear();
	Constraints.clear();
	RelativeTransforms.clear();
}
// AdvancedResourceManager functions~~~~~~~~~~
AdvancedResourceManager::AdvancedResourceManager() {}
std::string AdvancedResourceManager::getTextFile(std::string name) {
	if (TextFiles.count(name))
		return TextFiles[name];

	TextFiles[name] = "";
	// Attempt to load the file
	std::ifstream file;
	file.open(name.c_str());
	if (file.good())
		TextFiles[name] = std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
	return TextFiles[name];
}

IndexedModel AdvancedResourceManager::getIndexedModelFromFile(std::string name) {
	//~ std::cout << "\n INDEXED MODEL FROM FILE:\n" << name << std::endl;
	if (Models.count(name))
		return *Models[name];
	// EXECUTE THIS IF THERE'S A BACKSLASH COMMAND
	// Supported commands
	// RGB here is floating point zero to one for r, g, and b.
	//\Sphere(rad, long_complex, lat_complex, r, g, b, texmap?)
	//\Box(x,y,z,r,g,b, texmap?)
	// We will cache the result into our models list because it's more than
	// likely That it will be called twice or more. Note that this design does
	// overrides anything before the backslash...
	if (tkCmd(name, "\\Sphere(", false, false)) {
		size_t pos = (name.find("\\Sphere(")) + 8; // Goes to first character past \\Sphere(
		if (pos > name.length()) {
			std::cout << "\nERROR parsing Sphere command. ";
		}
		//~ std::cout << "\nCaught a \\SPHERE line!\nLine: " << name <<
		// std::endl;
		std::string args = name.substr(pos, std::string::npos);
		auto tokens = SplitString(args, ',');
		float rad = 1, r = 0, g = 0, b = 0;
		int long_complex = 3, lat_complex = 3;
		uint texmap = 0;

		if (tokens.size() > 0)
			rad = GkAtof(tokens[0].c_str());
		if (tokens.size() > 1)
			long_complex = GkAtoi(tokens[1].c_str());
		if (tokens.size() > 2)
			lat_complex = GkAtoi(tokens[2].c_str());

		if (long_complex < 3 || lat_complex < 3) {
			std::cout << "\nERROR! Can't have longitudinal complexity less than 3. "
						 "Using 3 for both..."
					  << std::endl;
			long_complex = 3;
			lat_complex = 3;
		}

		if (tokens.size() > 3)
			r = GkAtof(tokens[3].c_str());
		if (tokens.size() > 4)
			g = GkAtof(tokens[4].c_str());
		if (tokens.size() > 5)
			b = GkAtof(tokens[5].c_str());
		if (tokens.size() > 6)
			texmap = GkAtoui(tokens[6].c_str());
		//~ if(1)
		//~ {
		IndexedModel br = createSphere(rad, long_complex, lat_complex, glm::vec3(r, g, b));
		br.validate();
		Models[name] = new IndexedModel(br);

		//~ std::cout << "\npositions: " << br.positions.size() <<
		//~ "\ntexCoords: " << br.texCoords.size() <<
		//~ "\nnormals: " << br.normals.size() <<
		//~ "\ncolors: " << br.colors.size() <<
		//~ "\nIndices: " << br.indices.size();
		//~ }

		if (Models[name] && !texmap) {
			(*Models[name]).hadRenderFlagsInFile = true;
			(*Models[name]).renderflags = GK_RENDER | GK_COLOR_IS_BASE | GK_COLORED;
		} else if (Models[name]) {
			(*Models[name]).hadRenderFlagsInFile = true;
			(*Models[name]).renderflags = GK_RENDER | GK_TEXTURED;
		}
		//~ std::cout << "\nReturning sphere..." << std::endl;
		return IndexedModel(*Models[name]);
	} else if (tkCmd(name, "\\Box(", false, false)) { // tkCmd
		size_t pos = (name.find("\\Box(")) + 5;
		//~ if (pos > name.length())
		//~ {
		//~ std::cout << "\nERROR parsing Box command. Not enough args.";
		//~ }

		std::string args = name.substr(pos, std::string::npos);
		auto tokens = SplitString(args, ',');
		float x = 1, y = 1, z = 1, tx = 1, ty = 1, tz = 1, r = 0, g = 0, b = 0;
		uint texmap = 0;

		if (tokens.size() > 0)
			x = GkAtof(tokens[0].c_str());
		if (tokens.size() > 1)
			y = GkAtof(tokens[1].c_str());
		if (tokens.size() > 2)
			z = GkAtof(tokens[2].c_str());

		if (tokens.size() > 3)
			r = GkAtof(tokens[3].c_str());
		if (tokens.size() > 4)
			g = GkAtof(tokens[4].c_str());
		if (tokens.size() > 5)
			b = GkAtof(tokens[5].c_str());

		if (tokens.size() > 6)
			texmap = GkAtoui(tokens[6].c_str());

		if (tokens.size() > 7)
			tx = GkAtof(tokens[7].c_str());
		if (tokens.size() > 8)
			ty = GkAtof(tokens[8].c_str());
		if (tokens.size() > 9)
			tz = GkAtof(tokens[9].c_str());
		IndexedModel br = createBox(x, y, z, glm::vec3(r, g, b), glm::vec3(tx, ty, tz));
		br.validate();
		Models[name] = new IndexedModel(br);
		if (Models[name] && !texmap) {
			(*Models[name]).hadRenderFlagsInFile = true;
			(*Models[name]).renderflags = GK_RENDER | GK_COLOR_IS_BASE | GK_COLORED;
		} else if (Models[name]) {
			(*Models[name]).hadRenderFlagsInFile = true;
			(*Models[name]).renderflags = GK_RENDER | GK_TEXTURED;
		}
		return IndexedModel(*Models[name]);
	} else if (tkCmd(name, "\\Cone(", false, false)) {
		//~ IndexedModel createCone(float radius, float h, unsigned int
		// numsubdiv, glm::vec3 color = glm::vec3(0,0,0)); //note: CoM is h/4 up
		// from the base
		size_t pos = (name.find("\\Cone(")) + 6;

		std::string args = name.substr(pos, std::string::npos);
		auto tokens = SplitString(args, ',');
		float rad = 1, h = 1, r = 0, g = 0, b = 0;
		uint subdiv = 10;
		if (tokens.size() > 0)
			rad = GkAtof(tokens[0].c_str());
		if (tokens.size() > 1)
			h = GkAtof(tokens[1].c_str());

		if (tokens.size() > 2)
			subdiv = GkAtoui(tokens[2].c_str());
		if (subdiv > 1000000)
			subdiv = 1000000; // Safety check. I think a million is a bit much.

		if (tokens.size() > 3)
			r = GkAtof(tokens[3].c_str());
		if (tokens.size() > 4)
			g = GkAtof(tokens[4].c_str());
		if (tokens.size() > 5)
			b = GkAtof(tokens[5].c_str());
		IndexedModel br = createCone(rad, h, subdiv, glm::vec3(r, g, b));
		br.validate();
		Models[name] = new IndexedModel(br);
		(*Models[name]).hadRenderFlagsInFile = true;
		(*Models[name]).renderflags = GK_RENDER | GK_COLOR_IS_BASE | GK_COLORED;
		return IndexedModel(*Models[name]);

	} else if (tkCmd(name, "\\Plane(", false, false)) {
		size_t pos = (name.find("\\Plane(")) + 7;
		std::string args = name.substr(pos, std::string::npos);
		auto tokens = SplitString(args, ',');
		float Xdim = 1, Zdim = 1;
		uint Xsub = 1, Zsub = 1;
		bool smooth = false;
		uint texmap = 0;
		float texScale = 1;
		glm::vec3 color(1, 0, 0);
		for (auto& token : tokens)
			std::cout << "TOKEN: " << token << std::endl;
		if (tokens.size() > 0)
			Xdim = GkAtof(tokens[0].c_str());
		if (tokens.size() > 1)
			Zdim = GkAtof(tokens[1].c_str());
		std::cout << "\nArgs is \"" << args << "\"" << std::endl;
		std::cout << "\nXDIM IS " << Xdim << std::endl;
		std::cout << "\nZDIM IS " << Zdim << std::endl;
		if (tokens.size() > 2)
			Xsub = GkAtoui(tokens[2].c_str());
		if (tokens.size() > 3)
			Zsub = GkAtoui(tokens[3].c_str());
		std::cout << "\nXSUB IS " << Xsub << std::endl;
		std::cout << "\nZSUB IS " << Zsub << std::endl;
		if (tokens.size() > 4)
			smooth = GkAtoui(tokens[4].c_str());
		if (tokens.size() > 5)
			texmap = GkAtoui(tokens[5].c_str());

		if (tokens.size() > 6)
			color.x = GkAtof(tokens[6].c_str());
		if (tokens.size() > 7)
			color.y = GkAtof(tokens[7].c_str());
		if (tokens.size() > 8)
			color.z = GkAtof(tokens[8].c_str());
		if (tokens.size() > 9)
			texScale = GkAtof(tokens[9].c_str());
		std::cout << "\nTEXSCALE IS " << texScale << std::endl;
		IndexedModel br = createPlane(Xdim, Zdim, Xsub, Zsub, smooth, color, texScale);
		br.validate();
		Models[name] = new IndexedModel(br);
		if (Models[name] && !texmap) {
			(*Models[name]).hadRenderFlagsInFile = true;
			(*Models[name]).renderflags = GK_RENDER | GK_COLOR_IS_BASE | GK_COLORED;
		} else if (Models[name]) {
			(*Models[name]).hadRenderFlagsInFile = true;
			(*Models[name]).renderflags = GK_RENDER | GK_TEXTURED;
		}
		return IndexedModel(*Models[name]);
	} else if (tkCmd(name, "\\Hmap(", false, false)) {
		size_t pos = (name.find("\\Hmap(")) + 6;
		std::string args = name.substr(pos, std::string::npos);
		auto tokens = SplitString(args, ',');
		float Xdim = 1;
		float Zdim = 1;
		uint Xsub = 1;
		uint Zsub = 1;
		bool smooth = false;
		uint texmap = 0;
		float yScale = 1.0;
		glm::vec3 color(1, 0, 0);
		std::string texFile = "";
		float texScale = 1;
		if (tokens.size() > 0)
			Xdim = GkAtof(tokens[0].c_str());
		if (tokens.size() > 1)
			Zdim = GkAtof(tokens[1].c_str());
		if (tokens.size() > 2)
			Xsub = GkAtoui(tokens[2].c_str());
		if (tokens.size() > 3)
			Zsub = GkAtoui(tokens[3].c_str());
		if (tokens.size() > 4)
			smooth = GkAtoui(tokens[4].c_str());
		if (tokens.size() > 5)
			texmap = GkAtoui(tokens[5].c_str());
		if (tokens.size() > 6)
			texFile = tokens[6];
		if (tokens.size() > 7)
			color.x = GkAtof(tokens[7].c_str());
		if (tokens.size() > 8)
			color.y = GkAtof(tokens[8].c_str());
		if (tokens.size() > 9)
			color.z = GkAtof(tokens[9].c_str());
		if (tokens.size() > 10)
			yScale = GkAtof(tokens[10].c_str());
		if (tokens.size() > 11)
			texScale = GkAtof(tokens[11].c_str());
		int w, h, nc;
		unsigned char* imgData = Texture::stbi_load_passthrough((char*)texFile.c_str(), &w, &h, &nc, 1);
		IndexedModel br;
		if (!imgData)
			br = createPlane(Xdim, Zdim, Xsub, Zsub, smooth, color, texScale);
		else {
			br = genHeightMap(imgData, w, h, 1, Xdim, Zdim, yScale, smooth, texScale);
			free(imgData);
			imgData = nullptr;
		}
		br.validate();
		Models[name] = new IndexedModel(br);
		if (Models[name] && !texmap) {
			(*Models[name]).hadRenderFlagsInFile = true;
			(*Models[name]).renderflags = GK_RENDER | GK_COLOR_IS_BASE | GK_COLORED;
		} else if (Models[name]) {
			(*Models[name]).hadRenderFlagsInFile = true;
			(*Models[name]).renderflags = GK_RENDER | GK_TEXTURED;
		}
		return IndexedModel(*Models[name]);
	}

	// DO THIS IF THERE'S NO BACKSLASH
	OBJModel mdl(name);
	IndexedModel br = mdl.toIndexedModel();
	Models[name] = new IndexedModel(br);
	IndexedModel returnval = *Models[name];
	return returnval;
}

Mesh* AdvancedResourceManager::getStaticMesh(std::string filename, bool recalcnormals = false) {
	std::string name = filename;
	if (recalcnormals)
		name = name + "_RECALC";
	else
		name = name + "_NOCALC";
	if (Meshes.count(name)) // If it exists, provide it.
		return Meshes[name];
	// See if we can grab the indexed model
	IndexedModel myModel = getIndexedModelFromFile(filename);
	if (recalcnormals) {
		for (size_t i = 0; i < myModel.normals.size(); i++)
			myModel.normals[i] = glm::vec3(0, 0, 0);
		myModel.calcNormals();
	}
	Meshes[name] = new Mesh(myModel, false, true, true);
	if (theScene)
		theScene->registerMesh(Meshes[name]);
	return Meshes[name];
}
Mesh* AdvancedResourceManager::getIndependentMesh(std::string filename, bool instanced, bool is_static, bool recalcnormals, glm::mat4 trans) {
	Mesh* returnval = nullptr;
	//~ if(1)
	//~ { //TODO: fix bug with creating spheres... it's broken for some reason
	//~ std::cout << "Getting an independent mesh (before copy)" << std::endl;
	//~ IndexedModel* myModel_Ptr = new
	// IndexedModel(getIndexedModelFromFile(filename)); ~ std::cout << "Getting
	// an independent mesh (after copy)" << std::endl;
	IndexedModel myModel = getIndexedModelFromFile(filename);
	myModel.applyTransform(trans);
	//~ std::cout << "positions.size: " << myModel.positions.size() <<
	// std::endl;
	if (recalcnormals) {
		for (size_t i = 0; i < myModel.normals.size(); i++)
			myModel.normals[i] = glm::vec3(0, 0, 0);
		myModel.calcNormals();
	}
	//~ std::cout << "\nBEFORE MAKING MESH" << std::endl;
	returnval = new Mesh(myModel, instanced, is_static, false);
	//~ std::cout << "\nAFTER MAKING MESH" << std::endl;
	//~ if(myModel_Ptr) delete myModel_Ptr;
	//~ std::cout << "\nAFTER DELETING MYMODEL_PTR" << std::endl;
	//~ }
	//~ std::cout << "\nIt seems to be all good my dude" << std::endl;
	return returnval;
}
Texture* AdvancedResourceManager::getTexture(std::string filename, bool transParent, GLenum filter, GLenum wrapmode, float anisotropic) {
	std::string name = filename;
	if (transParent)
		name = name + "_TRAN";
	else
		name = name + "_OPAQ";

	if (Textures.count(name))
		return Textures[name];
	Textures[name] = new Texture(filename, transParent, filter, filter, wrapmode, anisotropic);
	return Textures[name];
}
CubeMap* AdvancedResourceManager::getCubeMap(std::string FIRST, std::string SECOND, std::string THIRD, std::string FOURTH, std::string FIFTH,
											 std::string SIXTH) {
	// The exact same as CubeMap will make its own name.
	std::string name = FIRST + "|" + SECOND + "|" + THIRD + "|" + FOURTH + "|" + FIFTH + "|" + SIXTH + "|";
	if (OurCubeMaps.count(name))
		return OurCubeMaps[name];
	OurCubeMaps[name] = new CubeMap(FIRST, SECOND, THIRD, FOURTH, FIFTH, SIXTH);
	return OurCubeMaps[name];
}
ALuint AdvancedResourceManager::loadWAV(std::string filename) {
	if (audiobuffers.count(filename))
		return audiobuffers[filename];
	audiobuffers[filename] = loadWAVintoALBuffer(filename.c_str());
	return audiobuffers[filename];
}

GkObject* AdvancedResourceManager::loadObject(std::string filename, bool isTemplate, GkObject* Template, Transform initTransform) {
	GkObject* RetVal = nullptr;
	int error_flag = 0;
	
	IndexedModel lastModel;
	Mesh* lastMesh = nullptr;
	std::string lastAnimationName = "";
	bool RetValOwnsLastMesh = true;
	Texture* lastTexture = nullptr;
	CubeMap* lastCubeMap = nullptr;
	MeshInstance* lastMeshInstance = nullptr;
	btRigidBody* lastRigidBody = nullptr;
	btCollisionShape* lastCollisionShape = nullptr;
	std::vector<btCollisionShape*> ShapesInOrder;
	std::vector<Mesh*> MeshesInOrder; // All the owned and shared meshes in the
									  // order they appear in the file.
	std::vector<std::string> Defines;
	Defines.push_back("$True");
	Defines.push_back("1");
	Defines.push_back("$False");
	Defines.push_back("0");
	GkEngine* parent = (GkEngine*)myEngine;
	float Scaling = initTransform.getScale().x;
	initTransform.setScale(glm::vec3(Scaling, Scaling, Scaling)); // JUST TO MAKE SURE
	// Creation hints
	if (isTemplate) {
		if (TemplateObjects.count(filename))
			return TemplateObjects[filename];

		// LOADING A TEMPLATE

		// NEEDN'T CONSIDER ANYTHING THATS GONNA BE INDEPENDENT
		std::stringstream* file;
		std::string line;
		file = getTextFileStream(filename);
		if (!file->good()) {
			TemplateObjects[filename] = nullptr;
			return nullptr;
		}
		RetVal = new GkObject();
		RetVal->name = "DO NOT REGISTER!!! BAD JUJU!!!";
		RetVal->GkObjectFile = filename;
		RetVal->creationScale = glm::vec3(1, 1, 1);
		RetVal->myAssetMode = INDEPENDENT;
		RetVal->isNull = false;
		//~ std::cout << "\nLOADING TEMPLATE!" << std::endl;
		TemplateObjects[filename] = RetVal;
		while (file->good()) {
			std::getline(*file, line);

			unsigned int lineLength = line.length();
			// The vertical bar shall be used to separate items.
			std::vector<std::string> tokens = SplitString(line, '|');
			//~ std::cout << "LINE: \n" << line << std::endl;
			//~ for(int i = 0; i < tokens.size(); i++)
			//~ std::cout << "TOKEN #" << i  << ":" << tokens[i] << std::endl;
			if (line[0] == '#') // it's a comment.
				continue;
			if (tokens.size() == 0 || lineLength < 2)
				continue;
			//~ RetVal->Lines.push_back(line);
			if (tkCmd(tokens[0], "DEFINE")) {
				if (tokens.size() > 2) {

					size_t pos = tokens[1].find("$");
					if (pos != std::string::npos) { // Valid define.
						//~ std::cout << "\nVALID DEFINE CREATED!\nTag:\"" <<
						// tokens[1].substr(pos) << "\"" << ~ "\nRepl:\"" <<
						// tokens[2] <<
						//"\"" << std::endl;
						Defines.push_back(tokens[1].substr(pos));
						Defines.push_back(tokens[2]);
					}
				}
			} else if (Defines.size() > 0 && tokens.size() > 1) { // Apply defines
				for (size_t i = 1; i < tokens.size(); i++) {
					//~ std::cout << "Looping Through Tokens... " << i <<
					// std::endl;
					for (size_t def = 0; def + 1 < Defines.size(); def += 2) {
						//~ std::cout << "Looping through defines... " << def <<
						// std::endl;
						std::string& tag = Defines[def];
						std::string& replacement = Defines[def + 1];
						//~ std::cout << "\nsearching for Tag:" << tag <<
						//"\nRepl:" << replacement << std::endl;
						size_t pos = tokens[i].find(tag);
						while (pos != std::string::npos) {
							//~ std::cout << "\nVALID DEFINE REPLACEMENT
							// FOUND!\nTag:" << tag
							//<< "\nRepl:" << replacement << std::endl;
							tokens[i].replace(pos, tag.length(), replacement);
							pos = tokens[i].find(tag);
							//~ std::cout << "\nToken is now \"" << tokens[i] <<
							//"\""<< std::endl;
						}
					}
				}
			}

			if (parent && parent->GSGELineInterpreter(*file, line, tokens, isTemplate, Template, initTransform, RetVal, error_flag,
													  lastModel, lastMesh, lastAnimationName, RetValOwnsLastMesh, lastTexture, lastCubeMap, lastMeshInstance,
													  lastRigidBody, lastCollisionShape, ShapesInOrder, MeshesInOrder, Scaling)) {

			} else if (tkCmd(tokens[0], "SYNCMODE")) {
				if ((std::size_t)(tokens[1].find("NONE")) != std::string::npos)
					RetVal->mySyncMode = NO_SYNC;
				else if ((std::size_t)(tokens[1].find("ONE_TO_ONE")) != std::string::npos)
					RetVal->mySyncMode = SYNC_ONE_TO_ONE;
				else if ((std::size_t)(tokens[1].find("ANIMATED")) != std::string::npos)
					RetVal->mySyncMode = SYNC_ANIMATED;
				else if ((std::size_t)(tokens[1].find("SOFTBODY")) != std::string::npos)
					RetVal->mySyncMode = SYNC_SOFTBODY;
				//~ } else if (tkCmd(tokens[0], "DENSITY")) {
				//~ if(tokens.size() > 1)
				//~ RetVal->Density = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "DISABLECULLING")) {
				RetVal->disableCulling = true;
			} else if (tkCmd(tokens[0], "MESH")) {
				std::string filename = "|"; // Guaranteed not to be a in the token!
				int independence = 0;
				int instancing = 0;
				int static_shock = 1;
				int RecalculateNormals = 1;
				if (tokens.size() > 1) {
					//~ RetVal->MeshCreationLines.push_back(line);
					filename = tokens[1];
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					if (independence) // we don't need this mesh...
					{
						MeshesInOrder.push_back(nullptr);
						RetVal->OwnedMeshes.push_back(nullptr);
						lastMesh = nullptr;
						RetValOwnsLastMesh = false;
						continue;
					}
					if (tokens.size() > 3)
						static_shock = GkAtoi(tokens[3].c_str());
					if (tokens.size() > 4)
						instancing = GkAtoi(tokens[4].c_str());
					if (tokens.size() > 5)
						RecalculateNormals = GkAtoi(tokens[5].c_str());
					Transform bruh;
					bruh.setScale(initTransform.getScale());
					lastMesh = getIndependentMesh(filename, instancing, static_shock, RecalculateNormals, bruh.getModel());

					if (!lastMesh) {
						std::cout << "\nERROR LOADING MESH, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					RetValOwnsLastMesh = true;
					RetVal->OwnedMeshes.push_back(lastMesh);
					MeshesInOrder.push_back(lastMesh);
					// Add the mesh to theScene
					theScene->registerMesh(lastMesh);
				} else {
					std::cout << "\nERROR!!! too few args on Mesh line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MSHMASK")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->mesh_meshmask = GkAtoi(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_DIFF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1) {
					lastMesh->InstancedMaterial.diffusivity = GkAtof(tokens[1].c_str());
					//~ std::cout << "\nSet PHONG_I_DIFF!" << std::endl;
				}
			} else if (tkCmd(tokens[0], "PHONG_I_CUBE_REF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1) {
					//~ std::cout << "\nENABLED CUBEMAP REFLECTIONS FOR
					// INSTANCED MESH!!!"
					//<< std::endl;
					lastMesh->instanced_enable_cubemap_reflections = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_I_CUBE_DIFF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->instanced_enable_cubemap_diffusion = GkAtoi(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_AMB")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.ambient = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_SPECR")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.specreflectivity = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_SPECD")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.specdamp = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_EMISS")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.emissivity = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0],
							 "TEXTURE")) { // RetValOwnsLastMesh determines if we
										   // do anything for templated meshes btw
				std::string filename = "|";
				GLenum filtermode = GL_LINEAR;
				GLenum wrapmode = GL_REPEAT;
				int transParent = 0;
				float anisotropic = 4.0f;
				if (tokens.size() > 1) {
					//~ RetVal->TextureCreationLines.push_back(line);
					filename = tokens[1];
					if (tokens.size() > 2)
						transParent = GkAtoi(tokens[2].c_str());
					if (tokens.size() > 3) {
						if ((std::size_t)(tokens[3].find("LINEAR")) != std::string::npos)
							filtermode = GL_LINEAR;
						else if ((std::size_t)(tokens[3].find("NEAREST")) != std::string::npos)
							filtermode = GL_NEAREST;
					}
					if (tokens.size() > 4) {
						if ((std::size_t)(tokens[4].find("REPEAT")) != std::string::npos)
							wrapmode = GL_REPEAT;
						else if ((std::size_t)(tokens[4].find("CLAMP")) != std::string::npos)
							wrapmode = GL_CLAMP_TO_EDGE;
						else if ((std::size_t)(tokens[4].find("MIRRORED_REPEAT")) != std::string::npos)
							wrapmode = GL_MIRRORED_REPEAT;
					}
					if (tokens.size() > 5)
						anisotropic = GkAtof(tokens[5].c_str());
					lastTexture = getTexture(filename, transParent, filtermode, wrapmode, anisotropic);
					if (!lastTexture) {
						std::cout << "\nERROR LOADING TEXTURE, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					RetVal->SharedTextures.push_back(lastTexture);
					if (lastMesh)
						lastMesh->pushTexture(SafeTexture(lastTexture));
				} else {
					std::cout << "\nERROR!!! too few args on Tex line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0],
							 "CUBEMAP")) { // Same for cube lines (see above)
				if (tokens.size() > 6) {
					//~ RetVal->CubeMapCreationLines.push_back(line);
					//~ std::cout << "\nCreating Cubemap!" << std::endl;
					lastCubeMap = getCubeMap(tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6]);
					if (!lastCubeMap) {
						std::cout << "\nERROR LOADING CUBEMAP, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					RetVal->SharedCubeMaps.push_back(lastCubeMap);
					if (lastMesh) {
						lastMesh->pushCubeMap(lastCubeMap);
						//~ std::cout << "\nActually pushed cubemap." <<
						// std::endl;
					}
				} else {
					std::cout << "\nERROR!!! too few args on Cube line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "VOBJ")) { //Allow OBJ files to be embedded within a GSGE file.
				//VOBJ|vfile.obj
				if(tokens.size() < 2) continue;
				std::string vfilename = tokens[1];
				std::string vfilecontent = "";
				std::getline(*file, line);
				//~ std::cout << "\nSTARTING VIRTUAL OBJ FILE: " << vfilename << std::endl;
				while(!tkCmd(line, "ENDVOBJ") && file->good()){
					vfilecontent += line + "\n";
					std::getline(*file, line);
				}
				OBJModel model(vfilecontent, true);
				model.myFileName = vfilename;
				//If it already exists? Delete it.
				if(Models.count(vfilename)) delete Models[vfilename];
				Models[vfilename] = new IndexedModel(model.toIndexedModel());
			} else if (tkCmd(tokens[0], "CONVEXHULL")) {
				std::string filename = "|";
				int independence = 1;
				if (tokens.size() > 1) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					if (independence) {
						RetVal->OwnedShapes.push_back(nullptr);
						ShapesInOrder.push_back(nullptr);
						continue;
					}
					IndexedModel bruh = getIndexedModelFromFile(tokens[1]);
					if (initTransform.getScale() != glm::vec3(1, 1,
															  1)) { // Apply a scaling transformation to it.
						Transform trans;
						trans.setScale(initTransform.getScale());
						bruh.applyTransform(trans.getModel());
					}
					if (bruh.positions.size() < 4) {
						std::cout << "\nERROR!!! Either your file failed to "
									 "load, or it didn't "
									 "have enough positions upon loading (at "
									 "least 4).\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					lastCollisionShape = makeConvexHullShape(bruh);
					if (!lastCollisionShape) {
						std::cout << "\nERROR!!! Convex Hull Shape came out "
									 "null for some "
									 "reason!!!\nLine: "
								  << line << std::endl;
						delete lastCollisionShape; // Important step...
						error_flag = 1;
						break;
					}
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on ConvexHull line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "BOXSHAPE")) {
				btVector3 Dimensions = btVector3(1, 1, 1);
				int independence = 1;
				if (tokens.size() > 3) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					Dimensions.setX(GkAtof(tokens[1].c_str()));
					Dimensions.setY(GkAtof(tokens[2].c_str()));
					Dimensions.setZ(GkAtof(tokens[3].c_str()));
					if (tokens.size() > 4)
						independence = GkAtoi(tokens[4].c_str());
					if (independence) {
						RetVal->OwnedShapes.push_back(nullptr);
						ShapesInOrder.push_back(nullptr);
						continue;
					}
					lastCollisionShape = new btBoxShape(Dimensions);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on BoxShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "SPHERESHAPE")) {

				float radius = 1;
				int independence = 1;
				if (tokens.size() > 1) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					radius = GkAtof(tokens[1].c_str());
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					if (independence) {
						RetVal->OwnedShapes.push_back(nullptr);
						ShapesInOrder.push_back(nullptr);
						continue;
					}
					lastCollisionShape = new btSphereShape(radius);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on SphereShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "CONESHAPE")) {

				float radius = 1;
				float height = 1;
				int independence = 1;
				if (tokens.size() > 2) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					radius = GkAtof(tokens[1].c_str());
					height = GkAtof(tokens[2].c_str());
					if (tokens.size() > 3)
						independence = GkAtoi(tokens[3].c_str());
					if (independence) {
						RetVal->OwnedShapes.push_back(nullptr);
						ShapesInOrder.push_back(nullptr);
						continue;
					}
					lastCollisionShape = new btConeShape(radius, height);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on ConeShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "TRIMSHSHAPE")) {
				if (tokens.size() > 1) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					int independence = 1;
					if (tokens.size() > 2)
						independence = GkAtof(tokens[2].c_str());
					if (independence) // We needn't consider this shape...
					{
						RetVal->OwnedShapes.push_back(nullptr);
						ShapesInOrder.push_back(nullptr);
						continue;
					}
					IndexedModel bruh = getIndexedModelFromFile(tokens[1]);
					if (initTransform.getScale() != glm::vec3(1, 1,
															  1)) { // Apply a scaling transformation to it.
						Transform trans;
						trans.setScale(initTransform.getScale());
						bruh.applyTransform(trans.getModel());
					}
					if (bruh.indices.size() < 3) {
						std::cout << "\nERROR!!! TriMesh failed to load, or "
									 "had too few "
									 "indices. Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					btTriangleMesh* buddy = makeTriangleMesh(bruh);
					if (!buddy) {
						std::cout << "\nERROR!!! TriMesh came out as NULL! "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					lastCollisionShape = new btBvhTriangleMeshShape(buddy, false);
					((btBvhTriangleMeshShape*)lastCollisionShape)->buildOptimizedBvh();
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);
				} else {
					std::cout << "\nERROR!!! too few args on TriMeshShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "REQUIRES_EXPORT")) {
				RetVal->requiresExporting = true;
			} else if (tkCmd(tokens[0], "COLSOUND")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colsoundbuffer = loadWAV(tokens[1]);
					RetVal->colSoundFileName = tokens[1];
				}
			} else if (tkCmd(tokens[0], "MINCOLSOUNDIMPULSE")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinImpulse = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "LOUDNESS")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundLoudness = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "DISABLECOLSOUNDS")) {
				/*
				 * float colSoundMaxDistance = 1000.0f;
				float colSoundRolloffFactor = 1.0f;
				 *
				 * */
				RetVal->ColInfo.disableColSounds = true;
			} else if (tkCmd(tokens[0], "MAXDISTANCE")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxDistance = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "ROLLOFF")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundRolloffFactor = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MINPITCH")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinPitch = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MAXPITCH")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxPitch = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MINGAIN")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinGain = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MAXGAIN")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxGain = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "LOADSOUND")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colsoundbuffer = loadWAV(tokens[1]);
				}
			} else if (tkCmd(tokens[0], "ANIM")) {
				//~ RetVal->AnimCreationLines.push_back(line);
				if (!RetVal->Animations) {
					RetVal->Animations = new std::map<std::string, GkAnim>();
					RetVal->shouldDeleteAnimations = true;
				}
				std::string name = "DEFAULT";
				float timing = 0.0166666;
				if (tokens.size() > 1)
					name = tokens[1];
				if (tokens.size() > 2)
					timing = GkAtof(tokens[2].c_str());
				(*(RetVal->Animations))[name] = GkAnim(name, timing);
				lastAnimationName = name;
			} else if (tkCmd(tokens[0], "FRAME")) {

				if (RetVal->Animations->count(lastAnimationName)) {
					(*(RetVal->Animations))[lastAnimationName].addFrame();
					//~ RetVal->AnimCreationLines.push_back(line);
				} else {
					std::cout << "\nErroneous FRAME command..." << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MATRIX")) {
				if (tokens.size() > 17 && RetVal->Animations && RetVal->Animations->count(lastAnimationName) &&
					((*(RetVal->Animations))[lastAnimationName].Frames.size() > 0)) // 2-17 are the matrix.
				{
					//~ RetVal->AnimCreationLines.push_back(line);
					uint id = GkAtoui(tokens[1].c_str());
					glm::mat4 matrix;
					for (int i = 2; i < 18; i++) // Will process i = 2, Won't process i = 18
						matrix[(i - 2) / 4][(i - 2) % 4] = GkAtof(tokens[i].c_str());
					(*(RetVal->Animations))[lastAnimationName].setMat4((uint)((*(RetVal->Animations))[lastAnimationName].Frames.size() - 1), id, matrix);
				} else {
					std::cout << "ERROR!!! Either not enough args for matrix, no "
								 "animations, last animation does not exist, or "
								 "forgot "
								 "to call FRAME.\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "TRANSFORM")) {
				if (tokens.size() > 1 && RetVal->Animations && RetVal->Animations->count(lastAnimationName) &&
					((*(RetVal->Animations))[lastAnimationName].Frames.size() > 0)

				) {
					//~ RetVal->AnimCreationLines.push_back(line);
					uint id = GkAtoui(tokens[1].c_str());
					glm::vec3 pos(0, 0, 0), rot(0, 0, 0), scale(1, 1, 1);
					if (tokens.size() > 4) {
						pos.x = GkAtof(tokens[2].c_str());
						pos.y = GkAtof(tokens[3].c_str());
						pos.z = GkAtof(tokens[4].c_str());
					}
					if (tokens.size() > 7) {
						rot.x = GkAtof(tokens[5].c_str());
						rot.y = GkAtof(tokens[6].c_str());
						rot.z = GkAtof(tokens[7].c_str());
					}
					if (tokens.size() > 10) {
						scale.x = GkAtof(tokens[8].c_str());
						scale.y = GkAtof(tokens[9].c_str());
						scale.z = GkAtof(tokens[10].c_str());
					}

					(*(RetVal->Animations))[lastAnimationName].setMat4((uint)((*(RetVal->Animations))[lastAnimationName].Frames.size() - 1), id,
																	   (Transform(pos, rot, scale)).getModel());
				} else {
					std::cout << "ERROR!!! Either not enough args for transform, no "
								 "animations, last animation does not exist, or "
								 "forgot "
								 "to call FRAME.\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			}
			// TODO: Continue writing this parser.
		}
		if(RetVal->requiresExporting){
			RetVal->ExportString = file->str();
		}
		if (file)
			delete file;

	} else if (Template && !isTemplate) { // From Template. We always assume the template is
										  // correct, hasn't been fucked with.
		std::stringstream* file;
		std::string line;
		file = getTextFileStream(filename);
		// The below crap isn't necessary as we can use ShapesInOrder.size() and
		// MeshesInOrder.size() uint shapeticker = 0, meshticker = 0; //Shapes
		// and Meshes can be owned by the template.

		//~ std::cout << "\nCREATING FROM TEMPLATE!!!" << std::endl;

		if (!file->good()) {
			return nullptr;
		}
		RetVal = new GkObject();
		RetVal->GkObjectFile = filename;
		RetVal->creationScale = glm::vec3(1, 1, 1);
		RetVal->myAssetMode = TEMPLATED;
		RetVal->myTemplate = Template;
		RetVal->isNull = false;
		//~ std::cout << "\nUSING TEMPLATE TO MAKE NEW OBJECT!" << std::endl;
		while (file->good()) {
			std::getline(*file, line);
			unsigned int lineLength = line.length();
			// The vertical bar shall be used to separate items.
			std::vector<std::string> tokens = SplitString(line, '|');
			//~ std::cout << "LINE: \n" << line << std::endl;
			//~ for(int i = 0; i < tokens.size(); i++)
			//~ std::cout << "TOKEN #" << i  << ":" << tokens[i] << std::endl;
			if (line[0] == '#') // it's a comment.
				continue;
			if (tokens.size() == 0 || lineLength < 2)
				continue;

			//~ RetVal->Lines.push_back(line);
			if (tkCmd(tokens[0], "DEFINE")) {
				if (tokens.size() > 2) {

					size_t pos = tokens[1].find("$");
					if (pos != std::string::npos) { // Valid define.
						//~ std::cout << "\nVALID DEFINE CREATED!\nTag:\"" <<
						// tokens[1].substr(pos) << "\"" << ~ "\nRepl:\"" <<
						// tokens[2] <<
						//"\"" << std::endl;
						Defines.push_back(tokens[1].substr(pos));
						Defines.push_back(tokens[2]);
					}
				}
			} else if (Defines.size() > 0 && tokens.size() > 1) { // Apply defines
				for (size_t i = 1; i < tokens.size(); i++) {
					//~ std::cout << "Looping Through Tokens... " << i <<
					// std::endl;
					for (size_t def = 0; def + 1 < Defines.size(); def += 2) {
						//~ std::cout << "Looping through defines... " << def <<
						// std::endl;
						std::string& tag = Defines[def];
						std::string& replacement = Defines[def + 1];
						//~ std::cout << "\nsearching for Tag:" << tag <<
						//"\nRepl:" << replacement << std::endl;
						size_t pos = tokens[i].find(tag);
						while (pos != std::string::npos) {
							//~ std::cout << "\nVALID DEFINE REPLACEMENT
							// FOUND!\nTag:" << tag
							//<< "\nRepl:" << replacement << std::endl;
							tokens[i].replace(pos, tag.length(), replacement);
							pos = tokens[i].find(tag);
							//~ std::cout << "\nToken is now \"" << tokens[i] <<
							//"\""<< std::endl;
						}
					}
				}
			}
			// All possible line types.
			if (parent && parent->GSGELineInterpreter(*file, line, tokens, isTemplate, Template, initTransform, RetVal, error_flag, 
													  lastModel, lastMesh, lastAnimationName, RetValOwnsLastMesh, lastTexture, lastCubeMap, lastMeshInstance,
													  lastRigidBody, lastCollisionShape, ShapesInOrder, MeshesInOrder, Scaling)) {

			} else if (tkCmd(tokens[0], "SYNCMODE")) {
				if ((std::size_t)(tokens[1].find("NONE")) != std::string::npos)
					RetVal->mySyncMode = NO_SYNC;
				else if ((std::size_t)(tokens[1].find("ONE_TO_ONE")) != std::string::npos)
					RetVal->mySyncMode = SYNC_ONE_TO_ONE;
				else if ((std::size_t)(tokens[1].find("ANIMATED")) != std::string::npos)
					RetVal->mySyncMode = SYNC_ANIMATED;
				else if ((std::size_t)(tokens[1].find("SOFTBODY")) != std::string::npos)
					RetVal->mySyncMode = SYNC_SOFTBODY;
				//~ } else if (tkCmd(tokens[0], "DENSITY")) {
				//~ if(tokens.size() > 1)
				//~ RetVal->Density = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "DISABLECULLING")) {
				RetVal->disableCulling = true;
			} else if (tkCmd(tokens[0], "MESH")) {
				std::string filename = "|"; // Guaranteed not to be in the token!
				int independence = 0;
				int instancing = 0;
				int static_shock = 1;
				int RecalculateNormals = 1;
				if (tokens.size() > 1) {
					//~ RetVal->MeshCreationLines.push_back(line);
					filename = tokens[1];
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					if (!independence) // Just snag from the Template.
					{
						RetVal->SharedMeshes.push_back(Template->OwnedMeshes[MeshesInOrder.size()]);
						lastMesh = Template->OwnedMeshes[MeshesInOrder.size()];
						// lastMesh = nullptr;
						// We don't want to be modifying it... set ownslastmesh
						// to false
						MeshesInOrder.push_back(Template->OwnedMeshes[MeshesInOrder.size()]);
						RetValOwnsLastMesh = false;
						continue;
					}
					if (tokens.size() > 3)
						static_shock = GkAtoi(tokens[3].c_str());
					if (tokens.size() > 4)
						instancing = GkAtoi(tokens[4].c_str());
					if (tokens.size() > 5)
						RecalculateNormals = GkAtoi(tokens[5].c_str());
					Transform bruh;
					bruh.setScale(initTransform.getScale());
					lastMesh = getIndependentMesh(filename, instancing, static_shock, RecalculateNormals, bruh.getModel());
					if (!lastMesh) {
						std::cout << "\nERROR LOADING MESH, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					RetValOwnsLastMesh = true;
					RetVal->OwnedMeshes.push_back(lastMesh);
					MeshesInOrder.push_back(lastMesh);
				} else {
					std::cout << "\nERROR!!! too few args on Mesh line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MSHMASK")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->mesh_meshmask = GkAtoi(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_DIFF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1) {
					lastMesh->InstancedMaterial.diffusivity = GkAtof(tokens[1].c_str());
					//~ std::cout << "\nSet PHONG_I_DIFF!" << std::endl;
				}
			} else if (tkCmd(tokens[0], "PHONG_I_CUBE_REF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1) {
					//~ std::cout << "\nENABLED CUBEMAP REFLECTIONS FOR
					// INSTANCED MESH!!!"
					//<< std::endl;
					lastMesh->instanced_enable_cubemap_reflections = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_I_CUBE_DIFF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->instanced_enable_cubemap_diffusion = GkAtoi(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_AMB")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.ambient = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_SPECR")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.specreflectivity = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_SPECD")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.specdamp = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_EMISS")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.emissivity = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "TEXTURE") && RetValOwnsLastMesh && lastMesh) { // RetValOwnsLastMesh determines if we do
																						// anything for templated meshes btw
				std::string filename = "|";
				GLenum filtermode = GL_LINEAR;
				GLenum wrapmode = GL_REPEAT;
				int transParent = 0;
				float anisotropic = 4.0f;
				if (tokens.size() > 1) {
					//~ RetVal->TextureCreationLines.push_back(line);
					filename = tokens[1];
					if (tokens.size() > 2)
						transParent = GkAtoi(tokens[2].c_str());
					if (tokens.size() > 3) {
						if ((std::size_t)(tokens[3].find("LINEAR")) != std::string::npos)
							filtermode = GL_LINEAR;
						else if ((std::size_t)(tokens[3].find("NEAREST")) != std::string::npos)
							filtermode = GL_NEAREST;
					}
					if (tokens.size() > 4) {
						if ((std::size_t)(tokens[4].find("REPEAT")) != std::string::npos)
							wrapmode = GL_REPEAT;
						else if ((std::size_t)(tokens[4].find("CLAMP")) != std::string::npos)
							wrapmode = GL_CLAMP_TO_EDGE;
						else if ((std::size_t)(tokens[4].find("MIRRORED_REPEAT")) != std::string::npos)
							wrapmode = GL_MIRRORED_REPEAT;
					}
					if (tokens.size() > 5)
						anisotropic = GkAtof(tokens[5].c_str());
					lastTexture = getTexture(filename, transParent, filtermode, wrapmode, anisotropic);
					if (!lastTexture) {
						std::cout << "\nERROR LOADING TEXTURE, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					RetVal->SharedTextures.push_back(lastTexture);
					lastMesh->pushTexture(SafeTexture(lastTexture));
				} else {
					std::cout << "\nERROR!!! too few args on Tex line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "CUBEMAP") && lastMesh) { // Same for cube lines (see above)
				if (tokens.size() > 6) {
					//~ std::cout << "\nCreating Cubemap!" << std::endl;
					//~ RetVal->CubeMapCreationLines.push_back(line);
					lastCubeMap = getCubeMap(tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6]);
					if (!lastCubeMap) {
						std::cout << "\nERROR LOADING CUBEMAP, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					RetVal->SharedCubeMaps.push_back(lastCubeMap);
					if (lastMesh) {
						lastMesh->pushCubeMap(lastCubeMap);
						//~ std::cout << "\nActually pushed cubemap." <<
						// std::endl;
					}
				} else {
					std::cout << "\nERROR!!! too few args on Cube line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "VOBJ")) { //Allow OBJ files to be embedded within a GSGE file.
				//VOBJ|vfile.obj
				if(tokens.size() < 2) continue;
				//~ std::string vfilename = tokens[1];
				//~ std::string vfilecontent = "";
				std::getline(*file, line);
				//~ std::cout << "\nSTARTING VIRTUAL OBJ FILE: " << vfilename << std::endl;
				while(!tkCmd(line, "ENDVOBJ") && file->good()){
					//~ vfilecontent += line + "\n";
					std::getline(*file, line);
				}
				//~ OBJModel model(vfilecontent, true);
				//~ model.myFileName = vfilename;
				//If it already exists? Delete it.
				//~ if(Models.count(vfilename)) delete Models[vfilename];
				//~ Models[vfilename] = new IndexedModel(model.toIndexedModel());
			}else if (tkCmd(tokens[0], "CONVEXHULL")) {
				std::string filename = "|";
				int independence = 1;
				if (tokens.size() > 1) {
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					if (!independence) // Grab from the Template.
					{
						// A template is going to have all its shapes owned,
						// right?
						//...So they'll be in order
						// So we can just snag it
						RetVal->SharedShapes.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						ShapesInOrder.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						continue;
					}
					//~ std::cout << "\nUh oh" << std::endl;
					IndexedModel bruh = getIndexedModelFromFile(tokens[1]);
					if (initTransform.getScale() != glm::vec3(1, 1,
															  1)) { // Apply a scaling transformation to it.
						Transform trans;
						trans.setScale(initTransform.getScale());
						bruh.applyTransform(trans.getModel());
					}
					if (bruh.positions.size() < 4) {
						std::cout << "\nERROR!!! Either your file failed to "
									 "load, or it didn't "
									 "have enough positions upon loading (at "
									 "least 4).\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					lastCollisionShape = makeConvexHullShape(bruh);
					if (!lastCollisionShape) {
						std::cout << "\nERROR!!! Convex Hull Shape came out "
									 "null for some "
									 "reason!!!\nLine: "
								  << line << std::endl;
						delete lastCollisionShape; // Important step...
						error_flag = 1;
						break;
					}
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on ConvexHull line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "BOXSHAPE")) {
				btVector3 Dimensions = btVector3(1, 1, 1);
				int independence = 1;
				if (tokens.size() > 3) {
					Dimensions.setX(GkAtof(tokens[1].c_str()));
					Dimensions.setY(GkAtof(tokens[2].c_str()));
					Dimensions.setZ(GkAtof(tokens[3].c_str()));
					if (tokens.size() > 4)
						independence = GkAtoi(tokens[4].c_str());
					if (!independence) // Grab from the Template.
					{
						// A template is going to have all its shapes owned,
						// right?
						//...So they'll be in order
						// So we can just snag it
						RetVal->SharedShapes.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						ShapesInOrder.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						continue;
					}
					lastCollisionShape = new btBoxShape(Dimensions);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on BoxShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "SPHERESHAPE")) {

				float radius = 1;
				int independence = 1;
				if (tokens.size() > 1) {
					radius = GkAtof(tokens[1].c_str());
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					if (!independence) // Grab from the Template.
					{
						// A template is going to have all its shapes owned,
						// right?
						//...So they'll be in order
						// So we can just snag it
						RetVal->SharedShapes.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						ShapesInOrder.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						continue;
					}
					lastCollisionShape = new btSphereShape(radius);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on SphereShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "CONESHAPE")) {

				float radius = 1;
				float height = 1;
				int independence = 1;
				if (tokens.size() > 2) {
					radius = GkAtof(tokens[1].c_str());
					height = GkAtof(tokens[2].c_str());
					if (tokens.size() > 3)
						independence = GkAtoi(tokens[3].c_str());
					if (!independence) // Grab from the Template.
					{
						// A template is going to have all its shapes owned,
						// right?
						//...So they'll be in order
						// So we can just snag it
						RetVal->SharedShapes.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						ShapesInOrder.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						continue;
					}
					lastCollisionShape = new btConeShape(radius, height);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on ConeShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "TRIMSHSHAPE")) {
				if (tokens.size() > 1) {
					int independence = 1;
					if (tokens.size() > 2)
						independence = GkAtof(tokens[2].c_str());
					if (!independence) // Grab from the Template.
					{
						// A template is going to have all its shapes owned,
						// right?
						//...So they'll be in order
						// So we can just snag it
						RetVal->SharedShapes.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						ShapesInOrder.push_back(Template->OwnedShapes[ShapesInOrder.size()]);
						continue;
					}
					IndexedModel bruh = getIndexedModelFromFile(tokens[1]);
					if (initTransform.getScale() != glm::vec3(1, 1,
															  1)) { // Apply a scaling transformation to it.
						Transform trans;
						trans.setScale(initTransform.getScale());
						bruh.applyTransform(trans.getModel());
					}
					if (bruh.indices.size() < 3) {
						std::cout << "\nERROR!!! TriMesh failed to load, or "
									 "had too few "
									 "indices. Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					btTriangleMesh* buddy = makeTriangleMesh(bruh);
					if (!buddy) {
						std::cout << "\nERROR!!! TriMesh came out as NULL! "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					lastCollisionShape = new btBvhTriangleMeshShape(buddy, false);
					((btBvhTriangleMeshShape*)lastCollisionShape)->buildOptimizedBvh();
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);
				} else {
					std::cout << "\nERROR!!! too few args on TriMeshShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MSHINST")) {
				if (tokens.size() > 1) // We need the mesh at minimum...
				{
					
					lastMeshInstance = new MeshInstance();
					RetVal->MeshInstances.push_back(lastMeshInstance);
					
					
					// Instantiate the meshinstance
					for(uint i = 1; i < tokens.size(); i++){
						uint id = GkAtoui(tokens[i].c_str());
						if (id >= MeshesInOrder.size() || !MeshesInOrder[id]) {
							std::cout << "\nERROR! You can't register to a mesh "
										 "that doesn't "
										 "exist!\nLine: "
									  << line << std::endl;
							error_flag = 1;
							break;
						}
						MeshesInOrder[id]->registerInstance(lastMeshInstance);
					}
				} else {
					std::cout << "\nERROR!!! too few args on MeshInstance line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MI_TEXOFFSET")) {
				if(lastMeshInstance && tokens.size() > 2)
				{
					lastMeshInstance->texOffset.x = GkAtof(tokens[1].c_str());
					lastMeshInstance->texOffset.y = GkAtof(tokens[2].c_str());
				}
			} else if (tkCmd(tokens[0], "MSH_TEXOFFSET")) {
				if(lastMesh && tokens.size() > 2)
				{
					lastMesh->instanced_texOffset.x = GkAtof(tokens[1].c_str());
					lastMesh->instanced_texOffset.y = GkAtof(tokens[2].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_DIFF")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.diffusivity = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_AMB")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.ambient = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_SPECR")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.specreflectivity = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_SPECD")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.specdamp = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_EMISS")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.emissivity = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_TEX")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->tex = GkAtoui(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_CUBE")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->cubeMap = GkAtoui(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_DIFF_CUBE")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->EnableCubemapDiffusion = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_REFL_CUBE")) {
				if (lastMeshInstance && tokens.size() > 1) {
					//~ std::cout << "SET CUBEMAP REFLECTIONS ON
					// MESHINSTANCE!!!\n" << std::endl;
					lastMeshInstance->EnableCubemapReflections = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_MSHMASK")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->mymeshmask = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "HITBOX")) {
				// TODO code this
				uint shape = 0;
				if (tokens.size() > 1) {
					shape = GkAtoui(tokens[1].c_str());
					if (shape >= ShapesInOrder.size() || (ShapesInOrder[shape]->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)) {
						std::cout << "\nERROR!!! Either Shape ID is too high. (It "
									 "starts at ZERO) "
									 "or it's a compound shape. Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					btTransform t;
					t.setIdentity();
					btMotionState* motion = new btDefaultMotionState(t); // This will be owned by the rigid body we create.
					btVector3 inertia(0, 0, 0);

					btRigidBody::btRigidBodyConstructionInfo info(0, motion, ShapesInOrder[shape], inertia);
					RetVal->Hitboxes.push_back(new btRigidBody(info));
					btRigidBody* body = RetVal->Hitboxes.back();
					body->setUserPointer((void*)(&RetVal->ColInfo));
				}
			} else if (tkCmd(tokens[0], "BTRIGIDBODY")) {
				//#BTRIGIDBODY|SHAPE|MASS|INIT X|Y|Z|EULER
				// X|Y|Z|FRICTION|RESTITUTION|ROLLINGFRICTION|LINEARDAMPENING|ANGULARDAMPENING

				//~ std::cout << "\nMAKING A RIGID BODY FOR A TEMPLATED
				// GkObject!" << std::endl;
				uint shape = 0;
				float mass = 1;
				// btVector3 InitialXYZ = btVector3(0,0,0);
				glm::vec3 initxyz_glm = glm::vec3(0, 0, 0);
				glm::vec3 initEuler_glm = glm::vec3(0, 0, 0);
				float friction = 0.5;
				float restitution = 0;
				float rollingfriction = 0;
				float lineardamp = 0;
				float angdamp = 0;
				if (tokens.size() > 1) {
					shape = GkAtoui(tokens[1].c_str());
					if (tokens.size() > 2)
						mass = GkAtof(tokens[2].c_str());
					if (tokens.size() > 5) {
						//~ InitialXYZ.setX(GkAtof(tokens[3].c_str()));
						//~ InitialXYZ.setY(GkAtof(tokens[4].c_str()));
						//~ InitialXYZ.setZ(GkAtof(tokens[5].c_str()));
						initxyz_glm.x = GkAtof(tokens[3].c_str());
						initxyz_glm.y = GkAtof(tokens[4].c_str());
						initxyz_glm.z = GkAtof(tokens[5].c_str());
					}
					if (tokens.size() > 8) {
						//~ InitialXYZ.setX(GkAtof(tokens[3].c_str()));
						//~ InitialXYZ.setY(GkAtof(tokens[4].c_str()));
						//~ InitialXYZ.setZ(GkAtof(tokens[5].c_str()));
						initEuler_glm.x = GkAtof(tokens[6].c_str());
						initEuler_glm.y = GkAtof(tokens[7].c_str());
						initEuler_glm.z = GkAtof(tokens[8].c_str());
					}
					Transform localTransform = Transform(initxyz_glm, initEuler_glm, glm::vec3(1, 1,
																							   1)); // Position it correctly in 3d space
					localTransform.setModel(initTransform.getModel() * localTransform.getModel());
					localTransform.setScale(glm::vec3(1, 1, 1));
					if (tokens.size() > 9)
						friction = GkAtof(tokens[9].c_str());
					if (tokens.size() > 10)
						restitution = GkAtof(tokens[10].c_str());
					if (tokens.size() > 11)
						rollingfriction = GkAtof(tokens[11].c_str());
					if (tokens.size() > 12)
						lineardamp = GkAtof(tokens[12].c_str());
					if (tokens.size() > 13)
						angdamp = GkAtof(tokens[13].c_str());
					if (shape >= ShapesInOrder.size()) {
						std::cout << "\nERROR!!! Shape ID is too high. It "
									 "starts at ZERO, "
									 "Integer value. Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					btTransform t = g2b_transform(localTransform.getModel());
					btMotionState* motion = new btDefaultMotionState(t); // This will be owned by the rigid body we create.
					btVector3 inertia(0, 0, 0);
					if (mass != 0.0) {
						ShapesInOrder[shape]->calculateLocalInertia(mass, inertia);
					}
					btRigidBody::btRigidBodyConstructionInfo info(mass, motion, ShapesInOrder[shape], inertia);
					info.m_friction = friction;
					info.m_restitution = restitution;
					info.m_rollingFriction = rollingfriction;
					info.m_linearDamping = lineardamp;
					info.m_angularDamping = angdamp;
					RetVal->RigidBodies.push_back(new btRigidBody(info));
					btRigidBody* body = RetVal->RigidBodies.back();
					body->setUserPointer((void*)(&RetVal->ColInfo));
				} else {
					std::cout << "\nERROR!!! too few args on btRigidBody line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MAKECHARCONTROLLER")) {
				RetVal->makeCharacterController();
				// to be developed.
			} else if (tkCmd(tokens[0], "REQUIRES_EXPORT")) {
				RetVal->requiresExporting = true;
			} else if (tkCmd(tokens[0], "COLSOUND")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colsoundbuffer = loadWAV(tokens[1]);
					RetVal->colSoundFileName = tokens[1];
				}
			} else if (tkCmd(tokens[0], "MINCOLSOUNDIMPULSE")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinImpulse = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "LOUDNESS")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundLoudness = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "DISABLECOLSOUNDS")) {
				RetVal->ColInfo.disableColSounds = true;
			} else if (tkCmd(tokens[0], "MAXDISTANCE")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxDistance = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "ROLLOFF")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundRolloffFactor = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MINPITCH")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinPitch = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MAXPITCH")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxPitch = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MINGAIN")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinGain = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MAXGAIN")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxGain = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "LOADSOUND")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					loadWAV(tokens[1]);
				}
			} else if (tkCmd(tokens[0], "ANIM")) {
				if (!RetVal->Animations) {
					RetVal->Animations = Template->Animations;
					RetVal->shouldDeleteAnimations = false;
				}
				//~ RetVal->AnimCreationLines.push_back(line);
			} else if (tkCmd(tokens[0], "FRAME")) {
				//~ RetVal->AnimCreationLines.push_back(line);
			} else if (tkCmd(tokens[0], "MATRIX")) {
				//~ RetVal->AnimCreationLines.push_back(line);
			} else if (tkCmd(tokens[0], "TRANSFORM")) {
				//~ RetVal->AnimCreationLines.push_back(line);
			}
			// TODO: Re-package for GkEngine's serializer later.
			else if (tkCmd(tokens[0], "FIXEDCONSTRAINT")) {
				if (tokens.size() < 3)
					continue;   // Needs at least the IDs.
				uint BodyA = 0; // arg 1
				uint BodyB = 0; // arg 2
				btTransform tia;
				btTransform tib;
				btTransform btrans;
				btTransform atrans;
				float BreakingImpulse = 0; // arg 3
				btFixedConstraint* b = nullptr;

				BodyA = GkAtoui(tokens[1].c_str());
				BodyB = GkAtoui(tokens[2].c_str());
				tia.setIdentity();
				tib.setIdentity();
				if (tokens.size() > 3)
					BreakingImpulse = GkAtof(tokens[3].c_str());
				if (BodyA >= RetVal->RigidBodies.size() || BodyB >= RetVal->RigidBodies.size() || BodyA == BodyB) {
					std::cout << "\nERROR! Bad Constraint line. Bad IDs. Aborting... Line: " << line << std::endl;
					error_flag = 1;
					break;
				}
				RetVal->RigidBodies[BodyA]->getMotionState()->getWorldTransform(atrans);
				RetVal->RigidBodies[BodyB]->getMotionState()->getWorldTransform(btrans);

				tib = btrans.inverse() * atrans;
				//~ tia = atrans.inverse() * btrans;

				b = new btFixedConstraint(*(RetVal->RigidBodies[BodyA]), *(RetVal->RigidBodies[BodyB]), tia, tib);
				b->setBreakingImpulseThreshold(BreakingImpulse);
				RetVal->Constraints.push_back(b);
			} else if (tkCmd(tokens[0], "POINT2POINTCONSTRAINT")) {
				if (tokens.size() < 3)
					continue;   // Needs at least the IDs.
				uint BodyA = 0; // arg 1
				uint BodyB = 0; // arg 2
				btVector3 pa;
				btVector3 pb;
				float BreakingImpulse = 0; // arg 9
				btPoint2PointConstraint* b = nullptr;

				BodyA = GkAtoui(tokens[1].c_str());
				BodyB = GkAtoui(tokens[2].c_str());
				if (tokens.size() > 5) {
					pa.setX(GkAtof(tokens[3].c_str()));
					pa.setY(GkAtof(tokens[4].c_str()));
					pa.setZ(GkAtof(tokens[5].c_str()));
				}
				if (tokens.size() > 8) {
					pb.setX(GkAtof(tokens[6].c_str()));
					pb.setY(GkAtof(tokens[7].c_str()));
					pb.setZ(GkAtof(tokens[8].c_str()));
				}
				if (tokens.size() > 9)
					BreakingImpulse = GkAtof(tokens[9].c_str());
				if (BodyA >= RetVal->RigidBodies.size() || BodyB >= RetVal->RigidBodies.size() || BodyA == BodyB) {
					std::cout << "\nERROR! Bad Constraint line. Bad IDs. Aborting... Line: " << line << std::endl;
					error_flag = 1;
					break;
				}
				b = new btPoint2PointConstraint(*(RetVal->RigidBodies[BodyA]), *(RetVal->RigidBodies[BodyB]), pa, pb);
				b->setBreakingImpulseThreshold(BreakingImpulse);
				RetVal->Constraints.push_back(b);
			}
			//~ else if (tkCmd(tokens[0], "6DOFCONSTRAINT")) {
			//~ if (tokens.size() < 15) {
			//~ std::cout << "\nERROR! Too few args for 6DOFConstraint. Line: " << line << std::endl;

			//~ continue;
			//~ }							 // Needs EVERYTHING
			//~ uint BodyA = 0;				 // arg 1
			//~ uint BodyB = 0;				 // arg 2
			//~ glm::mat4 TransformInA(1.0); // arg 3 - 8
			//~ btTransform tia;			 // After conversion to bullet type.
			//~ glm::mat4 TransformInB(1.0); // arg 9 - 14
			//~ btTransform tib;			 // ditto
			//~ glm::vec3 grabber;
			//~ Transform helper;
			//~ glm::vec3 LinLow(0); // arg 15,16,17
			//~ glm::vec3 LinHi(0);
			//~ glm::vec3 AngLow(0);
			//~ glm::vec3 AngHi(0);
			//~ glm::vec3 LinStiff(-1);
			//~ glm::vec3 AngStiff(-1);
			//~ glm::vec3 LinDamp(0);
			//~ glm::vec3 AngDamp(0);
			//~ float BreakingImpulse = 0;
			//~ btGeneric6DofSpring2Constraint* b = nullptr;

			//~ BodyA = GkAtoui(tokens[1].c_str());
			//~ BodyB = GkAtoui(tokens[2].c_str());
			//~ grabber.x = (GkAtof(tokens[3].c_str()));
			//~ grabber.y = (GkAtof(tokens[4].c_str()));
			//~ grabber.z = (GkAtof(tokens[5].c_str()));
			//~ helper.setPos(grabber);
			//~ grabber.x = (GkAtof(tokens[6].c_str()));
			//~ grabber.y = (GkAtof(tokens[7].c_str()));
			//~ grabber.z = (GkAtof(tokens[8].c_str()));
			//~ helper.setRot(grabber);
			//~ tia = g2b_transform(helper.getModel());
			//~ grabber.x = (GkAtof(tokens[9].c_str()));
			//~ grabber.y = (GkAtof(tokens[10].c_str()));
			//~ grabber.z = (GkAtof(tokens[11].c_str()));
			//~ helper.setPos(grabber);
			//~ grabber.x = (GkAtof(tokens[12].c_str()));
			//~ grabber.y = (GkAtof(tokens[13].c_str()));
			//~ grabber.z = (GkAtof(tokens[14].c_str()));
			//~ helper.setRot(grabber);
			//~ tib = g2b_transform(helper.getModel());
			//~ if (tokens.size() > 26) {
			//~ LinLow.x = (GkAtof(tokens[15].c_str()));
			//~ LinLow.y = (GkAtof(tokens[16].c_str()));
			//~ LinLow.z = (GkAtof(tokens[17].c_str()));
			//~ LinHi.x = (GkAtof(tokens[18].c_str()));
			//~ LinHi.y = (GkAtof(tokens[19].c_str()));
			//~ LinHi.z = (GkAtof(tokens[20].c_str()));
			//~ AngLow.x = (GkAtof(tokens[21].c_str()));
			//~ AngLow.y = (GkAtof(tokens[22].c_str()));
			//~ AngLow.z = (GkAtof(tokens[23].c_str()));
			//~ AngHi.x = (GkAtof(tokens[24].c_str()));
			//~ AngHi.y = (GkAtof(tokens[25].c_str()));
			//~ AngHi.z = (GkAtof(tokens[26].c_str()));
			//~ }

			//~ // Spring properties. Negative stiffness means disabled spring.
			//~ if (tokens.size() > 38) {
			//~ LinStiff.x = (GkAtof(tokens[27].c_str()));
			//~ LinStiff.y = (GkAtof(tokens[28].c_str()));
			//~ LinStiff.z = (GkAtof(tokens[29].c_str()));

			//~ AngStiff.x = (GkAtof(tokens[30].c_str()));
			//~ AngStiff.y = (GkAtof(tokens[31].c_str()));
			//~ AngStiff.z = (GkAtof(tokens[32].c_str()));

			//~ LinDamp.x = (GkAtof(tokens[33].c_str()));
			//~ LinDamp.y = (GkAtof(tokens[34].c_str()));
			//~ LinDamp.z = (GkAtof(tokens[35].c_str()));

			//~ AngDamp.x = (GkAtof(tokens[36].c_str()));
			//~ AngDamp.y = (GkAtof(tokens[37].c_str()));
			//~ AngDamp.z = (GkAtof(tokens[38].c_str()));
			//~ }

			//~ if (tokens.size() > 39)
			//~ BreakingImpulse = GkAtof(tokens[39].c_str());

			//~ if (BodyA >= RetVal->RigidBodies.size() || BodyB >= RetVal->RigidBodies.size() || BodyA == BodyB) {
			//~ std::cout << "\nERROR! Bad Constraint line. Bad IDs. Aborting... Line: " << line << std::endl;
			//~ error_flag = 1;
			//~ break;
			//~ }
			//~ b = new btGeneric6DofSpring2Constraint(*(RetVal->RigidBodies[BodyA]), *(RetVal->RigidBodies[BodyB]), tia, tib);
			//~ // Set limits
			//~ b->setLimit(0, LinLow.x, LinHi.x);
			//~ b->setLimit(1, LinLow.y, LinHi.y);
			//~ b->setLimit(2, LinLow.z, LinHi.z);
			//~ b->setLimit(3, AngLow.x, AngHi.x);
			//~ b->setLimit(4, AngLow.y, AngHi.y);
			//~ b->setLimit(5, AngLow.z, AngHi.z);
			//~ b->enableSpring(0, (LinStiff.x >= 0));
			//~ b->enableSpring(1, (LinStiff.y >= 0));
			//~ b->enableSpring(2, (LinStiff.z >= 0));
			//~ b->enableSpring(3, (AngStiff.x >= 0));
			//~ b->enableSpring(4, (AngStiff.y >= 0));
			//~ b->enableSpring(5, (AngStiff.z >= 0));

			//~ b->setStiffness(0, (LinStiff.x));
			//~ b->setStiffness(1, (LinStiff.y));
			//~ b->setStiffness(2, (LinStiff.z));
			//~ b->setStiffness(3, (AngStiff.x));
			//~ b->setStiffness(4, (AngStiff.y));
			//~ b->setStiffness(5, (AngStiff.z));

			//~ b->setDamping(0, (LinDamp.x));
			//~ b->setDamping(1, (LinDamp.y));
			//~ b->setDamping(2, (LinDamp.z));
			//~ b->setDamping(3, (AngDamp.x));
			//~ b->setDamping(4, (AngDamp.y));
			//~ b->setDamping(5, (AngDamp.z));
			//~ b->setBreakingImpulseThreshold(BreakingImpulse);
			//~ RetVal->Constraints.push_back(b);
			//~ }
			// TODO: Continue writing this parser.
		}
		if (RetVal->Animations && RetVal->Animations->count("DEFAULT")) {
			RetVal->setAnimation("DEFAULT", 0.0);
			//~ std::cout << "\nSetting Default Anim...";
		}
		if(RetVal->requiresExporting){
			RetVal->ExportString = file->str();
		}
		if (file)
			delete file;
	} else if (!Template && !isTemplate) { // Independent... but can still use
										   // shit from Resources
		std::stringstream* file;
		std::string line;
		file = getTextFileStream(filename);
		if (!file->good()) {
			return nullptr;
		}
		RetVal = new GkObject();
		RetVal->GkObjectFile = filename;
		RetVal->creationScale = glm::vec3(Scaling, Scaling, Scaling);
		RetVal->myAssetMode = INDEPENDENT;
		RetVal->isNull = false;

		while (file->good()) {
			std::getline(*file, line);

			unsigned int lineLength = line.length();
			// The vertical bar shall be used to separate items.
			std::vector<std::string> tokens = SplitString(line, '|');
			//~ std::cout << "LINE: \n" << line << std::endl;
			//~ for(int i = 0; i < tokens.size(); i++)
			//~ std::cout << "TOKEN #" << i  << ":" << tokens[i] << std::endl;
			if (line[0] == '#') // it's a comment.
				continue;
			if (tokens.size() == 0 || lineLength < 2)
				continue;

			//~ RetVal->Lines.push_back(line);
			if (tkCmd(tokens[0], "DEFINE")) {
				if (tokens.size() > 2) {

					size_t pos = tokens[1].find("$");
					if (pos != std::string::npos) { // Valid define.
						//~ std::cout << "\nVALID DEFINE CREATED!\nTag:\"" <<
						// tokens[1].substr(pos) << "\"" << ~ "\nRepl:\"" <<
						// tokens[2] <<
						//"\"" << std::endl;
						Defines.push_back(tokens[1].substr(pos));
						Defines.push_back(tokens[2]);
					}
				}
			} else if (Defines.size() > 0 && tokens.size() > 1) { // Apply defines
				for (size_t i = 1; i < tokens.size(); i++) {
					//~ std::cout << "Looping Through Tokens... " << i <<
					// std::endl;
					for (size_t def = 0; def + 1 < Defines.size(); def += 2) {
						//~ std::cout << "Looping through defines... " << def <<
						// std::endl;
						std::string& tag = Defines[def];
						std::string& replacement = Defines[def + 1];
						//~ std::cout << "\nsearching for Tag:" << tag <<
						//"\nRepl:" << replacement << std::endl;

						size_t pos = tokens[i].find(tag);
						while (pos != std::string::npos) {
							//~ std::cout << "\nVALID DEFINE REPLACEMENT
							// FOUND!\nTag:" << tag
							//<< "\nRepl:" << replacement << std::endl;
							tokens[i].replace(pos, tag.length(), replacement);
							pos = tokens[i].find(tag);
							//~ std::cout << "\nToken is now \"" << tokens[i] <<
							//"\""<< std::endl;
						}
					}
				}
			}
			// All possible line types.
			if (parent && parent->GSGELineInterpreter(*file, line, tokens, isTemplate, Template, initTransform, RetVal, error_flag, 
													  lastModel, lastMesh, lastAnimationName, RetValOwnsLastMesh, lastTexture, lastCubeMap, lastMeshInstance,
													  lastRigidBody, lastCollisionShape, ShapesInOrder, MeshesInOrder, Scaling)) {

			} else if (tkCmd(tokens[0], "SYNCMODE")) {
				if ((std::size_t)(tokens[1].find("NONE")) != std::string::npos)
					RetVal->mySyncMode = NO_SYNC;
				else if ((std::size_t)(tokens[1].find("ONE_TO_ONE")) != std::string::npos)
					RetVal->mySyncMode = SYNC_ONE_TO_ONE;
				else if ((std::size_t)(tokens[1].find("ANIMATED")) != std::string::npos)
					RetVal->mySyncMode = SYNC_ANIMATED;
				else if ((std::size_t)(tokens[1].find("SOFTBODY")) != std::string::npos)
					RetVal->mySyncMode = SYNC_SOFTBODY;
				//~ } else if (tkCmd(tokens[0], "DENSITY")) {
				//~ if(tokens.size() > 1)
				//~ RetVal->Density = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "DISABLECULLING")) {
				RetVal->disableCulling = true;
			} else if (tkCmd(tokens[0], "MESH")) {
				std::string filename = "|"; // Guaranteed not to be in the token!
				int independence = 0;
				int instancing = 0;
				int static_shock = 1;
				int RecalculateNormals = 1;
				if (tokens.size() > 1) {
					//~ RetVal->MeshCreationLines.push_back(line);
					filename = tokens[1];
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					if (tokens.size() > 3)
						static_shock = GkAtoi(tokens[3].c_str());
					if (tokens.size() > 4)
						instancing = GkAtoi(tokens[4].c_str());
					if (tokens.size() > 5)
						RecalculateNormals = GkAtoi(tokens[5].c_str());

					Transform bruh;
					bruh.setScale(glm::vec3(Scaling, Scaling, Scaling));
					//~ std::cout << "\nGetting Ind Mesh..." << std::endl;
					lastMesh = getIndependentMesh(filename, instancing, static_shock, RecalculateNormals, bruh.getModel());
					//~ std::cout << "\nBack to making our independent mesh..."
					//<< std::endl;
					if (!lastMesh) {
						std::cout << "\nERROR LOADING MESH, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << "\nFilename: \"" << filename << "\"" << std::endl;
						error_flag = 1;
						break;
					}
					//~ std::cout << "\nGot him!" << std::endl;
					RetValOwnsLastMesh = true;
					RetVal->OwnedMeshes.push_back(lastMesh);
					MeshesInOrder.push_back(lastMesh);
				} else {
					std::cout << "\nERROR!!! too few args on Mesh line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MSHMASK")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->mesh_meshmask = GkAtoi(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_DIFF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1) {
					lastMesh->InstancedMaterial.diffusivity = GkAtof(tokens[1].c_str());
					//~ std::cout << "\nSet PHONG_I_DIFF!" << std::endl;
				}
			} else if (tkCmd(tokens[0], "PHONG_I_CUBE_REF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1) {
					//~ std::cout << "\nENABLED CUBEMAP REFLECTIONS FOR
					// INSTANCED MESH!!!"
					//<< std::endl;
					lastMesh->instanced_enable_cubemap_reflections = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_I_CUBE_DIFF")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->instanced_enable_cubemap_diffusion = GkAtoi(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_AMB")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.ambient = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_SPECR")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.specreflectivity = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_SPECD")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.specdamp = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0], "PHONG_I_EMISS")) {
				if (RetValOwnsLastMesh && lastMesh && tokens.size() > 1)
					lastMesh->InstancedMaterial.emissivity = GkAtof(tokens[1].c_str());
			} else if (tkCmd(tokens[0],
							 "TEXTURE")) { // RetValOwnsLastMesh determines if we
										   // do anything for templated meshes btw
				std::string filename = "|";
				GLenum filtermode = GL_LINEAR;
				GLenum wrapmode = GL_REPEAT;
				int transParent = 0;
				float anisotropic = 4.0f;
				if (tokens.size() > 1) {
					//~ RetVal->TextureCreationLines.push_back(line);
					filename = tokens[1];
					if (tokens.size() > 2)
						transParent = GkAtoi(tokens[2].c_str());
					if (tokens.size() > 3) {
						if ((std::size_t)(tokens[3].find("LINEAR")) != std::string::npos)
							filtermode = GL_LINEAR;
						else if ((std::size_t)(tokens[3].find("NEAREST")) != std::string::npos)
							filtermode = GL_NEAREST;
					}
					if (tokens.size() > 4) {
						if ((std::size_t)(tokens[4].find("REPEAT")) != std::string::npos)
							wrapmode = GL_REPEAT;
						else if ((std::size_t)(tokens[4].find("CLAMP")) != std::string::npos)
							wrapmode = GL_CLAMP_TO_EDGE;
						else if ((std::size_t)(tokens[4].find("MIRRORED_REPEAT")) != std::string::npos)
							wrapmode = GL_MIRRORED_REPEAT;
					}
					if (tokens.size() > 5)
						anisotropic = GkAtof(tokens[5].c_str());
					lastTexture = getTexture(filename, transParent, filtermode, wrapmode, anisotropic);
					if (!lastTexture) {
						std::cout << "\nERROR LOADING TEXTURE, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					RetVal->SharedTextures.push_back(lastTexture);
					if (lastMesh)
						lastMesh->pushTexture(SafeTexture(lastTexture));
				} else {
					std::cout << "\nERROR!!! too few args on Tex line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0],
							 "CUBEMAP")) { // Same for cube lines (see above)
				if (tokens.size() > 6) {
					//~ std::cout << "\nCreating Cubemap!" << std::endl;
					//~ RetVal->CubeMapCreationLines.push_back(line);
					lastCubeMap = getCubeMap(tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6]);
					if (!lastCubeMap) {
						std::cout << "\nERROR LOADING CUBEMAP, GOT NULL. "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					RetVal->SharedCubeMaps.push_back(lastCubeMap);
					if (lastMesh) {
						lastMesh->pushCubeMap(lastCubeMap);
						//~ std::cout << "\nActually pushed cubemap." <<
						// std::endl;
					}
				} else {
					std::cout << "\nERROR!!! too few args on Cube line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "VOBJ")) { //Allow OBJ files to be embedded within a GSGE file.
				//VOBJ|vfile.obj
				if(tokens.size() < 2) continue;
				std::string vfilename = tokens[1];
				std::string vfilecontent = "";
				std::getline(*file, line);
				//~ std::cout << "\nSTARTING VIRTUAL OBJ FILE: " << vfilename << std::endl;
				while(!tkCmd(line, "ENDVOBJ") && file->good()){
					vfilecontent += line + "\n";
					std::getline(*file, line);
				}
				OBJModel model(vfilecontent, true);
				model.myFileName = vfilename;
				//If it already exists? Delete it.
				if(Models.count(vfilename)) delete Models[vfilename];
				Models[vfilename] = new IndexedModel(model.toIndexedModel());
			} else if (tkCmd(tokens[0], "CONVEXHULL")) {
				// Since this is an independent object, the independence flag
				// can be safely ignored.
				std::string filename = "|";
				int independence = 1;
				if (tokens.size() > 1) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					IndexedModel bruh = getIndexedModelFromFile(tokens[1]);
					if (Scaling != 1) { // Apply a scaling transformation to it.
						Transform trans;
						trans.setScale(glm::vec3(Scaling, Scaling, Scaling));
						bruh.applyTransform(trans.getModel());
					}
					if (bruh.positions.size() < 4) {
						std::cout << "\nERROR!!! Either your file failed to "
									 "load, or it didn't "
									 "have enough positions upon loading (at "
									 "least 4).\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					lastCollisionShape = makeConvexHullShape(bruh);
					if (!lastCollisionShape) {
						std::cout << "\nERROR!!! Convex Hull Shape came out "
									 "null for some "
									 "reason!!!\nLine: "
								  << line << std::endl;
						delete lastCollisionShape; // Important step...
						error_flag = 1;
						break;
					}
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on ConvexHull line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "BOXSHAPE")) {
				// Since this is an independent object, the independence flag
				// can be safely ignored.
				btVector3 Dimensions = btVector3(1, 1, 1);
				int independence = 1;

				if (tokens.size() > 3) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					Dimensions.setX(GkAtof(tokens[1].c_str()));
					Dimensions.setY(GkAtof(tokens[2].c_str()));
					Dimensions.setZ(GkAtof(tokens[3].c_str()));
					Dimensions *= Scaling;
					if (tokens.size() > 4)
						independence = GkAtoi(tokens[4].c_str());
					lastCollisionShape = new btBoxShape(Dimensions);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on BoxShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "SPHERESHAPE")) {
				// Since this is an independent object, the independence flag
				// can be safely ignored.
				float radius = 1;
				int independence = 1;
				//~ btVector3 Inertia = btVector3(0,0,0);
				//~ float mass = 1.0;

				if (tokens.size() > 1) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					radius = GkAtof(tokens[1].c_str());
					radius *= Scaling;
					if (tokens.size() > 2)
						independence = GkAtoi(tokens[2].c_str());
					//~ if(tokens.size() > 5)
					//~ {
					//~ Inertia.setX(GkAtof(tokens[3].c_str()));
					//~ Inertia.setY(GkAtof(tokens[4].c_str()));
					//~ Inertia.setZ(GkAtof(tokens[5].c_str()));
					//~ }
					//~ if(tokens.size() > 6)
					//~ mass = GkAtof(tokens[6].c_str());
					lastCollisionShape = new btSphereShape(radius);
					//~ if(mass != 0.0)
					//~
					//((btSphereShape*)lastCollisionShape)->calculateLocalInertia(mass,
					// Inertia);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on SphereShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "CONESHAPE")) {
				// Since this is an independent object, the independence flag
				// can be safely ignored.
				float radius = 1;
				float height = 1;
				int independence = 1;

				if (tokens.size() > 2) {
					//~ RetVal->ShapeCreationLines.push_back(line);
					radius = GkAtof(tokens[1].c_str());
					radius *= Scaling;
					height = GkAtof(tokens[2].c_str());
					height *= Scaling;
					if (tokens.size() > 3)
						independence = GkAtoi(tokens[3].c_str());
					lastCollisionShape = new btConeShape(radius, height);
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);

				} else {
					std::cout << "\nERROR!!! too few args on ConeShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "TRIMSHSHAPE")) {
				if (tokens.size() > 1) {
					//~ std::cout << "\nMaking Triangle Mesh Shape..." <<
					// std::endl; ~ RetVal->ShapeCreationLines.push_back(line);
					IndexedModel bruh = getIndexedModelFromFile(tokens[1]);
					if (Scaling != 1) { // Apply a scaling transformation to it.
						Transform trans;
						trans.setScale(glm::vec3(Scaling));
						bruh.applyTransform(trans.getModel());
					}
					if (bruh.indices.size() < 3) {
						std::cout << "\nERROR!!! TriMesh failed to load, or "
									 "had too few "
									 "indices. Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}

					btTriangleMesh* buddy = makeTriangleMesh(bruh);
					if (!buddy) {
						std::cout << "\nERROR!!! TriMesh came out as NULL! "
									 "Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					lastCollisionShape = new btBvhTriangleMeshShape(buddy, false);
					((btBvhTriangleMeshShape*)lastCollisionShape)->buildOptimizedBvh();
					//~ std::cout <<"\nMade Triangle Mesh Shape!" << std::endl;
					RetVal->OwnedShapes.push_back(lastCollisionShape);
					ShapesInOrder.push_back(lastCollisionShape);
				} else {
					std::cout << "\nERROR!!! too few args on TriMeshShape line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MSHINST")) {
				if (tokens.size() > 1) // We need the mesh at minimum...
				{
					
					lastMeshInstance = new MeshInstance();
					RetVal->MeshInstances.push_back(lastMeshInstance);
					
					
					// Instantiate the meshinstance
					for(uint i = 1; i < tokens.size(); i++){
						uint id = GkAtoui(tokens[i].c_str());
						if (id >= MeshesInOrder.size() || !MeshesInOrder[id]) {
							std::cout << "\nERROR! You can't register to a mesh "
										 "that doesn't "
										 "exist!\nLine: "
									  << line << std::endl;
							error_flag = 1;
							break;
						}
						MeshesInOrder[id]->registerInstance(lastMeshInstance);
					}
				} else {
					std::cout << "\nERROR!!! too few args on MeshInstance line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MI_TEXOFFSET")) {
				if(lastMeshInstance && tokens.size() > 2)
				{
					lastMeshInstance->texOffset.x = GkAtof(tokens[1].c_str());
					lastMeshInstance->texOffset.y = GkAtof(tokens[2].c_str());
				}
			} else if (tkCmd(tokens[0], "MSH_TEXOFFSET")) {
				if(lastMesh && tokens.size() > 2)
				{
					lastMesh->instanced_texOffset.x = GkAtof(tokens[1].c_str());
					lastMesh->instanced_texOffset.y = GkAtof(tokens[2].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_DIFF")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.diffusivity = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_AMB")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.ambient = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_SPECR")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.specreflectivity = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_SPECD")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.specdamp = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "PHONG_EMISS")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->myPhong.emissivity = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_TEX")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->tex = GkAtoui(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_CUBE")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->cubeMap = GkAtoui(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_DIFF_CUBE")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->EnableCubemapDiffusion = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_REFL_CUBE")) {
				if (lastMeshInstance && tokens.size() > 1) {
					//~ std::cout << "SET CUBEMAP REFLECTIONS ON
					// MESHINSTANCE!!!\n" << std::endl;
					lastMeshInstance->EnableCubemapReflections = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MI_MSHMASK")) {
				if (lastMeshInstance && tokens.size() > 1) {
					lastMeshInstance->mymeshmask = GkAtoi(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "HITBOX")) {
				// TODO code this
				uint shape = 0;
				if (tokens.size() > 1) {
					shape = GkAtoui(tokens[1].c_str());
					if (shape >= ShapesInOrder.size() || (ShapesInOrder[shape]->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)) {
						std::cout << "\nERROR!!! Either Shape ID is too high. (It "
									 "starts at ZERO) "
									 "or it's a compound shape. Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					btTransform t;
					t.setIdentity();
					btMotionState* motion = new btDefaultMotionState(t); // This will be owned by the rigid body we create.
					btVector3 inertia(0, 0, 0);

					btRigidBody::btRigidBodyConstructionInfo info(0, motion, ShapesInOrder[shape], inertia);
					RetVal->Hitboxes.push_back(new btRigidBody(info));
					btRigidBody* body = RetVal->Hitboxes.back();
					body->setUserPointer((void*)(&RetVal->ColInfo));
				}
			} else if (tkCmd(tokens[0], "BTRIGIDBODY")) {
				//#BTRIGIDBODY|SHAPE|MASS|INIT X|Y|Z|EULER
				// X|Y|Z|FRICTION|RESTITUTION|ROLLINGFRICTION|LINEARDAMPENING|ANGULARDAMPENING

				//~ std::cout << "\nMAKING A RIGID BODY FOR A TEMPLATED
				// GkObject!" << std::endl;
				uint shape = 0;
				float mass = 1;
				// btVector3 InitialXYZ = btVector3(0,0,0);
				glm::vec3 initxyz_glm = glm::vec3(0, 0, 0);
				glm::vec3 initEuler_glm = glm::vec3(0, 0, 0);
				float friction = 0.5;
				float restitution = 0;
				float rollingfriction = 0;
				float lineardamp = 0;
				float angdamp = 0;
				if (tokens.size() > 1) {
					shape = GkAtoui(tokens[1].c_str());
					if (tokens.size() > 2)
						mass = GkAtof(tokens[2].c_str());
					if (tokens.size() > 5) {
						//~ InitialXYZ.setX(GkAtof(tokens[3].c_str()));
						//~ InitialXYZ.setY(GkAtof(tokens[4].c_str()));
						//~ InitialXYZ.setZ(GkAtof(tokens[5].c_str()));
						initxyz_glm.x = GkAtof(tokens[3].c_str());
						initxyz_glm.y = GkAtof(tokens[4].c_str());
						initxyz_glm.z = GkAtof(tokens[5].c_str());
						//~ initxyz_glm = Scaling * initxyz_glm;
					}
					if (tokens.size() > 8) {
						//~ InitialXYZ.setX(GkAtof(tokens[3].c_str()));
						//~ InitialXYZ.setY(GkAtof(tokens[4].c_str()));
						//~ InitialXYZ.setZ(GkAtof(tokens[5].c_str()));
						initEuler_glm.x = GkAtof(tokens[6].c_str());
						initEuler_glm.y = GkAtof(tokens[7].c_str());
						initEuler_glm.z = GkAtof(tokens[8].c_str());
					}
					Transform localTransform = Transform(initxyz_glm, initEuler_glm, glm::vec3(1, 1,
																							   1)); // Position it correctly in 3d space
					localTransform.setModel(initTransform.getModel() * localTransform.getModel());
					localTransform.setScale(glm::vec3(1, 1, 1));
					if (tokens.size() > 9)
						friction = GkAtof(tokens[9].c_str());
					if (tokens.size() > 10)
						restitution = GkAtof(tokens[10].c_str());
					if (tokens.size() > 11)
						rollingfriction = GkAtof(tokens[11].c_str());
					if (tokens.size() > 12)
						lineardamp = GkAtof(tokens[12].c_str());
					if (tokens.size() > 13)
						angdamp = GkAtof(tokens[13].c_str());
					if (shape >= ShapesInOrder.size()) {
						std::cout << "\nERROR!!! Shape ID is too high. It "
									 "starts at ZERO, "
									 "Integer value. Aborting...\nLine: "
								  << line << std::endl;
						error_flag = 1;
						break;
					}
					btTransform t = g2b_transform(localTransform.getModel());
					btMotionState* motion = new btDefaultMotionState(t); // This will be owned by the rigid body we create.
					btVector3 inertia(0, 0, 0);
					if (mass != 0.0) {
						ShapesInOrder[shape]->calculateLocalInertia(mass, inertia);
					}
					btRigidBody::btRigidBodyConstructionInfo info(mass, motion, ShapesInOrder[shape], inertia);
					info.m_friction = friction;
					info.m_restitution = restitution;
					info.m_rollingFriction = rollingfriction;
					info.m_linearDamping = lineardamp;
					info.m_angularDamping = angdamp;
					RetVal->RigidBodies.push_back(new btRigidBody(info));
					btRigidBody* body = RetVal->RigidBodies.back();
					body->setUserPointer((void*)(&RetVal->ColInfo));
				} else {
					std::cout << "\nERROR!!! too few args on btRigidBody line. "
								 "Aborting...\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MAKECHARCONTROLLER")) {
				RetVal->makeCharacterController();
				// To be developed
			} else if (tkCmd(tokens[0], "REQUIRES_EXPORT")) {
				RetVal->requiresExporting = true;
			} else if (tkCmd(tokens[0], "COLSOUND")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colsoundbuffer = loadWAV(tokens[1]);
					RetVal->colSoundFileName = tokens[1];
				}
			} else if (tkCmd(tokens[0], "MINCOLSOUNDIMPULSE")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinImpulse = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "LOUDNESS")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundLoudness = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "DISABLECOLSOUNDS")) {
				RetVal->ColInfo.disableColSounds = true;
			} else if (tkCmd(tokens[0], "MAXDISTANCE")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxDistance = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "ROLLOFF")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundRolloffFactor = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MINPITCH")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinPitch = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MAXPITCH")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxPitch = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MINGAIN")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMinGain = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "MAXGAIN")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					RetVal->ColInfo.colSoundMaxGain = GkAtof(tokens[1].c_str());
				}
			} else if (tkCmd(tokens[0], "LOADSOUND")) {
				if (tokens.size() > 1) {
					//~ RetVal->soundCommands.push_back(line);
					loadWAV(tokens[1]);
				}
			} else if (tkCmd(tokens[0], "ANIM")) {
				//~ RetVal->AnimCreationLines.push_back(line);
				if (!RetVal->Animations) {
					RetVal->Animations = new std::map<std::string, GkAnim>();
					RetVal->shouldDeleteAnimations = true;
				}
				std::string name = "DEFAULT";
				float timing = 0.0166666;
				if (tokens.size() > 1)
					name = tokens[1];
				if (tokens.size() > 2)
					timing = GkAtof(tokens[2].c_str());
				(*(RetVal->Animations))[name] = GkAnim(name, timing);
				lastAnimationName = name;
			} else if (tkCmd(tokens[0], "FRAME")) {
				if ((*(RetVal->Animations)).count(lastAnimationName)) {
					(*(RetVal->Animations))[lastAnimationName].addFrame();
					//~ RetVal->AnimCreationLines.push_back(line);
				} else {
					std::cout << "\nErroneous FRAME command... Aborting...\nLine: " << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "MATRIX")) {
				if (tokens.size() > 17 && RetVal->Animations && (*(RetVal->Animations)).count(lastAnimationName) &&
					((*(RetVal->Animations))[lastAnimationName].Frames.size() > 0)) // 2-17 are the matrix.
				{
					//~ RetVal->AnimCreationLines.push_back(line);
					uint id = GkAtoui(tokens[1].c_str());
					glm::mat4 matrix;
					for (int i = 2; i < 18; i++) // Will process i = 2, Won't process i = 18
						matrix[(i - 2) / 4][(i - 2) % 4] = GkAtof(tokens[i].c_str());
					// Scale the matrix.
					//~ if (Scaling != 1) {
					//~ Transform b;
					//~ b.setModel(matrix);
					//~ b.setScale(glm::vec3(Scaling, Scaling, Scaling));
					//~ matrix = b.getModel();
					//~ }
					(*(RetVal->Animations))[lastAnimationName].setMat4((uint)((*(RetVal->Animations))[lastAnimationName].Frames.size() - 1), id, matrix);
				} else {
					std::cout << "ERROR!!! Either not enough args for matrix, no "
								 "animations, last animation does not exist, or "
								 "forgot "
								 "to call FRAME.\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			} else if (tkCmd(tokens[0], "TRANSFORM")) {
				if (tokens.size() > 1 && RetVal->Animations && RetVal->Animations->count(lastAnimationName) &&
					((*(RetVal->Animations))[lastAnimationName].Frames.size() > 0)

				) {
					//~ RetVal->AnimCreationLines.push_back(line);
					uint id = GkAtoui(tokens[1].c_str());
					glm::vec3 pos(0, 0, 0), rot(0, 0, 0), scale(1, 1, 1);
					if (tokens.size() > 4) {
						pos.x = GkAtof(tokens[2].c_str());
						pos.y = GkAtof(tokens[3].c_str());
						pos.z = GkAtof(tokens[4].c_str());
					}
					if (tokens.size() > 7) {
						rot.x = GkAtof(tokens[5].c_str());
						rot.y = GkAtof(tokens[6].c_str());
						rot.z = GkAtof(tokens[7].c_str());
					}
					if (tokens.size() > 10) {
						scale.x = GkAtof(tokens[8].c_str());
						scale.y = GkAtof(tokens[9].c_str());
						scale.z = GkAtof(tokens[10].c_str());
					}

					(*(RetVal->Animations))[lastAnimationName].setMat4((uint)((*(RetVal->Animations))[lastAnimationName].Frames.size() - 1), id,
																	   (Transform(pos * Scaling, rot, scale)).getModel());
				} else {
					std::cout << "ERROR!!! Either not enough args for transform, no "
								 "animations, last animation does not exist, or "
								 "forgot "
								 "to call FRAME.\nLine: "
							  << line << std::endl;
					error_flag = 1;
					break;
				}
			}
			// TODO: Re-package for GkEngine's serializer later.
			else if (tkCmd(tokens[0], "FIXEDCONSTRAINT")) {
				if (tokens.size() < 3)
					continue;   // Needs at least the IDs.
				uint BodyA = 0; // arg 1
				uint BodyB = 0; // arg 2
				btTransform tia;
				btTransform tib;
				btTransform btrans;
				btTransform atrans;
				float BreakingImpulse = 0; // arg 3
				btFixedConstraint* b = nullptr;

				BodyA = GkAtoui(tokens[1].c_str());
				BodyB = GkAtoui(tokens[2].c_str());
				tia.setIdentity();
				tib.setIdentity();
				if (tokens.size() > 3)
					BreakingImpulse = GkAtof(tokens[3].c_str());
				if (BodyA >= RetVal->RigidBodies.size() || BodyB >= RetVal->RigidBodies.size() || BodyA == BodyB) {
					std::cout << "\nERROR! Bad Constraint line. Bad IDs. Aborting... Line: " << line << std::endl;
					error_flag = 1;
					break;
				}
				RetVal->RigidBodies[BodyA]->getMotionState()->getWorldTransform(atrans);
				RetVal->RigidBodies[BodyB]->getMotionState()->getWorldTransform(btrans);

				tib = btrans.inverse() * atrans;
				//~ tia = atrans.inverse() * btrans;

				b = new btFixedConstraint(*(RetVal->RigidBodies[BodyA]), *(RetVal->RigidBodies[BodyB]), tia, tib);
				b->setBreakingImpulseThreshold(BreakingImpulse);
				RetVal->Constraints.push_back(b);
			} else if (tkCmd(tokens[0], "POINT2POINTCONSTRAINT")) {
				if (tokens.size() < 3)
					continue;   // Needs at least the IDs.
				uint BodyA = 0; // arg 1
				uint BodyB = 0; // arg 2
				btVector3 pa;
				btVector3 pb;
				float BreakingImpulse = 0; // arg 9
				btPoint2PointConstraint* b = nullptr;

				BodyA = GkAtoui(tokens[1].c_str());
				BodyB = GkAtoui(tokens[2].c_str());
				if (tokens.size() > 5) {
					pa.setX(GkAtof(tokens[3].c_str()));
					pa.setY(GkAtof(tokens[4].c_str()));
					pa.setZ(GkAtof(tokens[5].c_str()));
				}
				if (tokens.size() > 8) {
					pb.setX(GkAtof(tokens[6].c_str()));
					pb.setY(GkAtof(tokens[7].c_str()));
					pb.setZ(GkAtof(tokens[8].c_str()));
				}
				if (tokens.size() > 9)
					BreakingImpulse = GkAtof(tokens[9].c_str());
				if (BodyA >= RetVal->RigidBodies.size() || BodyB >= RetVal->RigidBodies.size() || BodyA == BodyB) {
					std::cout << "\nERROR! Bad Constraint line. Bad IDs. Aborting... Line: " << line << std::endl;
					error_flag = 1;
					break;
				}
				b = new btPoint2PointConstraint(*(RetVal->RigidBodies[BodyA]), *(RetVal->RigidBodies[BodyB]), pa, pb);
				b->setBreakingImpulseThreshold(BreakingImpulse);
				RetVal->Constraints.push_back(b);
			}
			//~ else if (tkCmd(tokens[0], "6DOFCONSTRAINT")) {
			//~ if (tokens.size() < 15) {
			//~ std::cout << "\nERROR! Too few args for 6DOFConstraint. Line: " << line << std::endl;

			//~ continue;
			//~ }							 // Needs EVERYTHING
			//~ uint BodyA = 0;				 // arg 1
			//~ uint BodyB = 0;				 // arg 2
			//~ glm::mat4 TransformInA(1.0); // arg 3 - 8
			//~ btTransform tia;			 // After conversion to bullet type.
			//~ glm::mat4 TransformInB(1.0); // arg 9 - 14
			//~ btTransform tib;			 // ditto
			//~ glm::vec3 grabber;
			//~ Transform helper;
			//~ glm::vec3 LinLow(0); // arg 15,16,17
			//~ glm::vec3 LinHi(0);
			//~ glm::vec3 AngLow(0);
			//~ glm::vec3 AngHi(0);
			//~ glm::vec3 LinStiff(-1);
			//~ glm::vec3 AngStiff(-1);
			//~ glm::vec3 LinDamp(0);
			//~ glm::vec3 AngDamp(0);
			//~ float BreakingImpulse = 0;
			//~ btGeneric6DofSpring2Constraint* b = nullptr;

			//~ BodyA = GkAtoui(tokens[1].c_str());
			//~ BodyB = GkAtoui(tokens[2].c_str());
			//~ grabber.x = (GkAtof(tokens[3].c_str()));
			//~ grabber.y = (GkAtof(tokens[4].c_str()));
			//~ grabber.z = (GkAtof(tokens[5].c_str()));
			//~ helper.setPos(grabber);
			//~ grabber.x = (GkAtof(tokens[6].c_str()));
			//~ grabber.y = (GkAtof(tokens[7].c_str()));
			//~ grabber.z = (GkAtof(tokens[8].c_str()));
			//~ helper.setRot(grabber);
			//~ tia = g2b_transform(helper.getModel());
			//~ grabber.x = (GkAtof(tokens[9].c_str()));
			//~ grabber.y = (GkAtof(tokens[10].c_str()));
			//~ grabber.z = (GkAtof(tokens[11].c_str()));
			//~ helper.setPos(grabber);
			//~ grabber.x = (GkAtof(tokens[12].c_str()));
			//~ grabber.y = (GkAtof(tokens[13].c_str()));
			//~ grabber.z = (GkAtof(tokens[14].c_str()));
			//~ helper.setRot(grabber);
			//~ tib = g2b_transform(helper.getModel());
			//~ if (tokens.size() > 26) {
			//~ LinLow.x = (GkAtof(tokens[15].c_str()));
			//~ LinLow.y = (GkAtof(tokens[16].c_str()));
			//~ LinLow.z = (GkAtof(tokens[17].c_str()));
			//~ LinHi.x = (GkAtof(tokens[18].c_str()));
			//~ LinHi.y = (GkAtof(tokens[19].c_str()));
			//~ LinHi.z = (GkAtof(tokens[20].c_str()));
			//~ AngLow.x = (GkAtof(tokens[21].c_str()));
			//~ AngLow.y = (GkAtof(tokens[22].c_str()));
			//~ AngLow.z = (GkAtof(tokens[23].c_str()));
			//~ AngHi.x = (GkAtof(tokens[24].c_str()));
			//~ AngHi.y = (GkAtof(tokens[25].c_str()));
			//~ AngHi.z = (GkAtof(tokens[26].c_str()));
			//~ }

			//~ // Spring properties. Negative stiffness means disabled spring.
			//~ if (tokens.size() > 38) {
			//~ LinStiff.x = (GkAtof(tokens[27].c_str()));
			//~ LinStiff.y = (GkAtof(tokens[28].c_str()));
			//~ LinStiff.z = (GkAtof(tokens[29].c_str()));

			//~ AngStiff.x = (GkAtof(tokens[30].c_str()));
			//~ AngStiff.y = (GkAtof(tokens[31].c_str()));
			//~ AngStiff.z = (GkAtof(tokens[32].c_str()));

			//~ LinDamp.x = (GkAtof(tokens[33].c_str()));
			//~ LinDamp.y = (GkAtof(tokens[34].c_str()));
			//~ LinDamp.z = (GkAtof(tokens[35].c_str()));

			//~ AngDamp.x = (GkAtof(tokens[36].c_str()));
			//~ AngDamp.y = (GkAtof(tokens[37].c_str()));
			//~ AngDamp.z = (GkAtof(tokens[38].c_str()));
			//~ }

			//~ if (tokens.size() > 39)
			//~ BreakingImpulse = GkAtof(tokens[39].c_str());

			//~ if (BodyA >= RetVal->RigidBodies.size() || BodyB >= RetVal->RigidBodies.size() || BodyA == BodyB) {
			//~ std::cout << "\nERROR! Bad Constraint line. Bad IDs. Aborting... Line: " << line << std::endl;
			//~ error_flag = 1;
			//~ break;
			//~ }
			//~ b = new btGeneric6DofSpring2Constraint(*(RetVal->RigidBodies[BodyA]), *(RetVal->RigidBodies[BodyB]), tia, tib);
			//~ // Set limits
			//~ b->setLimit(0, LinLow.x, LinHi.x);
			//~ b->setLimit(1, LinLow.y, LinHi.y);
			//~ b->setLimit(2, LinLow.z, LinHi.z);
			//~ b->setLimit(3, AngLow.x, AngHi.x);
			//~ b->setLimit(4, AngLow.y, AngHi.y);
			//~ b->setLimit(5, AngLow.z, AngHi.z);
			//~ b->enableSpring(0, (LinStiff.x >= 0));
			//~ b->enableSpring(1, (LinStiff.y >= 0));
			//~ b->enableSpring(2, (LinStiff.z >= 0));
			//~ b->enableSpring(3, (AngStiff.x >= 0));
			//~ b->enableSpring(4, (AngStiff.y >= 0));
			//~ b->enableSpring(5, (AngStiff.z >= 0));

			//~ b->setStiffness(0, (LinStiff.x));
			//~ b->setStiffness(1, (LinStiff.y));
			//~ b->setStiffness(2, (LinStiff.z));
			//~ b->setStiffness(3, (AngStiff.x));
			//~ b->setStiffness(4, (AngStiff.y));
			//~ b->setStiffness(5, (AngStiff.z));

			//~ b->setDamping(0, (LinDamp.x));
			//~ b->setDamping(1, (LinDamp.y));
			//~ b->setDamping(2, (LinDamp.z));
			//~ b->setDamping(3, (AngDamp.x));
			//~ b->setDamping(4, (AngDamp.y));
			//~ b->setDamping(5, (AngDamp.z));
			//~ b->setBreakingImpulseThreshold(BreakingImpulse);
			//~ RetVal->Constraints.push_back(b);
			//~ }

		} // EOF giant while loop for independent object parser
		if (RetVal->Animations && RetVal->Animations->count("DEFAULT")) {
			RetVal->setAnimation("DEFAULT", 0.0);
			//~ std::cout << "\nSetting Default Anim...";
		}
		if(RetVal->requiresExporting){
			RetVal->ExportString = file->str();
		}
		if (file)
			delete file;
	} // EOF independent object parser
	// Catch all

	if (error_flag) {
		if (RetVal)
			delete RetVal; // Retval owns everything we allocated. This takes
						   // care of the problem.
		return nullptr;
	}
	return RetVal;
}
GkObject* AdvancedResourceManager::getAnObject(std::string filename, bool templated, Transform initTransform) {
	// Force creation scales to be one-dimensional.
	glm::vec3 scale = initTransform.getScale();
	scale = glm::vec3(scale.x, scale.x, scale.x);
	initTransform.setScale(scale);

	GkObject* the_template = nullptr;
	if (templated) // Requires scale 1
	{
		//~ std::cout << "\nWe need a Template..." << std::endl;
		initTransform.setScale(glm::vec3(1, 1, 1)); // Force scale of 1 for templated objects.
		the_template = loadObject(filename, true, nullptr, initTransform);
		scale = glm::vec3(1, 1, 1);
		if (the_template)
			the_template->creationScale = scale; // Largely pointless
												 //~ std::cout << "About to load an object using a template..." <<
												 // std::endl;
	}

	GkObject* retval = loadObject(filename, false, the_template, initTransform);
	if (retval)
		retval->creationScale = scale;
	return retval;
}
std::stringstream* AdvancedResourceManager::getTextFileStream(std::string name) {
	getTextFile(name);
	return new std::stringstream(TextFiles[name]);
}

void AdvancedResourceManager::UnloadAll() {
	TextFiles.clear();
	for (auto it = audiobuffers.begin(); it != audiobuffers.end(); ++it) {
		alDeleteBuffers(1, &(it->second));
		it->second = 0;
	}
	audiobuffers.clear();
	for (auto it = Models.begin(); it != Models.end(); ++it) {
		delete it->second;
		it->second = nullptr;
	}
	Models.clear();
	for (auto it = Textures.begin(); it != Textures.end(); ++it) {
		delete it->second;
		it->second = nullptr;
	}
	Textures.clear();
	for (auto it = OurCubeMaps.begin(); it != OurCubeMaps.end(); ++it) {
		delete it->second;
		it->second = nullptr;
	}
	OurCubeMaps.clear();
	for (auto it = Meshes.begin(); it != Meshes.end(); ++it) {
		theScene->deregisterMesh(it->second);
		delete it->second;
		it->second = nullptr;
	}
	Meshes.clear();
	for (auto it = TemplateObjects.begin(); it != TemplateObjects.end(); ++it) {
		it->second->deRegister(theScene, world, myWorldType);
		delete it->second;
		it->second = nullptr;
	}
	TemplateObjects.clear();
}

AdvancedResourceManager::~AdvancedResourceManager() { UnloadAll(); }
/// GkAnim Constructor
GkAnim::GkAnim() {}
/// GkAnim Constructor
GkAnim::GkAnim(std::string _name) { name = _name; }
/// GkAnim Constructor
GkAnim::GkAnim(std::string _name, float _timing) {
	name = _name;
	timing = _timing;
}
/// GkAnim Constructor
GkAnim::GkAnim(const GkAnim& Other) {
	Frames = Other.Frames;
	timing = Other.timing;
	name = Other.name;
}
/// GkAnim Constructor (=)
void GkAnim::operator=(const GkAnim& Other) {
	Frames = Other.Frames;
	timing = Other.timing;
	name = Other.name;
}
/// Should be called pushFrame for consistency.
void GkAnim::addFrame(std::vector<glm::mat4> input) { Frames.push_back(input); }

/// Main method for setting animation transforms.
void GkAnim::setMat4(uint frame, uint item, glm::mat4 matrix) {
	while (Frames.size() < frame + 1)
		Frames.push_back(std::vector<glm::mat4>());
	const glm::mat4 infinity_scale(std::numeric_limits<float>::max()); // Diagonal of infinities.
	while (Frames[frame].size() < item + 1)							   // Nuh uh! Make it infinity.
		Frames[frame].push_back(infinity_scale);
	Frames[frame][item] = matrix;
}
glm::mat4 GkAnim::getMat4(uint frame, uint item) {
	if ((frame < Frames.size()) && (item < Frames[frame].size()))
		return Frames[frame][item];
	else
		return glm::mat4(std::numeric_limits<float>::max());
}
std::vector<glm::mat4>& GkAnim::operator[](int i) { return Frames[i]; }
const std::vector<glm::mat4>& GkAnim::operator[](int i) const { return Frames[i]; }
float GkAnim::getDuration() { return timing * Frames.size(); }
// Functions for UI crap~~~~~~~~~~~~~~~~~~~~~~~~~~
// GkControlInterface
GkControlInterface::GkControlInterface() {}
void GkControlInterface::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {}
void GkControlInterface::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {}
void GkControlInterface::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {}

void GkControlInterface::cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
												  glm::vec2 NormalizedCursorDelta) {}
void GkControlInterface::char_callback(GLFWwindow* window, unsigned int codepoint) {}
void GkControlInterface::joystickAxes_callback(int jid, const float* axes, int naxes){}
void GkControlInterface::joystickButtons_callback(int jid, const unsigned char* buttons, int nbuttons){}
bool GkControlInterface::doesGLFWContextSwitching() { return false; }

// TextBoxInfo
TextBoxInfo::TextBoxInfo(uint _x, uint _y, uint _width, uint _height, std::string _string, unchar _red, unchar _green, unchar _blue, unchar _alpha,
						 glm::vec3 _string_color, uint _text_offset_x, uint _text_offset_y, uint _charw, uint _charh) {
	x = _x;
	y = _y;
	width = _width;
	height = _height;
	string = _string;
	red = _red;
	green = _green;
	blue = _blue;
	alpha = _alpha;
	//~ background = _background;
	text_offset_x = _text_offset_x;
	text_offset_y = _text_offset_y;
	string_color = _string_color;
	charw = _charw;
	charh = _charh;
}
TextBoxInfo::TextBoxInfo() {}

inline bool isCursorInBox(uint box_x, uint box_y, uint box_width, uint box_height, uint screen_width, uint screen_height, float scalingFactor, double cX,
						  double cY) {
	float box_x_screen = (float)box_x * 1 / scalingFactor;
	float box_y_screen = (float)box_y * 1 / scalingFactor;

	float box_width_screen = (float)box_width * 1 / scalingFactor;
	float box_height_screen = (float)box_height * 1 / scalingFactor;
	return ((cX >= box_x_screen && (cX <= box_x_screen + box_width_screen)) && (cY >= box_y_screen && (cY <= box_y_screen + box_height_screen)));
}
// GkButton~~~~~~~~~~~~~~~~~
GkButton::GkButton(TextBoxInfo info) { box = info; }
void GkButton::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	//~ double n_x = (double) box.x * 1/scalingFactor / (double) screen_width;
	//~ double n_y = (double) box.y * 1/scalingFactor / (double) screen_height;
	//~ double n_width = (double) box.width * 1/scalingFactor / (double)
	// screen_width; ~ double n_height = (double) box.height * 1/scalingFactor/
	//(double) screen_height; ~ double n_x_2 = n_x + n_width; ~ double n_y_2 =
	// n_y
	//+ n_height;
	uint base_x = 0;
	uint base_y = 0;
	if (GkUIWindowParent) {
		GkUIWindow* b = (GkUIWindow*)GkUIWindowParent;
		base_x = b->WindowBody.x;
		base_y = b->WindowBody.y;
		if(box.shouldScroll)
			base_y += b->scrollPosition;
	}
	if (ClickCallBack &&
		isCursorInBox(base_x + box.x, base_y + box.y, box.width, box.height, screen_width, screen_height, scalingFactor, cursorPos[0], cursorPos[1]))
		ClickCallBack(this, button, action, mods);
}
void GkButton::cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
										glm::vec2 NormalizedCursorDelta) {
	cursorPos[0] = xpos;
	cursorPos[1] = screen_height - ypos;
	uint base_x = 0;
	uint base_y = 0;
	if (GkUIWindowParent) {
		GkUIWindow* b = (GkUIWindow*)GkUIWindowParent;
		base_x = b->WindowBody.x;
		base_y = b->WindowBody.y;
		if(box.shouldScroll)
			base_y += b->scrollPosition;
	}
	if (HoverCallBack &&
		isCursorInBox(base_x + box.x, base_y + box.y, box.width, box.height, screen_width, screen_height, scalingFactor, cursorPos[0], cursorPos[1]))
		HoverCallBack(this, xpos, ypos, nxpos, nypos, CursorDelta, NormalizedCursorDelta);
}
// GkUIWindow's Functions ~~~~~~~~~~~

GkUIWindow::GkUIWindow(uint _screen_width, uint _screen_height) {
	screen_width = _screen_width;
	screen_height = _screen_height;
}
void GkUIWindow::setScreenSize(uint _screen_width, uint _screen_height, float _scalingFactor) {
	screen_width = _screen_width;
	screen_height = _screen_height;
	scalingFactor = _scalingFactor;
	for (auto& item : Buttons) {
		item.screen_width = screen_width;
		item.screen_height = screen_height;
		item.scalingFactor = scalingFactor;
	}
	for (auto& item : Fields) {
		item.screen_width = screen_width;
		item.screen_height = screen_height;
		item.scalingFactor = scalingFactor;
	}
}
GkUIWindow::~GkUIWindow() { destruct(); }
void GkUIWindow::destruct() { TextBoxes.clear(); }
void GkUIWindow::char_callback(GLFWwindow* window, unsigned int codepoint) {
	if (isHidden || isInBackground)
		return;
	for (auto& field : Fields)
		field.char_callback(window, codepoint);
}
void GkUIWindow::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (isHidden || isInBackground)
		return;
	for (auto& field : Fields)
		field.key_callback(window, key, scancode, action, mods);
	//~ for(auto& bt : Buttons)
	//~ bt.key_callback(window, key, scancode, action, mods);
	if (KeyCallBack)
		KeyCallBack(this, key, scancode, action, mods);
}

void GkUIWindow::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (isHidden)
		return;
	auto& box = WindowBody;
	//~ double n_x = (double) box.x * 1/scalingFactor / (double) screen_width;
	//~ double n_y = (double) box.y * 1/scalingFactor / (double) screen_height;
	//~ double n_width = (double) box.width * 1/scalingFactor / (double)
	// screen_width; ~ double n_height = (double) box.height * 1/scalingFactor/
	//(double) screen_height; ~ double n_x_2 = n_x + n_width; ~ double n_y_2 =
	// n_y
	//+ n_height;

	if (ClickCallBack && isCursorInBox(box.x, box.y, box.width, box.height, screen_width, screen_height, scalingFactor, cursorPos[0], cursorPos[1]))
		ClickCallBack(this, button, action, mods);
	if (!isInBackground) {
		for (auto& buttone : Buttons)
			buttone.mouse_button_callback(window, button, action, mods);
		for (auto& field : Fields)
			field.mouse_button_callback(window, button, action, mods);
	}
}

void GkUIWindow::scroll_callback(GLFWwindow* w, double xOff, double yOff){
	if (isHidden || isInBackground)
		return;
	//scrollPosition += -yOff * scrollSpeed; //Scrolling DOWN means elements on the page move UP
	if(yOff > 0.1) //Scrolling up.
		scrollPosition -= scrollSpeed;
	else if (yOff < 0.1) //scrolling down.
		scrollPosition += scrollSpeed;
	
	if(scrollPosition > scrollRange.y)
		scrollPosition = scrollRange.y;
	if(scrollPosition < scrollRange.x)
		scrollPosition = scrollRange.x;
}

void GkUIWindow::cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
										  glm::vec2 NormalizedCursorDelta) {
	if (isHidden)
		return;
	auto& box = WindowBody;
	cursorPos[0] = xpos;
	cursorPos[1] = screen_height - ypos;
	if (HoverCallBack && isCursorInBox(box.x, box.y, box.width, box.height, screen_width, screen_height, scalingFactor, cursorPos[0], cursorPos[1]))
		HoverCallBack(this, xpos, ypos, nxpos, nypos, CursorDelta, NormalizedCursorDelta);
	if (!isInBackground) {
		for (auto& button : Buttons)
			button.cursor_position_callback(window, xpos, ypos, nxpos, nypos, CursorDelta, NormalizedCursorDelta);
		for (auto& field : Fields)
			field.cursor_position_callback(window, xpos, ypos, nxpos, nypos, CursorDelta, NormalizedCursorDelta);
	}
}
void GkUIWindow::BuilderPushUp(int pixels){
	BuilderTopBar += pixels;
	for (auto& textbox : TextBoxes) if(textbox.shouldScroll){
		textbox.y += pixels;
	}
	for (auto& textbox : Fields) if(textbox.shouldScroll){
		textbox.y += pixels;
	}
	for (auto& button : Buttons) {
		auto& textbox = button.box;
		if(textbox.shouldScroll){
			textbox.y += pixels;
		}
	}
	//Create the scroll range.
	int diff = WindowBody.height - BuilderTopBar;
	if(diff >= 0){
		scrollRange = glm::ivec2(0,0);
	} else { //diff is a negative number and it represents how many pixels have to be scrolled to get to the top.
		scrollRange = glm::ivec2(diff, 0);
		scrollPosition = diff;
	}
}
void GkUIWindow::draw(BMPFontRenderer* renderer) {
	// TODO: test needsReDraw
	if (isHidden)
		return;
	int base_x = 0;
	int base_y = 0;
	int max_x = 0;
	int max_y = 0;
	// Draw the window body.
	{
		//~ std::cout << "\nDRAWING WINDOW BODY!" << std::endl;
		auto& textbox = WindowBody;
		base_x = textbox.x;
		base_y = textbox.y;
		max_x = textbox.x + textbox.width;
		max_y = textbox.y + textbox.height;
		{
			renderer->writeRectangle(textbox.x, textbox.y, textbox.x + textbox.width, textbox.y + textbox.height, textbox.red, textbox.green, textbox.blue,
									 textbox.alpha);
		}
		// write the string
		renderer->writeString(textbox.string, textbox.x + textbox.text_offset_x, textbox.y + textbox.text_offset_y, textbox.charw, textbox.charh,
							  textbox.string_color, glm::vec3(0), false);
	}
	//~ base_y += scrollPosition;
	//~ max_y += scrollPosition;
	for (auto& textbox : TextBoxes) {
		//Test if we can even draw the textbox. for scrolling.
		
		if(textbox.shouldScroll){
			base_y += scrollPosition;
			//~ max_y += scrollPosition;
		}
		if(textbox.y + base_y + textbox.height > max_y || textbox.y + base_y < 0)
				continue;
		{
			renderer->writeRectangle(base_x + textbox.x, base_y + textbox.y, base_x + textbox.x + textbox.width, base_y + textbox.y + textbox.height,
									 textbox.red, textbox.green, textbox.blue, textbox.alpha);
		}
		// write the string
		renderer->writeString(textbox.string, base_x + textbox.x + textbox.text_offset_x, base_y + textbox.y + textbox.text_offset_y, textbox.charw,
							  textbox.charh, textbox.string_color, glm::vec3(0), false);
		if(textbox.shouldScroll){
			base_y -= scrollPosition;
			//~ max_y -= scrollPosition;
		}
	}

	for (auto& textbox : Fields) {
	
		//Test if we can even draw the textbox.
		//~ if(textbox.y + textbox.height > max_y || textbox.y < base_y)
				//~ continue;
		if(textbox.shouldScroll){
			base_y += scrollPosition;
			//~ max_y += scrollPosition;
		}
		if(textbox.y + base_y + textbox.height > max_y || textbox.y + base_y < 0)
				continue;
		{
			renderer->writeRectangle(base_x + textbox.x, base_y + textbox.y, base_x + textbox.x + textbox.width, base_y + textbox.y + textbox.height,
									 textbox.red, textbox.green, textbox.blue, textbox.alpha);
		}
		//TODO: Allow for multi-line editing and display cursor appropriately.
		if (textbox.isEditing) { // Draw a rectangular cursor surrounding the
								 // selected text element.
			renderer->writeRectangle(
				// top left corner of cursor
				base_x + textbox.x + textbox.text_offset_x + textbox.textcursorpos * textbox.charw, base_y + textbox.y + textbox.text_offset_y,
				base_x + textbox.x + textbox.text_offset_x + (textbox.textcursorpos + 1) * textbox.charw,
				base_y + textbox.y + textbox.text_offset_y + textbox.charh, textbox.red_cursor, textbox.green_cursor, textbox.blue_cursor, 255);
		}
		// write the string
		renderer->writeString(textbox.string, base_x + textbox.x + textbox.text_offset_x, base_y + textbox.y + textbox.text_offset_y, textbox.charw,
							  textbox.charh, textbox.string_color, glm::vec3(0), false);
		if(textbox.shouldScroll){
			base_y -= scrollPosition;
			//~ max_y -= scrollPosition;
		}
	
	}
	for (auto& button : Buttons) {
		auto& textbox = button.box;
		//~ if(textbox.y + textbox.height > max_y || textbox.y < base_y)
				//~ continue;
		if(textbox.shouldScroll){
			base_y += scrollPosition;
			//~ max_y += scrollPosition;
		}
		if(textbox.y + base_y + textbox.height > max_y || textbox.y + base_y < 0)
				continue;
		{
			renderer->writeRectangle(base_x + textbox.x, base_y + textbox.y, base_x + textbox.x + textbox.width, base_y + textbox.y + textbox.height,
									 textbox.red, textbox.green, textbox.blue, textbox.alpha);
		}
		// write the string
		renderer->writeString(textbox.string, base_x + textbox.x + textbox.text_offset_x, base_y + textbox.y + textbox.text_offset_y, textbox.charw,
							  textbox.charh, textbox.string_color, glm::vec3(0), false);
		if(textbox.shouldScroll){
			base_y -= scrollPosition;
			//~ max_y -= scrollPosition;
		}
	}
}
// GkEdTextField's functions
void GkEdTextField::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	uint base_x = 0;
	uint base_y = 0;
	if (GkUIWindowParent) {
		GkUIWindow* b = (GkUIWindow*)GkUIWindowParent;
		base_x = b->WindowBody.x;
		base_y = b->WindowBody.y;
		if(shouldScroll)
			base_y += b->scrollPosition;
	}

	if (ClickCallBack && isCursorInBox(base_x + x, base_y + y, width, height, screen_width, screen_height, scalingFactor, cursorPos[0], cursorPos[1]))
		ClickCallBack(this, button, action, mods);
}
void GkEdTextField::cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
											 glm::vec2 NormalizedCursorDelta) {
	cursorPos[0] = xpos;
	cursorPos[1] = screen_height - ypos;
	uint base_x = 0;
	uint base_y = 0;
	if (GkUIWindowParent) {
		GkUIWindow* b = (GkUIWindow*)GkUIWindowParent;
		base_x = b->WindowBody.x;
		base_y = b->WindowBody.y;
		if(shouldScroll)
			base_y += b->scrollPosition;
	}
	if (HoverCallBack && isCursorInBox(base_x + x, base_y + y, width, height, screen_width, screen_height, scalingFactor, cursorPos[0], cursorPos[1]))
		HoverCallBack(this, xpos, ypos, nxpos, nypos, CursorDelta, NormalizedCursorDelta);
}

void GkEdTextField::char_callback(GLFWwindow* window, unsigned int codepoint) {
	if (!isEditing) {
		//~ std::cout << "\nNot editting, not recieving callback" << std::endl;
		return;
	}
	// Handle all printable characters.
	if (codepoint > 126 || codepoint < 9 || codepoint == 27 || (codepoint > 9 && codepoint < 32)) // Not ASCII printable
	{
		//~ std::cout << "\nNot Ascii Printabe!" << std::endl;
		return;
	}
	unsigned char c = (unsigned char)codepoint;
	if (textcursorpos <= string.length()) {
		string.insert(string.begin() + textcursorpos, c);
		textcursorpos++;
	} else {
		//~ std::cout << "\nText cursor position bad." << std::endl;
	}
}
void GkEdTextField::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (!isEditing)
		return;
	// We're editting, so use the key callback
	if ((key == GLFW_KEY_ENTER && action == GLFW_PRESS) || (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)) {
		//~ std::cout << "\nEnter or Escape key pressed. No longer editting." << std::endl;
		isEditing = false;
	}
	// Cursor manip
	if (key == GLFW_KEY_HOME && action == GLFW_PRESS)
		textcursorpos = 0;
	if (key == GLFW_KEY_END && action == GLFW_PRESS)
		textcursorpos = string.length();
	if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
		if (textcursorpos)
			textcursorpos--;
	if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
		if (textcursorpos < string.length())
			textcursorpos++;
	// Delete removes at current position and does not change current position,
	// unless last character, in which case it moves back. Backspace removes at
	// previous position and moves back.
	if (key == GLFW_KEY_DELETE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		if (textcursorpos >= string.length()) // No characters to remove.
			return;
		string.erase(string.begin() + textcursorpos);
		if (textcursorpos >= string.length()) // We
		{
			textcursorpos--;
		}
		if (string.length() == 0)
			textcursorpos = 0;
	}
	if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		if (textcursorpos && ((textcursorpos - 1) < string.length())) // Backspace won't DO anything
		{
			string.erase(string.begin() + (--textcursorpos));
		}
	}
	if (KeyCallBack)
		KeyCallBack(this, key, scancode, action, mods);
}

// GkFPSCharacterController's functions
GkFPSCharacterController::GkFPSCharacterController(GkObject* _body, Camera* _myCamera, glm::vec3 _RelPos, std::string configuration,
												   btDiscreteDynamicsWorld* world, bool _NPCMode, bool giveToBody) {
	ClassName = "GkFPSCharacterController"; // Entity component property.
	body = _body;
	if (body && (body->RigidBodies.size() > 0)) {
		body->ColInfo.userPointer = (void*)this;
		body->ColInfo.userData = "GkFPSCharacterController";
		if (world) {
			world->removeRigidBody(body->RigidBodies[0]);
			world->addRigidBody(body->RigidBodies[0], COL_CHARACTERS, COL_NORMAL | COL_TRIGGER | COL_CHARACTERS);
		}
		//~ body->RigidBodies[0]->setDamping(dampening, 0);
	}
	RelPos = _RelPos;
	myCamera = _myCamera;
	if (!myCamera) {
		ownCamera = true;
		myCamera = new Camera(glm::vec3(0, 0, 0), 70, 1, 1, 100, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
	}
	setControlConfig(configuration);
	NPCMode = _NPCMode;
	if (giveToBody && body) {
		//~ std::cout << "\nAttempting to add owned entity component..." << std::endl;
		body->addOwnedEntityComponent(this);
		GkObjectParent = body;
	}
}
GkFPSCharacterController::~GkFPSCharacterController() {
	//~ std::cout << "\nStarting Character controller destructor..." << std::endl;
	if (ownCamera) { /*std::cout << "\nDeleting OUR camera!" << std::endl;*/
		delete myCamera;
	}
	//~ std::cout << "\nFinishing Character controller destructor..." << std::endl;
}

void GkFPSCharacterController::setControlConfig(std::string configuration) {
	using namespace std;
	stringstream bruh(configuration);
	string line;
	if (!bruh.good())
		return;
	while (bruh.good()) {
		getline(bruh, line);
		auto tokens = SplitString(line, '|'); // ISSA VECTOR NIG
		string command_name;
		if (tokens.size() > 0)
			command_name = tokens[0];
		else
			continue;

		if ((size_t)(command_name.find("#")) != string::npos) // Skip Comments.
			continue;
		else if ((size_t)(command_name.find("Forward")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				Forward[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("Backward")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				Backward[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("Left")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				Left[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("Right")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				Right[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("Jump")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				Jump[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("Crouch")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				Crouch[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("Run")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				Run[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("CamLock")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				CamLock[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("LookUp")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				LookUp[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("LookDown")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				LookDown[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("LookLeft")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				LookLeft[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("LookRight")) != string::npos) {
			for (size_t i = 1; i < 7 && i < tokens.size(); i++)
				LookRight[i - 1] = GkAtoi(tokens[i].c_str());
		} else if ((size_t)(command_name.find("ToggleCrouch")) != string::npos) {
			if (tokens.size() > 1)
				toggleCrouch = GkAtoi(tokens[1].c_str());
		} else if ((size_t)(command_name.find("ToggleRun")) != string::npos) {
			if (tokens.size() > 1)
				toggleRun = GkAtoi(tokens[1].c_str());
		}
	}
}
std::string GkFPSCharacterController::getControlConfig() {
	using namespace std;
	string retval = "#GkFPSCharacterController Controls Configuration\n#Default Portion:\n";
	retval += "Forward|" + to_string(Forward[0]) + "|" + to_string(Forward[1]) + "|" + to_string(Forward[2]) + "|" + to_string(Forward[3]) + "|" +
			  to_string(Forward[4]) + "|" + to_string(Forward[5]) + "\n";

	retval += "Backward|" + to_string(Backward[0]) + "|" + to_string(Backward[1]) + "|" + to_string(Backward[2]) + "|" + to_string(Backward[3]) + "|" +
			  to_string(Backward[4]) + "|" + to_string(Backward[5]) + "\n";

	retval += "Left|" + to_string(Left[0]) + "|" + to_string(Left[1]) + "|" + to_string(Left[2]) + "|" + to_string(Left[3]) + "|" + to_string(Left[4]) + "|" +
			  to_string(Left[5]) + "\n";

	retval += "Right|" + to_string(Right[0]) + "|" + to_string(Right[1]) + "|" + to_string(Right[2]) + "|" + to_string(Right[3]) + "|" + to_string(Right[4]) +
			  "|" + to_string(Right[5]) + "\n";

	retval += "Jump|" + to_string(Jump[0]) + "|" + to_string(Jump[1]) + "|" + to_string(Jump[2]) + "|" + to_string(Jump[3]) + "|" + to_string(Jump[4]) + "|" +
			  to_string(Jump[5]) + "\n";

	retval += "Crouch|" + to_string(Crouch[0]) + "|" + to_string(Crouch[1]) + "|" + to_string(Crouch[2]) + "|" + to_string(Crouch[3]) + "|" +
			  to_string(Crouch[4]) + "|" + to_string(Crouch[5]) + "\n";

	retval += "Run|" + to_string(Run[0]) + "|" + to_string(Run[1]) + "|" + to_string(Run[2]) + "|" + to_string(Run[3]) + "|" + to_string(Run[4]) + "|" +
			  to_string(Run[5]) + "\n";

	retval += "CamLock|" + to_string(CamLock[0]) + "|" + to_string(CamLock[1]) + "|" + to_string(CamLock[2]) + "|" + to_string(CamLock[3]) + "|" +
			  to_string(CamLock[4]) + "|" + to_string(CamLock[5]) + "\n";

	retval += "LookUp|" + to_string(LookUp[0]) + "|" + to_string(LookUp[1]) + "|" + to_string(LookUp[2]) + "|" + to_string(LookUp[3]) + "|" +
			  to_string(LookUp[4]) + "|" + to_string(LookUp[5]) + "\n";

	retval += "LookDown|" + to_string(LookDown[0]) + "|" + to_string(LookDown[1]) + "|" + to_string(LookDown[2]) + "|" + to_string(LookDown[3]) + "|" +
			  to_string(LookDown[4]) + "|" + to_string(LookDown[5]) + "\n";

	retval += "LookLeft|" + to_string(LookLeft[0]) + "|" + to_string(LookLeft[1]) + "|" + to_string(LookLeft[2]) + "|" + to_string(LookLeft[3]) + "|" +
			  to_string(LookLeft[4]) + "|" + to_string(LookLeft[5]) + "\n";

	retval += "LookRight|" + to_string(LookRight[0]) + "|" + to_string(LookRight[1]) + "|" + to_string(LookRight[2]) + "|" + to_string(LookRight[3]) + "|" +
			  to_string(LookRight[4]) + "|" + to_string(LookRight[5]) + "\n";

	retval += "ToggleCrouch|" + to_string((int)toggleCrouch) + "\n";
	retval += "ToggleRun|" + to_string((int)toggleRun) + "\n";
	return retval;
}

// Virtual so you can add more keys/buttons/whatever. Just call
// GkFPSCharacterController::key_callback(args) before you do that.
void GkFPSCharacterController::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	// Check keys.
	int* ct = Forward;
	float* st = &(Forward_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = Backward;
	st = &(Backward_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = Left;
	st = &(Left_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = Right;
	st = &(Right_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = Jump;
	st = &(Jump_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = Crouch;
	st = &(Crouch_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = Run;
	st = &(Run_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = CamLock;
	st = &(CamLock_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = LookUp;
	st = &(LookUp_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = LookDown;
	st = &(LookDown_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = LookLeft;
	st = &(LookLeft_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;

	ct = LookRight;
	st = &(LookRight_State);
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_PRESS)
		*st = 1;
	if ((key == ct[C_KEY] || scancode == ct[C_SCANCODE]) && action == GLFW_RELEASE)
		*st = 0;
}
void GkFPSCharacterController::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	int* ct = Forward;
	float* st = &(Forward_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = Backward;
	st = &(Backward_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = Left;
	st = &(Left_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = Right;
	st = &(Right_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = Jump;
	st = &(Jump_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = Crouch;
	st = &(Crouch_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = Run;
	st = &(Run_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = CamLock;
	st = &(CamLock_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = LookUp;
	st = &(LookUp_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = LookDown;
	st = &(LookDown_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = LookLeft;
	st = &(LookLeft_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;

	ct = LookRight;
	st = &(LookRight_State);
	if (button == ct[C_BUTTON] && action == GLFW_PRESS)
		*st = 1;
	if (button == ct[C_BUTTON] && action == GLFW_RELEASE)
		*st = 0;
}
void GkFPSCharacterController::cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
														glm::vec2 NormalizedCursorDelta) {
	CDelta = CursorDelta;
}
void GkFPSCharacterController::NPCtick(btDiscreteDynamicsWorld* world, GkSoundHandler* soundHandler) {
	// STILL REQUIRES A CAMERA!!!

	// do camera lock
	if (body->RigidBodies.size() < 1 || !myCamera) // Don't even bother.
		return;
	//~ if(!isCameraLocked && inControl && CamLock_State >= 0.5 &&
	// CamLock_State_Old < 0.5) ~ {this->lockCursor();isCameraLocked = true;}
	//~ else if(isCameraLocked && inControl && CamLock_State >= 0.5 &&
	// CamLock_State_Old < 0.5) ~ {this->unLockCursor();isCameraLocked = false;}
	//~ CamLock_State_Old = CamLock_State;

	//~ if(!wasInControl && inControl){
	//~ if(isCameraLocked)
	//~ this->lockCursor();
	//~ else
	//~ this->unLockCursor();
	//~ }

	Transform bruh;
	bruh.setModel(body->getWorldTransform());
	// Set camera position
	glm::vec3 bodypos = bruh.getPos();
	(*myCamera).pos = bodypos + eyeOffset;

	// Do camera re-orienting
	if (isCameraLocked) {
		(*myCamera).pitch((CDelta.y) * lookSpeed.y);
		(*myCamera).rotateY((CDelta.x) * -lookSpeed.x);
	} else {
		(*myCamera).pitch((LookUp_State - LookDown_State) * lookKeyMult.y * -lookSpeed.y);
		(*myCamera).rotateY((LookRight_State - LookLeft_State) * lookKeyMult.x * -lookSpeed.x);
	}
	// Re-orient the body so that it displays correctly.
	glm::vec3 vec_right = (*myCamera).getRight();
	glm::vec3 vec_forward = glm::normalize(glm::cross(Up, vec_right));
	bruh.setRotQuat(faceTowardPoint(glm::vec3(0, 0, 0), vec_forward, Up));
	body->setWorldTransform(bruh);
	float SpeedMult = 1.0;
	// do running
	if (!toggleRun) {
		if (Run_State >= 0.5 && !wasCrouching)
			SpeedMult = runningMultiplier;
	} else {
		if (Run_State >= 0.5 && Run_State_Old < 0.5)
			isRunning = !isRunning;
		if (isRunning && !wasCrouching)
			SpeedMult = runningMultiplier;
	}

	if (!toggleCrouch) {
		isCrouching = !(Crouch_State < 0.5);
		Crouch_State_Old = Crouch_State;
	} else {
		if (Crouch_State >= 0.5 && Crouch_State_Old < 0.5)
			isCrouching = !isCrouching;
		Crouch_State_Old = Crouch_State;
	}
	if (!isCrouching) {
		if (wasCrouching && wasOnGround) // Move the body up
		{
			body->setPos(bodypos + (standing_spring_natural_length - crouching_spring_natural_length) * Up);
		}
		spring_natural_length = standing_spring_natural_length;
		spring_max_length = standing_spring_max_length;
		spring_k_pull = standing_spring_k_pull;
		spring_k_push = standing_spring_k_push;
	} else {
		if (!wasCrouching && wasOnGround) // Move the body down
		{
			body->setPos(bodypos - (standing_spring_natural_length - crouching_spring_natural_length) * Up);
		}
		if (wasOnGround)
			SpeedMult = crouchingMultiplier;
		spring_natural_length = crouching_spring_natural_length;
		spring_max_length = crouching_spring_max_length;
		spring_k_pull = crouching_spring_k_pull;
		spring_k_push = crouching_spring_k_push;
	}
	{
		movementThisFrame.x = Backward_State - Forward_State;
		movementThisFrame.y = Left_State - Right_State;
		movementThisFrame = glm::normalize(movementThisFrame);
		movementThisFrame *= SpeedMult;
	}
	float dist;
	float force;
	float groundfriction;
	glm::mat3x3 SpringInfo(-1, 0, 0,   0, 0, 0,   0, 0, 0);
	//~ std::cout << "\nReporting Player at:\nX: " << bodypos.x <<
	//~ "\nY: " << bodypos.y <<
	//~ "\nZ: " << bodypos.z << std::endl;
	if (framesSinceLastJump > frameJumpSafetyBuffer) {
		//~ std::cout << "\nDOING SPRING!" << std::endl;
		SpringInfo = body->applyCharacterControllerSpring(glm::vec3(RelPos), // Position in local space relative to the body
														  -Up,				 // Direction from RelPos in local space.
														  spring_k_pull,	 // k value to use when the spring is pulling.
														  spring_k_push,	 // k value to use when the spring is pushing.
														  spring_natural_length,
														  spring_max_length, // Max Distance spring can "snap on" to things.
														  world, Up);
	}
	dist = SpringInfo[0][0];
	force = SpringInfo[0][1];
	groundfriction = clampf(0.01, 0.3, SpringInfo[0][2]);
	glm::vec3 groundVel = SpringInfo[1];
	glm::vec3 relVelBody = b2g_vec3(body->RigidBodies[0]->getLinearVelocity()) - groundVel;
	btVector3 FrictionImpulse = g2b_vec3(relVelBody * -groundfriction);
	glm::vec3 groundNormal = SpringInfo[2];
	onGround = (dist > 0);
	if (onGround) {
		//~ std::cout << "\nON THE GROUND!" << std::endl;
		if (framesSinceLastJump > frameJumpSafetyBuffer) { // Don't do it if we just jumped.
			glm::vec3 velbody = b2g_vec3(body->RigidBodies[0]->getLinearVelocity());
			velbody.y *= dampening; // So that we don't have to do dampening.
			body->setVel(velbody);
		}
	} else {
		//~ std::cout << "\nIN THE AIR!" << std::endl;
	}
	framesBeenOnGround += onGround;
	framesBeenOnGround = std::min(framesBeenOnGround, framesOnGroundBeforeApplyingGroundFriction + 1);
	// Do Jumping
	framesSinceLastJump++;
	framesSinceLastJump = std::min(framesSinceLastJump, framesBetweenJumps + 5);
	if (onGround)
		airjumps = 0;
	else
		framesBeenOnGround = 0;
	if ((Jump_State >= 0.5) && (framesSinceLastJump > framesBetweenJumps) && onGround) {
		body->RigidBodies[0]->applyCentralImpulse(g2b_vec3(Up * JumpForce));
		//~ std::cout << "\nJUMP!" << std::endl;
		if (soundHandler && jumpNoise != NO_BUFFER)
			soundHandler->playSound(jumpNoise,					 // noise
									bodypos, glm::vec3(0, 0, 0), // loc and vel
									NO_SOURCE, jumpGain, RandomFloat(jumpMinPitch, jumpMaxPitch), 0.5, 1.5, 1.0, 20);
		framesSinceLastJump = 0;
	} else if ((Jump_State >= 0.5) && (framesSinceLastJump > framesBetweenJumps) && (airjumps < max_airjumps)) {
		body->RigidBodies[0]->applyCentralImpulse(g2b_vec3(Up * airJumpForce));

		//~ std::cout << "\nAIRJUMP!" << std::endl;
		if (soundHandler && jumpNoise != NO_BUFFER)
			soundHandler->playSound(airJumpNoise,				 // noise
									bodypos, glm::vec3(0, 0, 0), // loc and vel
									NO_SOURCE, airJumpGain, RandomFloat(airJumpMinPitch, airJumpMaxPitch), 0.5, 1.5, 1.0, 20);
		framesSinceLastJump = 0;
		airjumps++;
	}
	//(onGround)?GroundMovementForce:AirMovementForce
	// Actually MOVE the character.
	glm::vec3 fuc(0, 0, 0);
	fuc += vec_forward * movementThisFrame.x * ((onGround) ? GroundMovementForce : AirMovementForce);
	fuc += vec_right * movementThisFrame.y * ((onGround) ? GroundMovementForce : AirMovementForce);
	//~ std::cout << "\nFUC X:" << fuc.x <<
	//~ " Y: " << fuc.y <<
	//~ " Z: " << fuc.z << std::endl;
	btVector3 fuc2 = g2b_vec3(fuc);
	if (fuc2.x() != 0 || fuc2.y() != 0 || fuc2.z() != 0)
		body->RigidBodies[0]->applyCentralImpulse(fuc2);

	if (onGround && (framesBeenOnGround >= framesOnGroundBeforeApplyingGroundFriction)) {
		body->RigidBodies[0]->applyCentralImpulse(FrictionImpulse);
		glm::vec3 relVelBody_noY = relVelBody - Up * glm::dot(Up, relVelBody);
		if (groundSpeedCap > 0 && glm::length(relVelBody_noY) > groundSpeedCap) {
			glm::vec3 diff = groundSpeedCap * glm::normalize(relVelBody_noY) - relVelBody_noY;
			body->addVel(diff);
		}
	} else { // Not applying ground friction
		glm::vec3 velbody;
		if (airSpeedCap > 0)
			velbody = b2g_vec3(body->RigidBodies[0]->getLinearVelocity());
		velbody -= Up * glm::dot(Up, velbody); // Don't factor in Upward velocity.
		if (airSpeedCap > 0 && glm::length(velbody) > airSpeedCap) {
			glm::vec3 diff = airSpeedCap * glm::normalize(velbody) - velbody;
			body->addVel(diff);
		}
	}
	// TODO footstep sounds.
	// Accumulate distance travelled over time and play foot steps.
	// Also play footsteps when we land on the ground.

	//~ CDelta = glm::vec2(0,0); //Reset
	wasCrouching = isCrouching; // Necessary for smooth transition from crouching to not.
	wasOnGround = onGround;
	//~ wasInControl = inControl;
}
void GkFPSCharacterController::tick(btDiscreteDynamicsWorld* world, GkSoundHandler* soundHandler, bool inControl) {
	// do camera lock
	if (NPCMode) {
		NPCtick(world, soundHandler);
		return;
	}
	if (body->RigidBodies.size() < 1) // Don't even bother.
		return;
	if (!isCameraLocked && inControl && CamLock_State >= 0.5 && CamLock_State_Old < 0.5) {
		this->lockCursor();
		isCameraLocked = true;
	} else if (isCameraLocked && inControl && CamLock_State >= 0.5 && CamLock_State_Old < 0.5) {
		this->unLockCursor();
		isCameraLocked = false;
	}
	CamLock_State_Old = CamLock_State;

	if (inControl) {
		if (isCameraLocked)
			this->lockCursor();
		else
			this->unLockCursor();
	}

	Transform bruh;
	bruh.setModel(body->getWorldTransform());
	// Set camera position
	glm::vec3 bodypos = bruh.getPos();
	if (inControl)
		(*myCamera).pos = bodypos + eyeOffset;

	// Do camera re-orienting
	if (isCameraLocked && inControl) {
		(*myCamera).pitch((CDelta.y) * lookSpeed.y);
		(*myCamera).rotateY((CDelta.x) * -lookSpeed.x);
	} else if (inControl) {
		(*myCamera).pitch((LookUp_State - LookDown_State) * lookKeyMult.y * -lookSpeed.y);
		(*myCamera).rotateY((LookRight_State - LookLeft_State) * lookKeyMult.x * -lookSpeed.x);
	}
	// Re-orient the body so that it displays correctly.
	glm::vec3 vec_right = (*myCamera).getRight();
	glm::vec3 vec_forward = glm::normalize(glm::cross(Up, vec_right));
	bruh.setRotQuat(faceTowardPoint(glm::vec3(0, 0, 0), vec_forward, Up));
	if (inControl)
		body->setWorldTransform(bruh);
	float SpeedMult = 1.0;
	// do running
	if (!toggleRun) {
		if (Run_State >= 0.5 && !wasCrouching)
			SpeedMult = runningMultiplier;
	} else {
		if (Run_State >= 0.5 && Run_State_Old < 0.5)
			isRunning = !isRunning;
		if (isRunning && !wasCrouching)
			SpeedMult = runningMultiplier;
	}

	if (!toggleCrouch) {
		isCrouching = !(Crouch_State < 0.5);
		Crouch_State_Old = Crouch_State;
	} else {
		if (Crouch_State >= 0.5 && Crouch_State_Old < 0.5)
			isCrouching = !isCrouching;
		Crouch_State_Old = Crouch_State;
	}
	if (!isCrouching) {
		if (wasCrouching && wasOnGround) // Move the body up
		{
			body->setPos(bodypos + (standing_spring_natural_length - crouching_spring_natural_length) * Up);
		}
		spring_natural_length = standing_spring_natural_length;
		spring_max_length = standing_spring_max_length;
		spring_k_pull = standing_spring_k_pull;
		spring_k_push = standing_spring_k_push;
	} else {
		if (!wasCrouching && wasOnGround) // Move the body down
		{
			body->setPos(bodypos - (standing_spring_natural_length - crouching_spring_natural_length) * Up);
		}
		if (wasOnGround)
			SpeedMult = crouchingMultiplier;
		spring_natural_length = crouching_spring_natural_length;
		spring_max_length = crouching_spring_max_length;
		spring_k_pull = crouching_spring_k_pull;
		spring_k_push = crouching_spring_k_push;
	}
	if (inControl) {
		movementThisFrame.x = Backward_State - Forward_State;
		movementThisFrame.y = Left_State - Right_State;
		movementThisFrame = glm::normalize(movementThisFrame);
		movementThisFrame *= SpeedMult;
	} else {
		movementThisFrame = glm::vec2(0, 0);
	}
	float dist;
	float force;
	float groundfriction;
	glm::mat3x3 SpringInfo(-1, 0, 0,   0, 0, 0,  0,0,0);
	//~ std::cout << "\nReporting Player at:\nX: " << bodypos.x <<
	//~ "\nY: " << bodypos.y <<
	//~ "\nZ: " << bodypos.z << std::endl;
	if (framesSinceLastJump > frameJumpSafetyBuffer) {
		//~ std::cout << "\nDOING SPRING!" << std::endl;
		SpringInfo = body->applyCharacterControllerSpring(glm::vec3(RelPos), // Position in local space relative to the body
														  -Up,				 // Direction from RelPos in local space.
														  spring_k_pull,	 // k value to use when the spring is pulling.
														  spring_k_push,	 // k value to use when the spring is pushing.
														  spring_natural_length,
														  spring_max_length, // Max Distance spring can "snap on" to things.
														  world, Up);
	}
	dist = SpringInfo[0][0];
	force = SpringInfo[0][1];
	groundfriction = clampf(0.01, 0.3, SpringInfo[0][2]);
	glm::vec3 groundVel = SpringInfo[1];
	glm::vec3 relVelBody = b2g_vec3(body->RigidBodies[0]->getLinearVelocity()) - groundVel;
	btVector3 FrictionImpulse = g2b_vec3(relVelBody * -groundfriction);
	glm::vec3 groundNormal = SpringInfo[2];
	onGround = (dist > 0);
	if (onGround) {
		//~ std::cout << "\nON THE GROUND!" << std::endl;
		if (framesSinceLastJump > frameJumpSafetyBuffer) { // Don't do it if we just jumped.
			glm::vec3 velbody = b2g_vec3(body->RigidBodies[0]->getLinearVelocity());
			velbody.y *= dampening; // So that we don't have to do dampening.
			body->setVel(velbody);
		}
	} else {
		//~ std::cout << "\nIN THE AIR!" << std::endl;
	}
	framesBeenOnGround += onGround;
	framesBeenOnGround = std::min(framesBeenOnGround, framesOnGroundBeforeApplyingGroundFriction + 1);
	// Do Jumping
	framesSinceLastJump++;
	framesSinceLastJump = std::min(framesSinceLastJump, framesBetweenJumps + 5);
	if (onGround)
		airjumps = 0;
	else
		framesBeenOnGround = 0;
	if ((Jump_State >= 0.5 && Jump_State_Old <= 0.5) && inControl && (framesSinceLastJump > framesBetweenJumps) && onGround) {
		body->RigidBodies[0]->applyCentralImpulse(g2b_vec3(Up * JumpForce));
		//~ std::cout << "\nJUMP!" << std::endl;
		if (soundHandler && jumpNoise != NO_BUFFER)
			soundHandler->playSound(jumpNoise,					 // noise
									bodypos, glm::vec3(0, 0, 0), // loc and vel
									NO_SOURCE, jumpGain, RandomFloat(jumpMinPitch, jumpMaxPitch), 0.5, 1.5, 1.0, 20);
		framesSinceLastJump = 0;
	} else if ((Jump_State >= 0.5 && Jump_State_Old <= 0.5) && inControl && (framesSinceLastJump > framesBetweenJumps) && (airjumps < max_airjumps)) {
		body->RigidBodies[0]->applyCentralImpulse(g2b_vec3(Up * airJumpForce));

		//~ std::cout << "\nAIRJUMP!" << std::endl;
		if (soundHandler && jumpNoise != NO_BUFFER)
			soundHandler->playSound(airJumpNoise,				 // noise
									bodypos, glm::vec3(0, 0, 0), // loc and vel
									NO_SOURCE, airJumpGain, RandomFloat(airJumpMinPitch, airJumpMaxPitch), 0.5, 1.5, 1.0, 20);
		framesSinceLastJump = 0;
		airjumps++;
	}
	//(onGround)?GroundMovementForce:AirMovementForce
	// Actually MOVE the character.
	glm::vec3 fuc(0, 0, 0);
	fuc += vec_forward * movementThisFrame.x * ((onGround) ? GroundMovementForce : AirMovementForce);
	fuc += vec_right * movementThisFrame.y * ((onGround) ? GroundMovementForce : AirMovementForce);
	//~ std::cout << "\nFUC X:" << fuc.x <<
	//~ " Y: " << fuc.y <<
	//~ " Z: " << fuc.z << std::endl;
	btVector3 fuc2 = g2b_vec3(fuc);
	if (inControl)
		if (fuc2.x() != 0 || fuc2.y() != 0 || fuc2.z() != 0)
			body->RigidBodies[0]->applyCentralImpulse(fuc2);

	if (onGround && (framesBeenOnGround >= framesOnGroundBeforeApplyingGroundFriction)) {
		body->RigidBodies[0]->applyCentralImpulse(FrictionImpulse);
		glm::vec3 relVelBody_noY = relVelBody - Up * glm::dot(Up, relVelBody);
		if (groundSpeedCap > 0 && glm::length(relVelBody_noY) > groundSpeedCap) {
			glm::vec3 diff = groundSpeedCap * glm::normalize(relVelBody_noY) - relVelBody_noY;
			body->addVel(diff);
		}
	} else { // Not applying ground friction
		glm::vec3 velbody;
		if (airSpeedCap > 0)
			velbody = b2g_vec3(body->RigidBodies[0]->getLinearVelocity());
		velbody -= Up * glm::dot(Up, velbody); // Don't factor in Upward velocity.
		if (airSpeedCap > 0 && glm::length(velbody) > airSpeedCap) {
			glm::vec3 diff = airSpeedCap * glm::normalize(velbody) - velbody;
			body->addVel(diff);
		}
	}
	// TODO footstep sounds.

	CDelta = glm::vec2(0, 0);   // Reset
	wasCrouching = isCrouching; // Necessary for smooth transition from crouching to not.
	wasOnGround = onGround;
	wasInControl = inControl;
	Jump_State_Old = Jump_State;
}
bool GkFPSCharacterController::isOnGround() { return onGround; }
void GkFPSCharacterController::lockCursor() {
	std::cout << "\nPlease add your cursor lock code to your class inheriting from "
			  << "GkFPSCharacterController...\nvoid lockCursor()\n I suggest using: "
			  << "getDevice()->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_DISABLED);" << std::endl;
}

void GkFPSCharacterController::unLockCursor() {
	std::cout << "\nPlease add your cursor unlock code to your class inheriting from "
			  << "GkFPSCharacterController...\nvoid unLockCursor()\n I suggest using: "
			  << "getDevice()->setInputMode(0, GLFW_CURSOR, GLFW_CURSOR_NORMAL);" << std::endl;
}

std::string GkFPSCharacterController::SerializeGkMap(std::string savepath) {
	// First, write all of our important properties to ints and floats
	// so that the exporter can export them.
	//~ writeToMaps();
	//~ std::string retval =  // Save int and float properties.
	//~ ints.clear();
	//~ floats.clear();
	return GkEntityComponent::SerializeGkMap(savepath);
}

void GkFPSCharacterController::writeToMaps() {
	for (uint i = 0; i < 6; i++) {
		ints["Forward[" + std::to_string(i) + "]"] = Forward[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["Backward[" + std::to_string(i) + "]"] = Backward[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["Left[" + std::to_string(i) + "]"] = Left[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["Right[" + std::to_string(i) + "]"] = Right[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["Jump[" + std::to_string(i) + "]"] = Jump[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["Crouch[" + std::to_string(i) + "]"] = Crouch[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["Run[" + std::to_string(i) + "]"] = Run[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["CamLock[" + std::to_string(i) + "]"] = CamLock[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["LookUp[" + std::to_string(i) + "]"] = LookUp[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["LookDown[" + std::to_string(i) + "]"] = LookDown[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["LookLeft[" + std::to_string(i) + "]"] = LookLeft[i];
	}
	for (uint i = 0; i < 6; i++) {
		ints["LookRight[" + std::to_string(i) + "]"] = LookRight[i];
	}
	floats["eyeOffset.x"] = eyeOffset.x;
	floats["eyeOffset.y"] = eyeOffset.y;
	floats["eyeOffset.z"] = eyeOffset.z;
	floats["lookSpeed.x"] = lookSpeed.x;
	floats["lookSpeed.y"] = lookSpeed.y;
	floats["lookKeyMult.x"] = lookKeyMult.x;
	floats["lookKeyMult.y"] = lookKeyMult.y;

	floats["RelPos.x"] = RelPos.x;
	floats["RelPos.y"] = RelPos.y;
	floats["RelPos.z"] = RelPos.z;

	floats["Up.x"] = Up.x;
	floats["Up.y"] = Up.y;
	floats["Up.z"] = Up.z;

	floats["GroundMovementForce"] = GroundMovementForce;
	floats["runningMultiplier"] = runningMultiplier;
	floats["crouchingMultiplier"] = crouchingMultiplier;
	floats["AirMovementForce"] = AirMovementForce;
	floats["groundSpeedCap"] = groundSpeedCap;
	floats["airSpeedCap"] = airSpeedCap;
	floats["JumpForce"] = JumpForce;
	floats["airJumpForce"] = airJumpForce;
	floats["dampening"] = dampening;
	floats["spring_natural_length"] = spring_natural_length;
	floats["spring_max_length"] = spring_max_length;
	floats["spring_k_pull"] = spring_k_pull;
	floats["spring_k_push"] = spring_k_push;
	floats["standing_spring_natural_length"] = standing_spring_natural_length;
	floats["standing_spring_max_length"] = standing_spring_max_length;
	floats["standing_spring_k_pull"] = standing_spring_k_pull;
	floats["standing_spring_k_push"] = standing_spring_k_push;
	floats["crouching_spring_natural_length"] = crouching_spring_natural_length;
	floats["crouching_spring_max_length"] = crouching_spring_max_length;
	floats["crouching_spring_k_pull"] = crouching_spring_k_pull;
	floats["crouching_spring_k_push"] = crouching_spring_k_push;

	ints["onGround"] = onGround; // Bool properties should export as int.
	// ints["isCameraLocked"] = isCameraLocked; //We don't want to export this property.
	ints["isRunning"] = isRunning;
	ints["toggleCrouch"] = toggleCrouch;
	ints["toggleRun"] = toggleRun;
	ints["isCrouching"] = isCrouching;
	ints["wasCrouching"] = wasCrouching;
	ints["wasOnGround"] = wasOnGround;
	ints["framesOnGroundBeforeApplyingGroundFriction"] = framesOnGroundBeforeApplyingGroundFriction;
	floats["partialGroundFriction"] = partialGroundFriction;
	ints["framesBeenOnGround"] = framesBeenOnGround;
	ints["framesSinceLastJump"] = framesSinceLastJump;
	ints["framesBetweenJumps"] = framesBetweenJumps;
	ints["frameJumpSafetyBuffer"] = frameJumpSafetyBuffer;
	ints["max_airjumps"] = max_airjumps;
	ints["airjumps"] = airjumps;
}
void GkFPSCharacterController::readFromMaps() {
	for (uint i = 0; i < 6; i++) {
		Forward[i] = ints["Forward[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		Backward[i] = ints["Backward[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		Left[i] = ints["Left[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		Right[i] = ints["Right[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		Jump[i] = ints["Jump[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		Crouch[i] = ints["Crouch[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		Run[i] = ints["Run[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		CamLock[i] = ints["CamLock[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		LookUp[i] = ints["LookUp[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		LookDown[i] = ints["LookDown[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		LookLeft[i] = ints["LookLeft[" + std::to_string(i) + "]"];
	}
	for (uint i = 0; i < 6; i++) {
		LookRight[i] = ints["LookRight[" + std::to_string(i) + "]"];
	}
	eyeOffset.x = floats["eyeOffset.x"];
	eyeOffset.y = floats["eyeOffset.y"];
	eyeOffset.z = floats["eyeOffset.z"];
	lookSpeed.x = floats["lookSpeed.x"];
	lookSpeed.y = floats["lookSpeed.y"];
	lookKeyMult.x = floats["lookKeyMult.x"];
	lookKeyMult.y = floats["lookKeyMult.y"];

	RelPos.x = floats["RelPos.x"];
	RelPos.y = floats["RelPos.y"];
	RelPos.z = floats["RelPos.z"];

	Up.x = floats["Up.x"];
	Up.y = floats["Up.y"];
	Up.z = floats["Up.z"];

	GroundMovementForce = floats["GroundMovementForce"];
	runningMultiplier = floats["runningMultiplier"];
	crouchingMultiplier = floats["crouchingMultiplier"];
	AirMovementForce = floats["AirMovementForce"];
	groundSpeedCap = floats["groundSpeedCap"];
	airSpeedCap = floats["airSpeedCap"];
	JumpForce = floats["JumpForce"];
	airJumpForce = floats["airJumpForce"];
	dampening = floats["dampening"];
	spring_natural_length = floats["spring_natural_length"];
	spring_max_length = floats["spring_max_length"];
	spring_k_pull = floats["spring_k_pull"];
	spring_k_push = floats["spring_k_push"];
	standing_spring_natural_length = floats["standing_spring_natural_length"];
	standing_spring_max_length = floats["standing_spring_max_length"];
	standing_spring_k_pull = floats["standing_spring_k_pull"];
	standing_spring_k_push = floats["standing_spring_k_push"];
	crouching_spring_natural_length = floats["crouching_spring_natural_length"];
	crouching_spring_max_length = floats["crouching_spring_max_length"];
	crouching_spring_k_pull = floats["crouching_spring_k_pull"];
	crouching_spring_k_push = floats["crouching_spring_k_push"];

	onGround = ints["onGround"]; // Bool properties should export as int.
	// ints["isCameraLocked"] = isCameraLocked; //We don't want to import this property.
	isRunning = ints["isRunning"];
	toggleCrouch = ints["toggleCrouch"];
	toggleRun = ints["toggleRun"];
	isCrouching = ints["isCrouching"];
	wasCrouching = ints["wasCrouching"];
	wasOnGround = ints["wasOnGround"];
	framesOnGroundBeforeApplyingGroundFriction = ints["framesOnGroundBeforeApplyingGroundFriction"];
	partialGroundFriction = floats["partialGroundFriction"];
	framesBeenOnGround = ints["framesBeenOnGround"];
	framesSinceLastJump = ints["framesSinceLastJump"];
	framesBetweenJumps = ints["framesBetweenJumps"];
	frameJumpSafetyBuffer = ints["frameJumpSafetyBuffer"];
	max_airjumps = ints["max_airjumps"];
	airjumps = ints["airjumps"];
	ints.clear();
	floats.clear();
}
bool GkFPSCharacterController::LoadGkMap(std::stringstream& file, std::string& line, std::vector<std::string>& tokens, std::string& savedir,
										 std::string& savefile) {
	//~ if (GkEntityComponent::LoadGkMap(file, line, tokens, savedir, savefile)) {
		//~ readFromMaps();
		//~ return true;
	//~ }
	return GkEntityComponent::LoadGkMap(file, line, tokens, savedir, savefile);
}
// GkSoundHandler functions

GkSoundHandler::GkSoundHandler(uint num_sound_sources, uint num_stream_sources) { construct(num_sound_sources, num_stream_sources); }
void GkSoundHandler::construct(uint num_sound_sources, uint num_stream_sources) {
	if (!isNull) {
		alDeleteSources(sources_sounds.size(), &(sources_sounds[0]));
		alDeleteSources(sources_streams.size(), &(sources_streams[0]));
	}
	isNull = false;
	while (sources_sounds.size() < num_sound_sources)
		sources_sounds.push_back(0);
	while (sources_streams.size() < num_stream_sources)
		sources_streams.push_back(0);
	alGenSources(num_sound_sources, &(sources_sounds[0]));
	alGenSources(num_stream_sources, &(sources_streams[0]));
	ALenum error = alGetError();
	if (error != AL_NO_ERROR) {
		std::cout << "\nOPENAL ERROR!" << std::endl;
		if (error == AL_INVALID_NAME)
			std::cout << "Invalid Name" << std::endl;
		if (error == AL_INVALID_ENUM)
			std::cout << "Invalid Enum" << std::endl;
		if (error == AL_INVALID_VALUE)
			std::cout << "Invalid Value" << std::endl;
		if (error == AL_INVALID_OPERATION)
			std::cout << "Invalid Operation" << std::endl;
		if (error == AL_OUT_OF_MEMORY)
			std::cout << "Out of Memory" << std::endl;
	}
}
GkSoundHandler::~GkSoundHandler() {
	if (!isNull) {
		alDeleteSources(sources_sounds.size(), &(sources_sounds[0]));
		alDeleteSources(sources_streams.size(), &(sources_streams[0]));
	}
}
// If a source is specified it'll play the buffer on the source from the
// beginning. If no buffer is specified, it will only update the location and
// velocity. if no source, it will pick a source that isn't paused or playing.
// If there are no available sources? it will not play the sound and return
// source Returns the source Uint it modified.
ALuint GkSoundHandler::playSound(ALuint soundbuffer, glm::vec3 loc, glm::vec3 vel, ALuint source, float gain, float pitch, float mingain, float maxgain,
								 float rolloff_factor, float max_distance, bool looping) {
	if (isNull)
		return source;
	int state;
	bool specifiedSource = alIsSource(source);
	if (!specifiedSource) // Means we gotta find one
		for (auto& s : sources_sounds)
			if (alIsSource(s)) // Dont use an invalid one
			{
				alGetSourcei(s, AL_SOURCE_STATE, &state);
				if (state == AL_INITIAL || state == AL_STOPPED) {
					source = s;
					break;
				}
			}
	if (alIsBuffer(soundbuffer) && alIsSource(source))
		alSourcei(source, AL_BUFFER, soundbuffer);
	if (alIsSource(source)) {
		alSource3f(source, AL_POSITION, loc.x, loc.y, loc.z);
		alSource3f(source, AL_VELOCITY, vel.x, vel.y, vel.z);
		alSourcef(source, AL_GAIN, gain);
		alSourcef(source, AL_PITCH, pitch);
		alSourcef(source, AL_MIN_GAIN, mingain);
		alSourcef(source, AL_MAX_GAIN, maxgain);
		alSourcef(source, AL_ROLLOFF_FACTOR, rolloff_factor);
		alSourcef(source, AL_MAX_DISTANCE, max_distance);
		alSourcei(source, AL_LOOPING, (looping) ? AL_TRUE : AL_FALSE);
	}
	if (alIsBuffer(soundbuffer) && alIsSource(source))
		alSourcePlay(source);
	return source;
}
// Does exact same thing as above but enqueus streams onto a source.

ALuint GkSoundHandler::QueueAndPlayStream(ALuint soundbuffer, glm::vec3 loc, glm::vec3 vel, ALuint source, float gain, float pitch, float mingain,
										  float maxgain, float rolloff_factor, float max_distance, bool looping) {
	if (isNull)
		return source;
	int state;
	bool specifiedSource = alIsSource(source);
	if (!specifiedSource) // Means we gotta find one
		for (auto& s : sources_streams)
			if (alIsSource(s)) // Dont use an invalid one
			{
				alGetSourcei(s, AL_SOURCE_STATE, &state);
				if (state == AL_INITIAL || state == AL_STOPPED) {
					source = s;
					break;
				}
			}
	if (alIsBuffer(soundbuffer) && alIsSource(source))
		alSourceQueueBuffers(source, 1, &soundbuffer);
	if (alIsSource(source)) {
		alSource3f(source, AL_POSITION, loc.x, loc.y, loc.z);
		alSource3f(source, AL_VELOCITY, vel.x, vel.y, vel.z);
		alSourcef(source, AL_GAIN, gain);
		alSourcef(source, AL_PITCH, pitch);
		alSourcef(source, AL_MIN_GAIN, mingain);
		alSourcef(source, AL_MAX_GAIN, maxgain);
		alSourcef(source, AL_ROLLOFF_FACTOR, rolloff_factor);
		alSourcef(source, AL_MAX_DISTANCE, max_distance);
		alSourcei(source, AL_LOOPING, (looping) ? AL_TRUE : AL_FALSE);
	}
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	if (alIsBuffer(soundbuffer) && alIsSource(source) && !(state == AL_PLAYING))
		alSourcePlay(source);
	return source;
}
// For when you pause the game.
void GkSoundHandler::pauseAllSound() {
	if (isNull)
		return;
	int state;
	for (ALuint source : sources_sounds) {
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING)
			alSourcePause(source);
	}
	for (ALuint source : sources_streams) {
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING)
			alSourcePause(source);
	}
}
void GkSoundHandler::resumeAllSound() {
	if (isNull)
		return;
	int state;
	for (ALuint source : sources_sounds) {
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state == AL_PAUSED)
			alSourcePlay(source);
	}
	for (ALuint source : sources_streams) {
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state == AL_PAUSED)
			alSourcePlay(source);
	}
}
// GkEntityComponent crap
bool GkEntityComponent::canUseProcessor(void* processor) {
	if (!processor) {
		return false;
	}
	return true; // Default behavior of a GkEntityComponent.
}
void GkEntityComponent::destruct() { deregisterFromProcessor(); }

void GkEntityComponent::deregisterFromProcessor() {
	if (myProcessor)
		((GkEntityComponentProcessor*)myProcessor)->deregisterComponent(this);
	myProcessor = nullptr;
}

std::string GkEntityComponent::SerializeGkMap(std::string savepath) {
	size_t objID = 0;
	size_t index = 0;
	writeToMaps();
	std::vector<GkEntityComponent*>& dads_owned_entity_components = ((GkObject*)GkObjectParent)->OwnedEntityComponents;
	objID = ((GkObject*)GkObjectParent)->getID();
	for(size_t i = 0; i < dads_owned_entity_components.size(); i++)
		if(dads_owned_entity_components[i] == this)
			index = i;
	std::string RetVal = "\nSTARTECPROPERTIES|" + std::to_string(objID) + "|" + std::to_string(index);
	for (auto it = floats.begin(); it != floats.end(); it++) {
		RetVal += "\nECFLOAT|" + it->first + "|" + std::to_string(it->second);
	}
	for (auto it = ints.begin(); it != ints.end(); it++) {
		RetVal += "\nECINT|" + it->first + "|" + std::to_string(it->second);
	}
	for (auto it = strings.begin(); it != strings.end(); it++) {
		RetVal += "\nECSTRING|" + it->first + "|" + it->second;
	}
	RetVal += "\nENDECPROPERTIES|" + getClassName() + "\n";
	return RetVal;
}

bool GkEntityComponent::LoadGkMap(std::stringstream& file, std::string& line, std::vector<std::string>& tokens, std::string& savedir, std::string& savefile) {
	if (tkCmd(tokens[0], "STARTECPROPERTIES")) { // Starting EC properties
		// Check if this command applies to us at all. Needs to be the correct Object ID as well as
		// the correct index.
		//~ size_t objID = 0, index = 0;
		//~ if(GkObjectParent)
		//~ {
		//~ objID = ((GkObject*)GkObjectParent)->getID();
		//~ GkObject* dad = ((GkObject*)GkObjectParent);
		//~ bool inThere = 0;
		//~ for(size_t i = 0; i < dad->OwnedEntityComponents; i++)
		//~ if(dad->OwnedEntityComponents == this)
		//~ {inThere = true; index = i;}
		//~ if (!inThere)
		//~ return false;
		//~ }
		//~ else
		//~ return false;
		//~ //Now perform the check
		//~ if(
		//~ (tokens.size() < 2) ||
		//~ (
		//~ objID != GkAtoui(tokens[1].c_str())
		//~ || index != GkAtoui(tokens[2].c_str())
		//~ )
		//~ ) return false;
		// Else, continue.
		std::getline(file, line);
		tokens = SplitString(line, '|');
		while (file.good() && !tkCmd(tokens[0], "ENDECPROPERTIES")) {
			if (tkCmd(tokens[0], "ECFLOAT")) {
				if (tokens.size() > 2)
					floats[tokens[1]] = GkAtof(tokens[2].c_str());
			} else if (tkCmd(tokens[0], "ECINT")) {
				if (tokens.size() > 2)
					ints[tokens[1]] = GkAtoi(tokens[2].c_str());
			} else if (tkCmd(tokens[0], "ECSTRING")){
				if (tokens.size() > 2)
					strings[tokens[1]] = tokens[2];
			}
			std::getline(file, line);
			tokens = SplitString(line, '|');
		}
		readFromMaps();
		return true;
	}
	return false;
}

void GkEntityComponentProcessor::registerComponent(GkEntityComponent* comp) {
	if (!comp) {
		return;
	}
	if (!(comp->canUseProcessor(this))) {return;}
	comp->myProcessor = (void*)this;
	for (GkEntityComponent*& item : RegisteredComponents)
		if (!item) {
			//~ std::cout << "\nAdded EC." << std::endl;
			item = comp;
			return;
		}
	RegisteredComponents.push_back(comp);
	//~ std::cout << "\nAdded EC, pushed back" << std::endl;
}
/// Deregister an entity component from this processor.
void GkEntityComponentProcessor::deregisterComponent(GkEntityComponent* comp) {
	if (comp) {
		for (GkEntityComponent*& item : RegisteredComponents)
			if (item == comp) {
				//~ std::cout << "\nDeRegistered EC from ECSP." << std::endl;
				item = nullptr;
			}
	}
}

// GkAnimPlayback stuff

void GkAnimPlayer::MidFrameProcess() {
	for (AnimPlaybackState& PS : animPlaybacks)
		if (PS.animation)
			PS.tick(frameTime);
}
void GkAnimPlayer::startAnimPlayback(std::string anim_name, float playspeed, float maxTime, float Offset, bool indefinite, uint priority) {
	if (!((GkObject*)GkObjectParent)->Animations)
		return; // Doesn't have animations
	if (priority == UINT_MAX) {
		for (priority = 0; priority < GK_MAX_PLAYING_ANIMATIONS; priority++)
			if (!animPlaybacks[priority].animation)
				break;
		if (priority == UINT_MAX)
			return;
	}
	if (((GkObject*)GkObjectParent)->Animations->count(anim_name) == 0)
		return;
	auto& PS = animPlaybacks[priority];
	PS.animation = &((*(((GkObject*)GkObjectParent)->Animations))[anim_name]);
	PS.elapsedTime = Offset;
	PS.playbackSpeed = playspeed;
	PS.maxElapsed = maxTime;
	PS.indefinite = indefinite;
}
void GkAnimPlayer::haltAllAnimations(){
	for (AnimPlaybackState& PS : animPlaybacks)
		PS.animation = nullptr;
}
/// Apply animations.
void GkAnimPlayer::SeparatedProcess() {
	for (AnimPlaybackState& PS : animPlaybacks)
		if (PS.animation) // Still playing
		{
			if ((PS.playbackSpeed > 0 && (PS.elapsedTime >= PS.maxElapsed)) || (PS.playbackSpeed < 0 && (PS.elapsedTime <= PS.maxElapsed))) {
				PS.animation = nullptr;
				continue;
			} // Halt animation.
			// Apply animation
			((GkObject*)GkObjectParent)->setAnimation(*PS.animation, PS.elapsedTime);
		}
}
//GkVoxel stuff~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void GkVoxelChunk::destruct(){
	//~ std::cout << "\nMARKER 1" << std::endl;
	if(myRigidBody)
		world->removeRigidBody(myRigidBody);
	if(myMesh)
		theScene->deregisterMesh(myMesh);
	//~ std::cout << "\nMARKER 2" << std::endl;
	if(myMesh) delete myMesh; myMesh = nullptr;
	if(myRigidBody)
	{
		delete myRigidBody->getMotionState();
		delete myRigidBody;
	} myRigidBody = nullptr;
	//~ std::cout << "\nMARKER 3" << std::endl;
	if(myColShape)
	{
		delete myColShape->getMeshInterface();
		delete myColShape;
	} myColShape = nullptr;
	//~ std::cout << "\nMARKER 4" << std::endl;
	if(voxelData) free(voxelData); voxelData = nullptr;
	//~ std::cout << "\nMARKER 5" << std::endl;
}
GkVoxelChunk::GkVoxelChunk(
		uint _xdim, uint _ydim, uint _zdim, 
		GkScene* _theScene, 
		btDiscreteDynamicsWorld* _world,
		Texture* _Tex,
		Transform _worldTransform,
		float _voxelScale,
		std::vector<glm::vec3>* _colors
		
){
	voxelScale = _voxelScale;
	worldTransform = _worldTransform;
	world = _world;
	theScene = _theScene;
	xdim = _xdim; ydim = _ydim; zdim = _zdim;
	colors = _colors; 
	Tex = _Tex;
	useColors = (!(Tex)) && colors;
	voxelData = (unsigned char*)malloc(xdim * ydim * zdim);
	for(size_t i = 0; i < xdim * ydim * zdim; i++)
		voxelData[i] = 0; //Using 1 for testing.
	myMesh = new Mesh(IndexedModel(), false, true, false);
	colInfo.userData = "GkVoxelChunk";
	theScene->registerMesh(myMesh);
	myMesh->registerInstance(&myMeshInstance);
	if(Tex)
		myMesh->pushTexture(SafeTexture(Tex));
	
	update();
}
void GkVoxelChunk::update(){
	//~ std::cout << "\nEntered Update!" << std::endl;
	if(!needsUpdate) return;
	myMeshInstance.myTransform = worldTransform;
	IndexedModel b = Voxel2Model(voxelData, xdim, ydim, zdim, colors);
	//~ std::cout << "\nCreated Voxel Model!" << std::endl;
	if(voxelScale != 1.0f)
		b.applyTransform(
			Transform(
				glm::vec3(0,0,0),
				glm::vec3(0,0,0), 
				glm::vec3(voxelScale, voxelScale, voxelScale)
			).getModel()
		);
	
	
	if(myRigidBody)
	{	
		world->removeRigidBody(myRigidBody);
		delete myRigidBody->getMotionState();
		delete myRigidBody;
	} myRigidBody = nullptr;
	if(myColShape)
	{
		delete myColShape->getMeshInterface();
		delete myColShape;
	} myColShape = nullptr;
	if(b.indices.size() < 3) { //NOT A SINGLE TRI
		myMeshInstance.shouldRender = false;
		return;
	} else {
		//~ std::cout << "\nIt has indices!" << std::endl;
		myMesh->reShapeMesh(b);
		if(useColors)
			myMesh->renderflags = GK_RENDER | GK_COLORED | GK_COLOR_IS_BASE | GK_TINT;
		else
			myMesh->renderflags = GK_RENDER | GK_TEXTURED | GK_TINT;
		myMeshInstance.shouldRender = true;
	} 
	btTriangleMesh* bt_b = makeTriangleMesh(b);
	myColShape = new btBvhTriangleMeshShape(bt_b, false);
	btTransform t; t = g2b_transform(worldTransform.getModel());
	btMotionState* motion = new btDefaultMotionState(t);
	btVector3 inertia(0,0,0);
	btRigidBody::btRigidBodyConstructionInfo info(0, motion, myColShape, inertia);
	myRigidBody = new btRigidBody(info);
	myRigidBody->setUserPointer((void*)&colInfo);
	world->addRigidBody(myRigidBody, COL_NORMAL | COL_HITSCAN, COL_NORMAL | COL_CHARACTERS);
	needsUpdate = false;
}
void GkVoxelChunk::Serialize(std::string chunkfilename){
	std::ofstream file;
	file.open(chunkfilename, std::ios::trunc);
	if(file.is_open())
		for(size_t i = 0; i < xdim * ydim * zdim; i++)
			file << std::to_string((uint)(voxelData[i])) << "\n";
	else
		std::cout << "\nERROR!!! Chunk failed to save. Cannot open file " << chunkfilename << std::endl;
}
void GkVoxelChunk::Load(std::string chunkfilename){
	std::ifstream file;
	file.open(chunkfilename);
	std::string line;
	if(file.is_open()){
		for(size_t i = 0; i < xdim * ydim * zdim; i++)
			if(file.good())
			{
				std::getline(file, line);
				voxelData[i] = (unsigned char)GkAtoui(line.c_str());
			}
	} else
	std::cout << "\nERROR!!! Chunk failed to load. Cannot open file " << chunkfilename << std::endl;
}

//GkVoxelWorld stuff

GkVoxelWorld::GkVoxelWorld(
	uint _chunk_xdim, uint _chunk_ydim, uint _chunk_zdim, 
	uint _xnumchunks, uint _ynumchunks, uint _znumchunks,
	GkScene* _theScene, 
	btDiscreteDynamicsWorld* _world,
	Texture* _Tex,
	float _voxelScale,
	const std::vector<glm::vec3>& _colors,
	int _meshmask
){
	chunk_xdim = _chunk_xdim;
	chunk_ydim = _chunk_ydim;
	chunk_zdim = _chunk_zdim;
	xnumchunks = _xnumchunks;
	ynumchunks = _ynumchunks;
	znumchunks = _znumchunks;
	theScene = _theScene;
	world = _world;
	Tex = _Tex;
	voxelScale = _voxelScale;
	colors = _colors;
	meshmask = _meshmask;
	while(chunks.size() > 0)
	{
		delete chunks.back();
		chunks.erase(chunks.end() - 1);
	}
	while(chunks.size() < xnumchunks * ynumchunks * znumchunks)chunks.push_back(nullptr);
	for(uint x = 0; x < xnumchunks; x++)
	for(uint y = 0; y < ynumchunks; y++)
	for(uint z = 0; z < znumchunks; z++)
	{
		Transform t(
			glm::vec3(
				x * chunk_xdim * voxelScale,
				y * chunk_ydim * voxelScale,
				z * chunk_zdim * voxelScale
			),
			glm::vec3(0,0,0),
			glm::vec3(1,1,1)
		);
		chunks[x + y * xnumchunks + z * xnumchunks * ynumchunks] = 
		new GkVoxelChunk(chunk_xdim, chunk_ydim, chunk_zdim, 
			theScene, world, 
			Tex, t, voxelScale, &colors
		);
		chunks[x + y * xnumchunks + z * xnumchunks * ynumchunks]->myMeshInstance.mymeshmask = meshmask;
		chunks[x + y * xnumchunks + z * xnumchunks * ynumchunks]->myMesh->mesh_meshmask = meshmask;
	}
}
GkVoxelWorld::~GkVoxelWorld(){destruct();}
void GkVoxelWorld::Save(std::string savedir){
	std::string dirname = savedir + "VoxelWorldChunks/";
	MakeDir(dirname.c_str());
	for(size_t i = 0; i < chunks.size(); i++)
	{
		std::string fn = dirname + "Chunk_" + std::to_string(i) + ".txt";
		chunks[i]->Serialize(fn);
	}
}
void GkVoxelWorld::Load(std::string savedir){
	std::string dirname = savedir + "VoxelWorldChunks/";
	for(size_t i = 0; i < chunks.size(); i++)
	{
		std::string fn = dirname + "Chunk_" + std::to_string(i) + ".txt";
		chunks[i]->Load(fn);
	}
}
void GkVoxelWorld::destruct(){
	while(chunks.size() > 0)
	{
		delete chunks.back();
		chunks.erase(chunks.end() - 1);
	}
}
void GkVoxelWorld::update(){
	//TODO update world transform of chunks from our transform.
	for(auto* item: chunks)
		if(item)
			item->update();
}
void GkVoxelWorld::setBlock (uint x, uint y, uint z, unsigned char value){
	uint chunkwise_x = x/chunk_xdim;
	uint chunkwise_y = y/chunk_ydim;
	uint chunkwise_z = z/chunk_zdim;
	if(chunkwise_x + chunkwise_y * xnumchunks + chunkwise_z * xnumchunks * ynumchunks >= chunks.size())
		return;
	uint chunkx = x%chunk_xdim;
	uint chunky = x%chunk_ydim;
	uint chunkz = x%chunk_zdim;
	chunks[chunkwise_x + chunkwise_y * xnumchunks + chunkwise_z * xnumchunks * ynumchunks]
		->getBlock(chunkx, chunky, chunkz) = value;
	chunks[chunkwise_x + chunkwise_y * xnumchunks + chunkwise_z * xnumchunks * ynumchunks]
		->needsUpdate = true;
}
void GkVoxelWorld::setPhongMaterial(Phong_Material phong){
	for(auto*& chunk: chunks) if(chunk)
		chunk->myMeshInstance.myPhong = phong;
}
void GkUIRenderingThread::Execute() {
		if (!myEngine)
			return;
		BMPFontRenderer* myFont = ((GkEngine*)myEngine)->getFont();
		if (!myFont)
			return;
		myFont->clearScreen(0, 0, 0, 0);
		for (auto*& i : ((GkEngine*)myEngine)->subWindows)
			if (i)
				i->draw(myFont);
	}
	
static pthread_mutex_t stdoutlock = PTHREAD_MUTEX_INITIALIZER;
void lock_stdout() { pthread_mutex_lock(&stdoutlock); }
void unLock_stdout() { pthread_mutex_unlock(&stdoutlock); }

/*
void* GkLockStepThread::thread_func(void* me_void) {
	GkLockStepThread* me = (GkLockStepThread*)me_void; // For convenience.
	int ret;
	if (!me)
		pthread_exit(nullptr);
	while (!(me->shouldKillThread)) {
		ret = pthread_cond_wait(&(me->myCond), &(me->myMutex));
		if (!(me->shouldKillThread))
			me->Execute();
	}
	pthread_exit(nullptr);
}
*/
void* GkLockStepThread::thread_func(void* me_void) {
	GkLockStepThread* me = (GkLockStepThread*)me_void;
	int ret = 0;
	if (!me)pthread_exit(NULL);
	//if(!me->execute)pthread_exit(NULL);
	while (1) {
		//ret = pthread_cond_wait(&(me->myCond), &(me->myMutex));
		pthread_barrier_wait(&me->myBarrier);
		//puts("\nTHREAD ACTIVATING...");
		pthread_mutex_lock(&me->myMutex);
		//puts("\nTHREAD ACTIVATED");
		//if(ret)pthread_exit(NULL);
		if (!(me->shouldKillThread))
			me->Execute();
		else if(me->shouldKillThread){
			pthread_mutex_unlock(&me->myMutex);
			//puts("\nTHREAD DYING...");
			//pthread_barrier_wait(&me->myBarrier);
			//puts("\nTHREAD DED!");
			pthread_exit(NULL);
		}
		//puts("\nTHREAD DEACTIVATING...");
		pthread_mutex_unlock(&me->myMutex);
		//puts("\nTHREAD DEACTIVATED");
		pthread_barrier_wait(&me->myBarrier);
		//puts("\nTIME FOR A NEW CYCLE...");
	}
	pthread_exit(NULL);
}

}; // namespace GSGE
