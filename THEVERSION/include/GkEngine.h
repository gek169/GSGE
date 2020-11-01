#ifndef GKENGINE_H
#define GKENGINE_H

#include "FontRenderer.h"
#include "GekAL.h"
#include <algorithm>
#include <bullet/BulletCollision/CollisionDispatch/btGhostObject.h>
#include <bullet/BulletCollision/Gimpact/btBoxCollision.h>
#include <bullet/BulletCollision/btBulletCollisionCommon.h>
#include <bullet/BulletSoftBody/btDefaultSoftBodySolver.h>
#include <bullet/BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <pthread.h>

// In both cases, MakeDir takes
// a path to create and outputs
// 1 on error.

#define uint unsigned int
#define unchar unsigned char
#define BIT(x) (1 << (x))
#include <glm/gtc/type_ptr.hpp>

namespace GSGE {
using namespace gekRender;
int MakeDir(const char* fn);

enum WorldType : uint { BTDISCRETEWORLD, BTSOFTRIGIDWORLD };

/// Collision groups used by the btDiscreteDynamicsWorld.
/**
 * COL_NORMAL contains the majority of physics objects.
 * COL_HITSCAN is exclusively for things which a hitscan weapon might strike (Including hitboxes)
 * COL_TRIGGER is for any sort of game logic trigger, such as an explosion volume.
 * COL_CHARACTERS is for character bodies, NOT their hitboxes.
 * It is planned that there will be more collision groups for static geometry versus
 * dynamic elements
 * so that AI character controllers
 * can parse the world more easily by using raytests and/or ghostobjects
 *
 * This is on my TODO list.
 * */
enum CollisionGroups : uint {
	COL_NOTHING = 0,
	COL_NORMAL = BIT(1),
	COL_HITSCAN = BIT(2),
	COL_TRIGGER = BIT(3), // E.g. Water, script triggers, explosion radii, etc
	COL_CHARACTERS = BIT(4),
	COL_HITRAY = BIT(5),
	COL_EDITOR = BIT(6),
	COL_EDITOR_RAY = BIT(7),
	COL_EVERYTHING = UINT_MAX
};

/// Turn a glm::mat4 into a btTransform
btTransform g2b_transform(glm::mat4 trans);
/// Turn a btTransform into a glm::mat4
glm::mat4 b2g_transform(btTransform input);

/// Token Command
/**
 * Takes in a token to search for CommandName. Checks for isspace characters
 * Before and After in the token, based on args.
 * True means that the token is in fact invoking that command.
 * */
inline bool tkCmd(const std::string& token, const std::string& CommandName, bool checkBefore = true, bool checkAfter = true) {
	size_t pos = token.find(CommandName);
	size_t end;
	if (pos == std::string::npos) // It flat out isn't there.
		return false;
	end = pos + CommandName.length();

	bool nothingPreceding = (!checkBefore) || (pos == 0 || isspace(token[pos - 1]));
	bool nothingAfter = (!checkAfter) || ((end >= token.length() - 1) || isspace(token[end]));
	return nothingPreceding && nothingAfter;
}
inline float clampf(float min, float max, float test_value) {
	if (test_value < min)
		return min;
	if (test_value > max)
		return max;
	return test_value;
}
/// GLM to Bullet Vec3
inline btVector3 g2b_vec3(glm::vec3 in) { return btVector3(in.x, in.y, in.z); }

/// Bullet to GLM Vec3.
inline glm::vec3 b2g_vec3(btVector3 in) { return glm::vec3(in.getX(), in.getY(), in.getZ()); }

/// GLM to Bullet Quaternion
inline btQuaternion g2b_quat(glm::quat in) { return btQuaternion(in.x, in.y, in.z, in.w); }

/// Bullet to GLM Quaternion
inline glm::quat b2g_quat(btQuaternion in) {
	return glm::quat(in.x(), in.y(), in.z(),
					 in.w()); // documentation of btQuadWord used here...
}

inline void name_compat(std::string& fixme) {
	if (fixme == "") {
		fixme = " ";
		return;
	}
	std::replace(fixme.begin(), fixme.end(), '|', ' ');
	std::replace(fixme.begin(), fixme.end(), '\n', ' ');
}

//~ btConvexHullShape* getFrustumShape(Camera* cam);

// Helpers for making shapes

/// Triangle mesh creator
/**
 * Used for generating static btBvhTriangleMeshShapes.
 * Due to Bullet's own limitations, there is a hard limit of slightly over
 * 2 million triangles.
 * */
btTriangleMesh* makeTriangleMesh(IndexedModel input_model);
IndexedModel retrieveIndexedModelFromTriMesh(btTriangleMesh* trimesh);
/// ConvexHull creator
/**
 * Used for generating convex hull point-clouds from given model data.
 * Note that for standard geometric primitives like cubes and spheres
 * you should use Bullet's geometric primitive types
 * as they are considerably faster than GJK+EPA collision detection.
 * */
btConvexHullShape* makeConvexHullShape(IndexedModel input_model);

/// Used for GkObject
enum AssetMode : uint {
	INDEPENDENT, ///< Has no dependencies on Assets loaded by Resource Manager
	TEMPLATED	///< Dependent on ResourceManager Assets.
};

/// Synchronization mode for GkObjects.
/**
 * One to one syncing synchronizes Meshinstances[i] to RigidBodies[i%RigidBodies.size()]
 * using RelativeTransform[i].
 *
 * Animated synchronizes Meshinstance[i] to RigidBodies[0]
 * using RelativeTransform[i]
 *
 * Softbody syncing is not implemented yet. I don't know how to make my program figure out
 * which vertex in a btSoftBody is the same one as the one in the Mesh's IndexedModel.
 * nor have I thought up an intuitive and powerful way to provide a frontend for softbody
 * creation to content creators.
 * */
enum SyncMode : uint {
	SYNC_ONE_TO_ONE, ///< BtRigidBody 0 syncs to MeshInstance 0 using transform
					 ///< 0... Rigidbody 1 to Meshinstance 1 using transform 1...

	SYNC_ANIMATED, ///< All meshinstances are animated using their transform
				   ///< relative to Rigid Body 0

	SYNC_SOFTBODY, ///< Mesh 0 syncs to the Soft Body. This feature is unimplimented
	SYNC_WELD,	 ///< Child GkObjects which are single body props have their rigid bodies moved.
	NO_SYNC		   ///< No synchronization is performed
};
// FIRST INTENDED USE OF A GkControlInterface:
// There's a pointer to one in GkEngine
// it's the "active control interface"
// You're supposed to put callback hooks into your main.cpp which
// feed into GkEngine's callback hooks.
// The GkControlInterface that GkEngine has will be called.

// SECOND INTENDED USE OF A GkControlInterface:
// If you have another rendering window which needs controls?
// Gotcha covered bud.

/// Represents a "control mode" in a program
/**
 * GkEngine has a member pointer activeControlInterface.
 * All event callbacks sent to the engine will be immediately sent to the ControlInterface.
 * The engine also does some nice stuff like compute cursor deltas (Normalized and not)
 *
 * This implementation allows for easily switching between, say, a menu and controlling a first-person character.
 * Switching between control interfaces is demonstrated in the GSGE main demo main3.cpp
 * simply open it up and search for "activeControlInterface" and you'll see the two switching functions.
 * Current TODOs include: Implementing Joystick Axis/Button callbacks, and a file drag and drop callback
 *
 * I'm also considering window resize and focus callbacks.
 * */
class GkControlInterface {
  public:
	/// The constructor doesn't do anything.
	GkControlInterface();
	virtual ~GkControlInterface() {}
	/// redirect of GLFW callback
	virtual void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	/// redirect of GLFW callback
	virtual void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	/// redirect of GLFW callback with added details.
	virtual void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
										  glm::vec2 NormalizedCursorDelta);
	/// redirect of GLFW callback
	virtual void char_callback(GLFWwindow* window, unsigned int codepoint);
	/// redirect of GLFW callback
	virtual void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	virtual void joystickAxes_callback(int jid, const float* axes, int naxes);
	virtual void joystickButtons_callback(int jid, const unsigned char* buttons, int nbuttons);
	/// If a control interface causes GLFW to switch contexts ever
	/// then this will be true.
	virtual bool doesGLFWContextSwitching();

	/// Callback the user would implement in UI elements such as buttons, subwindows, etc
	void (*ClickCallBack)(GkControlInterface* me, int button, int action, int mods) = nullptr;
	/// Callback the user would implement in UI elements such as buttons, subwindows, etc
	void (*HoverCallBack)(GkControlInterface* me, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
						  glm::vec2 NormalizedCursorDelta) = nullptr;
	/// Callback the user would implement in UI elements such as buttons, subwindows, etc
	void (*KeyCallBack)(GkControlInterface* me, int key, int scancode, int action, int mods) = nullptr;

	/// If this control interface has a parent sub-window, this points to it.
	GkControlInterface* GkUIWindowParent = nullptr;
	std::string CI_Name = "";
	std::vector<void*> userPointers;
	std::vector<std::string> userStrings;
  protected:
	double cursorPos[2];
	double ncursorPos[2];
};



/// Object that represents all the information necessary to draw a text box.
/**
 * This data-only object doesn't know how to draw itself
 * but a window can draw it.
 *
 * The Texture* background member doesn't
 * */
struct TextBoxInfo { // Information on how to draw a button, textbox, etc
  public:
	TextBoxInfo();
	TextBoxInfo(uint _x, uint _y, uint _width, uint _height, std::string _string = "", unchar _red = 0, unchar _green = 0, unchar _blue = 0, unchar _alpha = 0,
				glm::vec3 _string_color = glm::vec3(1, 1, 1), uint _text_offset_x = 0, uint _text_offset_y = 0,
				uint _charw = 16, uint _charh = 16);
	virtual ~TextBoxInfo(){}

	std::string string = "";							   ///< string to be displayed
	unsigned char red = 0, green = 0, blue = 0, alpha = 0; ///< Color of the textbox background.
	bool shouldScroll = true;							   ///< Should the textbox scroll within the UI window?
	glm::vec3 string_color = glm::vec3(255);			   ///< Color of the text we're gonna draw.
	uint width = 0, height = 0, charw = 16, charh = 16;	///< Textbox width, height, and target character width and height.
	uint x = 0, y = 0;									   ///< Coordinates of the top-left corner of this textbox relative to the parent GkUIWindow.
	uint text_offset_x = 0, text_offset_y = 0;			   ///< Offset relative to the top left corner of the textbox to start drawing characters.
};

/// Editable Text field UI element.
/**
 * Meant to provide a text input field in GUIs.
 *
 * Text selection and copy-paste are currently unsupported. Those are TODOs.
 * This class is completely untested.
 * */
class GkEdTextField : public TextBoxInfo, public GkControlInterface {
	// TODO add selection stuff
  public:
	GkEdTextField(uint _x, uint _y, uint _width, uint _height, std::string _string = "", unchar _red = 0, unchar _green = 0, unchar _blue = 0,
				  unchar _alpha = 0, glm::vec3 _string_color = glm::vec3(1, 1, 1), uint _text_offset_x = 0,
				  uint _text_offset_y = 0, uint charw = 16, uint charh = 16)
		: TextBoxInfo(_x, _y, _width, _height, _string, _red, _green, _blue, _alpha, _string_color, _text_offset_x, _text_offset_y, charw, charh) {
	}
	virtual void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) override;
	virtual void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) override;
	virtual void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
										  glm::vec2 NormalizedCursorDelta) override;
	virtual void char_callback(GLFWwindow* window, unsigned int codepoint) override;
	uint screen_width = 1, screen_height = 1;
	float scalingFactor = 1.0;
	bool isEditing = false;
	char red_cursor = 255;
	char green_cursor = 255;
	char blue_cursor = 255;
	// cursor is never transparent
	uint textcursorpos = 0;
	bool needsReDraw = true;
  protected:
};

/// Data container for a single animation parsed from GSGE file
/**
 * Holds all the individual frames of a single animation.
 *
 * GSGE's animation system works around meshes rather than bones
 * for the purposes of reducing runtime cost.
 *
 * Individual Mesh Instances are placed relative to physics objects (see: GkObject Sync Modes)
 *
 * A single frame consists of the transforms of all those mesh instances relative to their parent physics objects
 * An animation is the amalgamation of those frames.
 *
 * In the majority of cases where animation is seriously used, ANIMATED syncing will be used in the GkObject
 * and there will be only one rigid body (E.g. a bumper sphere in a character controller)
 *
 * OBSCURE INTENDED USES OF THE ANIMATION SYSTEM:
 * 1) Animation groups
 * During planning i realized that I wanted to be able to animate different parts of a character differently,
 * E.g. I might want a character to be running with their legs, but be aiming/shooting a gun at the same time.
 * or looking in a particular direction.
 *
 * To specify an animation group, simply create an animation with the name of the animation group
 * that contains valid transforms for all members of the animation group.
 *
 * Make all transforms be a transform from the root of the object's space to the root of the animation group.
 * E.g. an animation group for a "torso" might be rooted at the base of the midsection.
 * so make a transform that goes from 0,0,0 in object space to somewhere inside the torso in object space.
 *
 * Then when you want to perform transformations on the animation group, you can simply ask your GkObject's
 * "Animations" member for the animation corresponding to the group name (E.g. "upper body" or "head")
 *
 * and from that animation, retrieve any valid mat4 from frame zero
 * to find the root transform of the group.
 * */
struct GkAnim {
	GkAnim();								  ///< Constructor. What else is there to say?
	GkAnim(std::string _name);				  ///< Constructor that sets the name of the animation.
	GkAnim(std::string _name, float _timing); ///< Constructor that sets the name AND timing.
	GkAnim(const GkAnim& Other);			  ///< Copy constructor.
	void operator=(const GkAnim& Other);
	void addFrame(std::vector<glm::mat4> input = std::vector<glm::mat4>());
	void setMat4(uint frame, uint item, glm::mat4 matrix); ///< set a specific mat4. Fills in holes with "invalid" mat4s.
	glm::mat4 getMat4(uint frame, uint item);			   ///< get a specific mat4. Returns invalid mat4 for out-of-range frames or ids.
	std::vector<glm::mat4>& operator[](int i);			   ///< Access frames
	const std::vector<glm::mat4>& operator[](int i) const; ///< Access frames
	float getDuration();								   ///< timing * Frames.size() essentially.
	std::vector<std::vector<glm::mat4>> Frames;			   ///< Access convention Frames[Frame][i] where i is the meshinstance id.
	std::string name = "NONAME";						   ///< What is this animation called?
	float timing = 0.016666;							   ///< Length of a frame. Default is 60hz animation.
};

/// Bullet Contact Information struct
/**
 * Represents a single contact from bullet physics.
 *
 * */
struct GkbtContactInformation {
	btCollisionObject* body = nullptr;					 ///< OUR body
	int bodyInternalType;								 ///< typically it's gonna be btCollisionObject::CO_RIGID_BODY
	btCollisionObject* other = nullptr;					 ///< The other guy
	int otherInternalType;								 ///< typically it's gonna be btCollisionObject::CO_RIGID_BODY
	size_t otherID = std::numeric_limits<size_t>::max(); ///< GkObject ID of the other... if it has one.
	float force = 0.0;									 ///< force of the impact. Used for calculating how loud the collision sound should be.
	btVector3 Worldspace_Location = btVector3(0, 0, 0);  ///< Worldspace location on body.
};
static const ALuint NO_SOURCE = UINT_MAX;
static const ALuint NO_BUFFER = UINT_MAX;
#define GK_MAX_CONTACTS 24

/// Complete amalgamation of information necessary to resolve necessary actions from a collision
/**
 * Meant to be used by a GkObject but not necessarily. The user could implement it into their own types.
 *
 * Decoupled from GkObject for code extensibility.
 * */
struct GkCollisionInfo {
	void* myGkObject = nullptr;
	void* userPointer = nullptr;
	GkbtContactInformation Contacts[GK_MAX_CONTACTS];

	GkbtContactInformation OldContacts[GK_MAX_CONTACTS];
	uint numContacts = 0;
	uint numContacts_Old = 0;
	bool noColSoundsThisFrame = false;
	inline void reset() {
		for (size_t i = 0; i < GK_MAX_CONTACTS; i++)
			OldContacts[i] = Contacts[i];
		numContacts_Old = numContacts;
		numContacts = 0;
		noColSoundsThisFrame = false;
	}
	inline bool isContactOld(int index) {
		if (index >= GK_MAX_CONTACTS)
			return true;
		bool isOld = false;
		for (size_t i = 0; i < GK_MAX_CONTACTS && i < numContacts_Old; i++)
			isOld = isOld || (Contacts[index].other == OldContacts[i].other);
		return isOld;
	}
	inline void addContact(GkbtContactInformation bruh) {
		if (numContacts < GK_MAX_CONTACTS) {
			Contacts[numContacts++] = bruh;
		} else {
			noColSoundsThisFrame = true; // TOO MANY CONTACTS!
		}
	}
	std::string userData = ""; // So the user can specify more information if they need to.
	bool disableColSounds = false;
	ALuint colsoundbuffer = NO_BUFFER; // Collision sound of this object.
	float colSoundLoudness = 1.0f;	 // Multiplied by collision force to get
									   // volume with which to play the sound.
	float colSoundMinPitch = 0.8f;
	float colSoundMaxPitch = 1.2f;
	float colSoundMinGain = 0.8f;
	float colSoundMaxGain = 1.2f;
	float colSoundMinImpulse = 1.0f;
	float colSoundMaxDistance = 1000.0f;
	float colSoundRolloffFactor = 1.0f;
};

/// Complete sound playback engine.
/**
 * Provides automatic management and handling of AL sources
 * as well as the listener object.
 *
 * Streams are meant for things like network-transmitted audio
 * like game microphone chats
 * as you would typically receive the data in chunks
 *
 * GkEngine has a sound handler as a member.
 * */
class GkSoundHandler {
  public:
	GkSoundHandler() { isNull = true; }								 ///< pretty simple constructor
	GkSoundHandler(uint num_sound_sources, uint num_stream_sources); ///< invokes construct
	void construct(uint num_sound_sources, uint num_stream_sources); ///< Generates sources. Sets isnull to false.
	~GkSoundHandler();												 ///< deletes the sources. TODO: Implement a destruct() method.

	/// Play a sound.
	/**
	 * If a source is specified it'll play the buffer on the source from the
	 * beginning. if no source, it will pick a source that isn't paused or
	 * playing. If there are no available sources? it will not play the sound
	 *
	 * if the provided source was valid or a valid source was found,
	 * the AL source properties will be updated.
	 *
	 *
	 * Always returns either the given source (default NO_SOURCE) or the source that played the sound.
	 * If the given source was valid, it will of course have played the buffer on it.
	 * */
	ALuint playSound(ALuint soundbuffer, glm::vec3 loc, glm::vec3 vel, ALuint source = NO_SOURCE, float gain = 1.0, float pitch = 1.0, float mingain = 0.5,
					 float maxgain = 1.5, float rolloff_factor = 1.0, float max_distance = 1000, bool looping = false
					 // TODO: add AL_MIN_GAIN, AL_MAX_GAIN, AL_DROPOFF, etc params
	);

	/// Queue and play a stream on a source.
	/**
	 * If a source is specified it'll queue the buffer on the source.
	 * if no source, it will pick a source that isn't paused or
	 * playing. If there are no available sources? it will not queue the stream.
	 * and return NO_SOURCE (or the source provided in the args)
	 *
	 * if the provided source was valid or a valid source was found,
	 * the AL source properties will be updated.
	 *
	 * Returns the given source (default: NO_SOURCE) or the one it found (if the given was invalid)
	 * */
	ALuint QueueAndPlayStream(ALuint soundbuffer, glm::vec3 loc, glm::vec3 vel, ALuint source = NO_SOURCE, float gain = 1.0, float pitch = 1.0,
							  float mingain = 0.5, float maxgain = 1.5, float rolloff_factor = 1.0, float max_distance = 1000, bool looping = false
							  // TODO: add AL_MIN_GAIN, AL_MAX_GAIN, AL_DROPOFF, etc params
	);
	/// Function for pausing all audio.
	void pauseAllSound();
	/// Function for resuming all audio.
	void resumeAllSound();
	/// Function for synchronizing the AL listener to the camera.
	static inline void syncALListener(Camera* cam, glm::vec3 vel = glm::vec3(0, 0, 0)) { syncCameraStateToALListener(cam, vel); }
	bool isNull = true; ///< Convention of GSGE and gekRender to have an isNull property.

  protected:
	std::vector<ALuint> sources_sounds;  ///< Sources for playing sound effects on.
	std::vector<ALuint> sources_streams; ///< Sources for playing streams on.
};


/// Entity Component base class
/**
 * GSGE was not originally going to have an entity component system
 * however I eventually figured out how it worked and it turned out
 * that my entire engine architecture had already been built around
 * the fundamental concept of an entity component.
 *
 * The fundamental concept is thus: You have some behavior you want
 * to implement into an in-game object. That means number crunching.
 * Your bedonkin' brain realizes that it could be done in parellel
 * to other things- You can collect up all the data necessary during
 * a "mid-frame" portion, and then during the majority of the frame,
 * crunch numbers.
 *
 * The MeshInstance-Mesh relationship in gekRender is almost
 * iconically this. Meshinstance collects data from the game
 * during the mid-frame. Mesh processes those Meshinstances and
 * makes GL calls. Suddenly, physics and graphics can be done
 * in-parallel. Bam. Done.
 *
 * And thus, GSGE gained entity components.
 *
 * EntityComponentProcessors take EntityComponents and do their
 * jobs during the parallel portion of the frame. The primary
 * reason for separating the two being, of course, that
 * different kinds of EntityComponents may have different
 * processing requirements.
 *
 * EntityComponents collect data during the mid-frame for use
 * in the parallel step.
 *
 * EntityComponents can also act the opposite way around: they
 * can be the source of data from which other things pull from.
 *
 * An AI-controlled GkObject might make decisions during the
 * parallel step, and then apply them to a character controller
 * in the mid-frame, for instance.
 * It might decide to load/store navigational node information into
 * a central repository, managed by an EC processor, so that other
 * AIs can navigate the game-world more easily.
 * */
class GkEntityComponent {
  public:
	/// Constructor literally just sets the classname.
	GkEntityComponent() : ClassName("GkEntityComponent") {}

	/// Invokes destruct
	virtual ~GkEntityComponent() {
		//~ std::cout << "\nACTUAL EC DESTRUCTOR START" << std::endl;
		destruct();
		//~ std::cout << "\nACTUAL EC DESTRUCTOR END" << std::endl;
	}
	/// Destructor for Entity Components.
	/**
	 * This base destructor clears its dependents and deregisters
	 * from its parents as well as its parent GkObject, if it has one.
	 * It also removes itself from its dependents.
	 * */
	virtual void destruct();

	/// Code that is executed in the mid-frame. Data collection and Decision application.
	virtual void MidFrameProcess() {}

	/// Code that is executed in the parallel step.
	virtual void SeparatedProcess() {}
	/// Serialize this Entity Component. Returns GSGE File lines.
	
	virtual void writeToMaps() {}
	virtual void readFromMaps() {}
	virtual bool LoadGkMap(std::stringstream& file, 
							std::string& line,
							std::vector<std::string>& tokens,
							std::string& savedir,
							std::string& savefile);
	// Returns GkMap lines.
	virtual std::string SerializeGkMap(std::string savepath);
	// Returns GSGE file lines.
	virtual std::string SerializeGSGE(){return "";}
	/// Can we use a given EntityComponentProcessor?
	/**
	 * Some Entity components might require special processors
	 * so we need to evaluate a processor
	 * based on its EntityTypesProcessed member.
	 *
	 * You can implement it however you want, of course.
	 * */
	virtual bool canUseProcessor(void* processor);

	virtual void deregisterFromProcessor();
	void* myProcessor = nullptr;	///< our processor!
	void* GkObjectParent = nullptr; ///< our parent GkObject... if we have one
	/// Get the name of this class.
	std::string getClassName() { return ClassName.c_str(); }
	std::map<std::string, float> floats;
	std::map<std::string, int> ints;
	std::map<std::string, std::string> strings;
  protected:
	std::string ClassName;
};

/// Entity Component Processor
/**
 * Does both mid-frame and parallel processing for GkEntityComponents.
 * */
class GkEntityComponentProcessor {
  public:
	/// Constructor, Note we're setting THREE strings here.
	/**
	 * BackendType specifies what parallelism mechanism is used by this processor.
	 * Here? We have none. So it's just single threaded.
	 * The class name is self-explanatory.
	 * */
	GkEntityComponentProcessor() : BackendType("Single-Threaded"), ClassName("GkEntityComponentProcessor") {}
	void MidFrameProcess() {
		for (auto* item : RegisteredComponents)
			if (item)
				item->MidFrameProcess();
	}
	/// All processing done by this processor in the parallel step.
	/**
	 * The default behavior is just to invoke each component's SeparatedProcess member function
	 * but for more complicated entity components such as an AI system
	 * you might need more than that.
	 * */
	void SeparatedProcess() {
		for (auto* item : RegisteredComponents)
			if (item)
				item->SeparatedProcess();
	}

	/// Initialize the processor.
	/**
	 * This function would set up the processor to be able to start running.
	 * */
	virtual void init(void* Engine) { myEngine = Engine; }
	/// Begin the parallel step.
	virtual void StartSplitProcessing() {
		if (!paused)
			SeparatedProcess();
	}
	/// Halt the calling thread until the parallel step is finished.
	virtual void ConfirmProcessingHalted() {}
	/// Destroy this processor.
	virtual void destruct() {}
	/// Register an entity component to be processed during the parallel step.
	void registerComponent(GkEntityComponent* comp);
	/// Deregister an entity component from this processor.
	void deregisterComponent(GkEntityComponent* comp);

	/// Serialize this Entity Component Processor to a GKMAP file.
	/**
	 * Entity Component processors are serialized before GkObjects.
	 * This way, they will be created before GkObjects are loaded
	 * and Entity Components can register to their processors-of-choise
	 * */
	//~ virtual std::string Serialize(std::string savepath) { return std::string(); }

	std::vector<GkEntityComponent*> RegisteredComponents; ///< All components registered.
	void* myEngine = nullptr;							  ///< Pointer to the GkEngine that we operate under/using

	const char* getClassName() { return ClassName.c_str(); }
	const char* getBackendType() { return BackendType.c_str(); }
	bool paused = false;

  protected:
	std::string ClassName;
	///< Backend for parallel step of this Processor.
	std::string BackendType;
};

/// The main class of this game engine.
/**
 * Meant to represent most typical in-game objects with physics, behaviors, renderable bits, etc
 * */
class GkObject {
  public:
	/// Constructor
	GkObject();
	/// Destructor
	~GkObject();

	size_t getID();
	void setID(size_t _ID);
	bool isSingleBodyProp();
	void markForDeletion() { markedForDeletion = true; }
	/// Perform the synchronization step. Synchronize Graphical meshes with physics objects.
	void Sync();
	/// Destroy this object.
	void destruct();

	/// Play any and all collision sounds.
	void PlayColSounds(GkSoundHandler* soundHandler);

	/// Apply a transformation to an animation group
	/**
	 * Animation groups are sets of meshinstances in a GkObject which should be animated
	 * independently of pre-made animations, and together.
	 *
	 * You might want the head of your character to point toward something, for instance.
	 * */
	void ApplyTransformToAnimGroup(std::string anim_name, glm::mat4 transform);
	void setAnimation(std::string anim_name, float t);
	void setAnimation(GkAnim& anim, float t);
	void clearAnimation();

	void addOwnedEntityComponent(GkEntityComponent* new_owned);
	//~ void addSharedEntityComponent(GkEntityComponent* new_shared);
	void deRegister(GkScene* theScene, btDiscreteDynamicsWorld* world, WorldType myWorldType, bool partial = false);
	void Register(GkScene* theScene, btDiscreteDynamicsWorld* world, WorldType myWorldType);
	inline GkEntityComponent* getEntityComponent(std::string classname){
		for(auto* item : OwnedEntityComponents) if(item && item->getClassName() == classname) return item;
		return nullptr;
	}
	/// Returns all lines needed to be added to GkMap file
	/// to create this object.
	std::string Serialize(std::string savepath);
	// FUNCTIONS WHICH WORK ON AN OBJECT WITH ONLY A SINGLE RIGID BODY
	void makeCharacterController();						///< Sets angular inertia to 0
	void makeKinematic(btDiscreteDynamicsWorld* world); ///< Sets mass to 0 and inertia to 0
	void makeDynamic(btDiscreteDynamicsWorld* world);
	void setRot(glm::vec3 rot); ///< Sets rotation of Rigid Body 0
	void setRot(glm::quat rot); ///< As a quaternion
	void setPos(glm::vec3 pos, bool resetForces = false, bool resetLinVel = false,
				bool resetAngVel = false); ///< Sets the position of rigid body 0
	void setVel(glm::vec3 vel);			   ///< Set the linear velocity of rigid body 0
	void addVel(glm::vec3 vel);			   ///< add velocity to rigid body 0
	glm::vec3 getVel();					   ///< get the velocity of rigid body 0
	glm::vec3 getAngVel();				   ///< get the angular velocity of rigid body 0
	void setAngVel(glm::vec3 vel);		   ///< set the angular velocity of rigid body 0.
	/// Set the world transform of rigid body zero. Args for advanced usage...
	void setWorldTransform(Transform trans, bool apply_to_body = true, bool apply_to_motionstate = true, bool lockScale = true);
	Transform getWorldTransform();
	/// THE SMART WAY TO DO CHARACTER CONTROLLERS!
	/**
	 * Returns the distance from the closest ray hit in X
	 * Force applied by spring in Y
	 * Ground Friction coefficient in Z
	 * And the local velocity of the point the spring collided with
	 * is returned as the second vec3.
	 * */
	glm::mat3x3 applyCharacterControllerSpring(glm::vec3 RelPos,	   // Position in local space relative to the body
											   glm::vec3 RelDirection, // Direction from RelPos in local space.
											   float k_pull,		   // k value to use when the spring is pulling.
											   float k_push,		   // k value to use when the spring is pushing.
											   float natural_length,
											   float max_length, // Max Distance spring can "snap on" to things.
											   btDiscreteDynamicsWorld* world, glm::vec3 dirfilter = glm::vec3(0, 1, 0), // Set to 1,1,1 for fun!
											   float reactionFactor = 0.02);

	AssetMode myAssetMode = INDEPENDENT;
	SyncMode mySyncMode = SYNC_ANIMATED; // the default mode.
	GkObject* myTemplate = nullptr;
	bool haveProperlyDeRegistered = false;
	bool partialDeregistered = false;
	bool isNull = true;
	std::string name = "NoName"; // Templates will have their file name
	std::string GkObjectFile = ""; //GkObject file, virtual or real.
	std::string ExportString = ""; //GSGE string to export to file.
	//~ btSoftBody* Squishy = nullptr; // Currently Unused.

	std::vector<Mesh*> OwnedMeshes;
	std::vector<Mesh*> SharedMeshes;

	std::vector<Texture*> SharedTextures;
	std::vector<Texture*> OwnedTextures;

	std::vector<CubeMap*> SharedCubeMaps;
	std::vector<MeshInstance*> MeshInstances;

	std::vector<btCollisionShape*> OwnedShapes;
	std::vector<btCollisionShape*> SharedShapes;
	std::vector<GkEntityComponent*> OwnedEntityComponents;


	std::vector<btRigidBody*> RigidBodies;
	std::vector<btRigidBody*> Hitboxes;
	
	std::vector<btTypedConstraint*> Constraints; // Constraints
	std::map<std::string, GkAnim>* Animations = nullptr;
	bool shouldDeleteAnimations = false;
	bool markedForDeletion = false; // Used by Entity components.
	bool disableCulling = false; //Forcefully disable culling.
	std::vector<glm::mat4> RelativeTransforms;
	std::string colSoundFileName = "";
	void* userPointer = nullptr;
	GkCollisionInfo ColInfo;
	glm::vec3 creationScale = glm::vec3(0, 0, 0); // Used for exporting to string.

	bool requiresExporting = false; /// Does the GkEngine have to try to export us?
	bool donotsave = false; /// Do we forcefully prevent ourselves from saving to file?
	size_t ID; // Identifier
};

class AdvancedResourceManager {
  public:
	AdvancedResourceManager();
	~AdvancedResourceManager();
	std::string getTextFile(std::string name);
	std::stringstream* getTextFileStream(std::string name);
	IndexedModel getIndexedModelFromFile(std::string name);
	Mesh* getStaticMesh(std::string filename, bool recalcnormals);
	Mesh* getIndependentMesh(std::string filename, bool instanced, bool is_static, bool recalcnormals, glm::mat4 trans = glm::mat4());
	Texture* getTexture(std::string filename, bool transParent, GLenum filter, GLenum wrapmode, float anisotropic);
	CubeMap* getCubeMap(std::string FIRST, std::string SECOND, std::string THIRD, std::string FOURTH, std::string FIFTH, std::string SIXTH);
	GkObject* loadObject(std::string filename, bool isTemplate, GkObject* Template, Transform initTransform);
	GkObject* getAnObject(std::string filename, bool templated, Transform initTransform);
	ALuint loadWAV(std::string filname);
	void UnloadAll();
	// Data Caches
	std::map<std::string, std::string> TextFiles; // Key is filepath, Content is content of that file. Any
												  // textmode file.
	std::map<std::string, IndexedModel*> Models;  // string is filename.obj
	std::map<std::string, Texture*> Textures;
	std::map<std::string, Mesh*> Meshes;
	std::map<std::string, GkObject*> TemplateObjects; // string is filename.gsge
	std::map<std::string, CubeMap*> OurCubeMaps;	  // string is all six filenames one after the other.
	std::map<std::string, ALuint> audiobuffers;
	// Stuff this thing needs to be aware of.
	btDiscreteDynamicsWorld* world = nullptr;
	GkScene* theScene = nullptr;
	WorldType myWorldType = BTDISCRETEWORLD;
	void* myEngine = nullptr;
};

class GkButton : public GkControlInterface {
  public:
	GkButton(TextBoxInfo info);
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) override;
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
								  glm::vec2 NormalizedCursorDelta) override;
	uint screen_width = 1, screen_height = 1;
	float scalingFactor = 1.0;
	TextBoxInfo box;
	bool isPressed = false; // Set by the callback.
	bool needsReDraw = true;

  protected:
};

class GkUIWindow : public GkControlInterface {
  public:
	GkUIWindow(){}
	GkUIWindow(uint _screen_width, uint _screen_height);
	void setScreenSize(uint _screen_width, uint _screen_height, float _scalingFactor);
	~GkUIWindow();
	void destruct();
	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) override;
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) override;
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
								  glm::vec2 NormalizedCursorDelta) override;
	void scroll_callback(GLFWwindow*, double, double);
	void char_callback(GLFWwindow* window, unsigned int codepoint) override;
	void draw(BMPFontRenderer* renderer);
	inline void addButton(GkButton bt) {
		bt.GkUIWindowParent = this;
		Buttons.push_back(bt);
	}
	inline void addField(GkEdTextField et) {
		et.GkUIWindowParent = this;
		Fields.push_back(et);
	}
	inline void makeInActive() {
		for (auto& item : Fields) {
			item.isEditing = false;
			item.textcursorpos = 0;
		}
		isInBackground = true;
	}
	void BuilderPushUp(int pixels);
	TextBoxInfo WindowBody;
	bool isHidden = false;
	bool isInBackground = true;
	std::vector<TextBoxInfo> TextBoxes;
	std::vector<GkButton> Buttons;
	std::vector<GkEdTextField> Fields;
	uint screen_width = 0, screen_height = 0;
	float scalingFactor = 1.0;
	// Used by callbacks on this GkUIWindow.
	bool isMoveable = false;
	bool isMoving = false;
	bool isResizeable = false;
	bool isResizing = false;
	bool needsReDraw = true;
	glm::ivec2 scrollRange = glm::ivec2(0,0);
	int scrollSpeed = 10;
	int scrollPosition = 0;
	int BuilderTopBar = 0; // Incremented upward with the PushUp command.
  protected:
	GkUIWindow(const GkUIWindow& other) {}
	void operator=(const GkUIWindow& other) {}
};

// COMMON UI CALLBACKS
// BUTTONS
static void ToggleWhiteBlackButtonClick(GkControlInterface* me, int button, int action, int mods) {
	GkButton* me_button = (GkButton*)me;
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
		me_button->isPressed = !me_button->isPressed;
	if (me_button->isPressed) {
		me_button->box.string_color = glm::vec3(0);
		me_button->box.red = 255;
		me_button->box.green = 255;
		me_button->box.blue = 255;
	} else {
		me_button->box.string_color = glm::vec3(1);
		me_button->box.red = 0;
		me_button->box.green = 0;
		me_button->box.blue = 0;
	}
}
static void HideWindowButtonClick(GkControlInterface* me, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
		if (me && me->GkUIWindowParent)
			((GkUIWindow*)(me->GkUIWindowParent))->isHidden = true;
}
// WINDOWS
static void MakeWindowActiveWindowClick(GkControlInterface* me, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
		if (me)
			((GkUIWindow*)(me))->isInBackground = false;
}
static void MakeWindowActiveWindowHover(GkControlInterface* me, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
										glm::vec2 NormalizedCursorDelta) {
	if (me)
		((GkUIWindow*)(me))->isInBackground = false;
}
static void MakeWindowHiddenEscapeKey(GkControlInterface* me, int key, int scancode, int action, int mods){
	if (me && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
		((GkUIWindow*)(me))->isInBackground = true;
		((GkUIWindow*)(me))->isHidden = true;
	}
}
// FIELDS
static void fieldClickActive(GkControlInterface* me, int button, int action, int mods) {
	GkEdTextField* b = (GkEdTextField*)me;
	if (b)
		if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
			b->isEditing = true;
			b->textcursorpos = 0;
		}
}
static void fieldClickActiveClear(GkControlInterface* me, int button, int action, int mods) {
	GkEdTextField* b = (GkEdTextField*)me;
	if (b)
		if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
			b->isEditing = true;
			b->string = "";
			b->textcursorpos = 0;
		}
}

/// Q*ake-like character controller design
/**
 * Functions as a character controller for player input
 * It can also be used in NPC mode, in which case things like cursor deltas aren't used
 * and only the lookup, lookdown, lookleft, and lookright states are used.
 * Also, the "incontrol" variable isn't used in NPCtick.
 * */
class GkFPSCharacterController : public GkControlInterface, public GkEntityComponent {
  public:
	GkFPSCharacterController(GkObject* _body, // Also my parent.
							 Camera* _myCamera = nullptr, glm::vec3 _RelPos = glm::vec3(0, 0, 0), std::string configuration = "",
							 btDiscreteDynamicsWorld* world = nullptr, bool _NPCMode = false, bool giveToBody = true);
	~GkFPSCharacterController();

	virtual void setControlConfig(std::string configuration);
	// call GkFPSCharacterController::setControlConfig() in your overriding version
	// to set this base class's config.
	virtual std::string getControlConfig(); // See above

	virtual void writeToMaps() override;
	virtual void readFromMaps() override;
	virtual std::string SerializeGkMap(std::string savepath) override;
	virtual bool LoadGkMap(std::stringstream& file, std::string& line, std::vector<std::string>& tokens, std::string& savedir, std::string& savefile) override;
	// Virtual so you can add more keys/buttons/whatever. Just call
	// GkFPSCharacterController::key_callback(args) before you do that.
	virtual void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) override;
	virtual void mouse_button_callback(GLFWwindow* window, int button, int action,
									   int mods) override; // See above
	virtual void cursor_position_callback(GLFWwindow* window, double xpos, double ypos, double nxpos, double nypos, glm::vec2 CursorDelta,
										  glm::vec2 NormalizedCursorDelta) override; // See above
	virtual void tick(btDiscreteDynamicsWorld* world, GkSoundHandler* soundHandler = nullptr, bool inControl = true);
	virtual void NPCtick(btDiscreteDynamicsWorld* world, GkSoundHandler* soundHandler = nullptr);
	bool isOnGround();
	virtual void lockCursor();
	virtual void unLockCursor();

	// We need access to the world to do raytests and get coefficients of
	// friction and whatnot.

	// KEYBINDS
	// note to self
	// glfwGetKeyName(key, scancode);
	// If the key is -1, the scancode is used. for glfwGetKeyName

	// key, scancode, button, <custom>, <custom>, <custom>
	enum ControlTypes { C_KEY, C_SCANCODE, C_BUTTON, C_CUSTOM1, C_CUSTOM2, C_CUSTOM3 };
	// The custom ones might be joystick inputs for example.
	int Forward[6] = {-2, -2, -2, -2, -2, -2};
	float Forward_State = 0;
	int Backward[6] = {-2, -2, -2, -2, -2, -2};
	float Backward_State = 0;
	int Left[6] = {-2, -2, -2, -2, -2, -2};
	float Left_State = 0;
	int Right[6] = {-2, -2, -2, -2, -2, -2};
	float Right_State = 0;
	int Jump[6] = {-2, -2, -2, -2, -2, -2};
	float Jump_State = 0;
	float Jump_State_Old = 0;
	int Crouch[6] = {-2, -2, -2, -2, -2, -2};
	float Crouch_State = 0;
	float Crouch_State_Old = 0;
	int Run[6] = {-2, -2, -2, -2, -2, -2};
	float Run_State = 0;
	float Run_State_Old = 0;
	// Camlock is toggled each distinct press-release pattern.
	// So it has to save an "old" state.
	int CamLock[6] = {-2, -2, -2, -2, -2, -2};
	float CamLock_State = 0;
	float CamLock_State_Old = 0;
	int LookUp[6] = {-2, -2, -2, -2, -2, -2};
	float LookUp_State = 0;
	int LookDown[6] = {-2, -2, -2, -2, -2, -2};
	float LookDown_State = 0;
	int LookLeft[6] = {-2, -2, -2, -2, -2, -2};
	float LookLeft_State = 0;
	int LookRight[6] = {-2, -2, -2, -2, -2, -2};
	float LookRight_State = 0;

	// Eye properties
	glm::vec3 eyeOffset = glm::vec3(0, 0.3, 0);
	// Movement properties:
	glm::vec2 lookSpeed = glm::vec2(0.01, 0.01);
	glm::vec2 lookKeyMult = glm::vec2(3, 3);
	glm::vec2 CDelta;
	glm::vec2 movementThisFrame = glm::vec2(0, 0); // Forward-back, Left/right
	float GroundMovementForce = 3.2f;			   // Amount of force used to move the player
												   // if they're on the ground.
	float runningMultiplier = 2;				   // Amount to multiply Ground Movement Force by if running.
	float crouchingMultiplier = 0.8;			   //^But for crouching
	float AirMovementForce = 0.11;				   // Amount of force used to move the player in the air.
	float groundSpeedCap = 30;					   // NOTE: only applied if ground friction is applied
	float airSpeedCap = 30;

	float JumpForce = 14;   // Ground jumping force.
	float airJumpForce = 8; // Air-jumping force
	float dampening = 0.5;
	// CHARACTER CONTROLLER SPRING PROPERTIES
	float spring_natural_length;
	float spring_max_length;
	float spring_k_pull; // stiffness of the character controller spring when
						 // it's pulling you toward the ground.
	float spring_k_push; // stiffness of the character controller spring when
						 // it's pushing up up.

	float standing_spring_natural_length = 3;
	float standing_spring_max_length = 3.8;
	float standing_spring_k_pull = 10; // stiffness of the character controller spring when it's pulling
									   // you toward the ground.
	float standing_spring_k_push = 10; // stiffness of the character controller
									   // spring when it's pushing up up.

	float crouching_spring_natural_length = 2;
	float crouching_spring_max_length = 2.8;
	float crouching_spring_k_pull = 10; // stiffness of the character controller spring when it's pulling
										// you toward the ground.
	float crouching_spring_k_push = 10; // stiffness of the character controller
										// spring when it's pushing up up.

	glm::vec3 RelPos = glm::vec3(0, 0, 0); // Position in local space relative to the body,
										   // where the raycast for the spring starts.
	glm::vec3 Up = glm::vec3(0, 1, 0);	 // The direction that is "up" for the character.
	bool onGround = false;
	bool isCameraLocked = false;
	bool isRunning = false;
	bool toggleCrouch = false;
	bool toggleRun = false;
	bool isCrouching = false;
	bool wasCrouching = false;
	bool wasOnGround = false;
	bool wasInControl = false;
	int framesOnGroundBeforeApplyingGroundFriction = 5; ///< How many frames of lee-way do people get to bunnyhop?
	float partialGroundFriction = 0.2;					///< 20 percent of ground friction during those frames...
	int framesBeenOnGround = 22;						///< How many frames have we been on the ground? 0
														///< means... we're not on the ground.
	int framesSinceLastJump = 22;						///< Counts up frames since the last time you jumped.
	int framesBetweenJumps = 22;						///< Frames spent on the ground before a jump can occur again.
	int frameJumpSafetyBuffer = 9;						///< Frames after a jump where character controller spring isn't applied.
	//~ int framesBetweenFootsteps = 20;
	//~ int framesSinceLastFootStep = 0;
	int max_airjumps = 0; // Maximum number of air jumps
	int airjumps = 0;	 // Air-jumps thus far.
	GkObject* body = nullptr;
	Camera* myCamera = nullptr;
	void* userPointer = nullptr;
	ALuint footstepNoise = NO_BUFFER;
	float footstepGain = 1.0f;
	float footStepMinPitch = 0.8f;
	float footStepMaxPitch = 1.2f;
	ALuint jumpNoise = NO_BUFFER; // Noise used for jumping
	float jumpGain = 1.0f;
	float jumpMinPitch = 0.8f;
	float jumpMaxPitch = 1.2f;
	ALuint airJumpNoise = NO_BUFFER; // Obvious at this point right?
	float airJumpGain = 1.0f;
	float airJumpMinPitch = 0.8f;
	float airJumpMaxPitch = 1.2f;
	bool NPCMode = false;

  protected:
	bool ownCamera = false;
};

struct AnimPlaybackState {
	float elapsedTime = 0;
	GkAnim* animation = nullptr;
	float maxElapsed = 0; // If playbackSpeed is negative, this is actually MINelapsed.
	float playbackSpeed = 1.0f;
	bool indefinite = false;
	void tick(float t) { elapsedTime += playbackSpeed * t; }
};

// This object serializes
#define GK_MAX_PLAYING_ANIMATIONS 8
class GkAnimPlayer : public GkEntityComponent {
  public:
	GkAnimPlayer(GkObject* new_owner) {
		ClassName = "GkAnimPlayer";
		GkObjectParent = (void*)new_owner;
		new_owner->addOwnedEntityComponent(this);
	}
	virtual ~GkAnimPlayer(){}
	/// Receive time tick.
	void MidFrameProcess() override;
	/// Start playing an animation.
	void startAnimPlayback(std::string anim_name, 
							float playspeed = 1.0f,
							float maxTime = 10.0f,
							float Offset = 0.0f,
							bool indefinite = false,
							uint priority = UINT_MAX // Automatic placement
	);
	void haltAllAnimations();
	/// Apply animations.
	void SeparatedProcess() override;
	void writeToMaps() override{
		ints.clear();
		strings.clear();
		floats.clear();
		for(size_t i = 0; i < GK_MAX_PLAYING_ANIMATIONS; i++){
			AnimPlaybackState& playbackstate = animPlaybacks[i];
			if(!playbackstate.animation) continue;
			strings["animPlaybacks["+std::to_string(i)+"].anim_name"] = playbackstate.animation->name;
			floats["animPlaybacks["+std::to_string(i)+"].elapsedTime"] = playbackstate.elapsedTime;
			floats["animPlaybacks["+std::to_string(i)+"].maxElapsed"] = playbackstate.maxElapsed;
			floats["animPlaybacks["+std::to_string(i)+"].playbackSpeed"] = playbackstate.playbackSpeed;
			ints["animPlaybacks["+std::to_string(i)+"].indefinite"] = playbackstate.indefinite;
		}
		//Write out the current pose of our parent.
		ints["RelativeTransforms.size()"] = ((GkObject*)GkObjectParent)->RelativeTransforms.size();
		for(size_t mati = 0; mati < ((GkObject*)GkObjectParent)->RelativeTransforms.size(); mati++){
			for(size_t i = 0; i < 16; i++)
				floats["RelativeTransforms[" + std::to_string(mati) + "][" + std::to_string(i/4) + "][" + std::to_string(i%4) + "]"] = ((GkObject*)GkObjectParent)->RelativeTransforms[mati][i/4][i%4];
		}
	}
	void readFromMaps() override{
		for(size_t i = 0; i < GK_MAX_PLAYING_ANIMATIONS; i++){
			AnimPlaybackState& playbackstate = animPlaybacks[i];
			if(!strings.count("animPlaybacks["+std::to_string(i)+"].anim_name")) continue;
			auto anim_name = strings["animPlaybacks["+std::to_string(i)+"].anim_name"];
			playbackstate.animation = &((*(((GkObject*)GkObjectParent)->Animations))[anim_name]);
			playbackstate.elapsedTime = floats["animPlaybacks["+std::to_string(i)+"].elapsedTime"];
			playbackstate.maxElapsed = floats["animPlaybacks["+std::to_string(i)+"].maxElapsed"];
			playbackstate.playbackSpeed = floats["animPlaybacks["+std::to_string(i)+"].playbackSpeed"];
			playbackstate.indefinite = ints["animPlaybacks["+std::to_string(i)+"].indefinite"];
		}
		//~ std::cout << "\nRecorded number of relative transforms: " << ints["RelativeTransforms.size()"] << std::endl;
		while(((GkObject*)GkObjectParent)->RelativeTransforms.size() < ints["RelativeTransforms.size()"])
			((GkObject*)GkObjectParent)->RelativeTransforms.push_back(glm::mat4(std::numeric_limits<float>::max()));
		for(size_t mati = 0; mati < ((GkObject*)GkObjectParent)->RelativeTransforms.size() && mati < ints["RelativeTransforms.size()"]; mati++){
			for(size_t i = 0; i < 16; i++)
				((GkObject*)GkObjectParent)->RelativeTransforms[mati][i/4][i%4] = floats["RelativeTransforms[" + std::to_string(mati) + "][" + std::to_string(i/4) + "][" + std::to_string(i%4) + "]"];
		}
		ints.clear();
		strings.clear();
		floats.clear();
		//~ SeparatedProcess();
	}
	AnimPlaybackState animPlaybacks[GK_MAX_PLAYING_ANIMATIONS];
	float frameTime = 0.01666666;

  protected:
};

/// An entity component for animating a mesh
/**
 * Implement SeparatedProcess() in your derived class to make/assign an IndexedModel to meshToApply.
 * */
class GkMeshAnimator : public GkEntityComponent{
  public: //We could have said struct but NAH
	GkMeshAnimator(GkObject* new_owner, uint index_of_mesh_to_alter = 0, float _ft = 0.0166666f){
		ClassName = "GkMeshAnimator";
		GkObjectParent = (void*)new_owner;
		meshIndex = index_of_mesh_to_alter;
		frametime = _ft;
	}
	virtual ~GkMeshAnimator(){}
	virtual void MidFrameProcess() override{
		if(GkObjectParent && 
			((GkObject*)GkObjectParent)->OwnedMeshes.size() > meshIndex &&
			((GkObject*)GkObjectParent)->OwnedMeshes[meshIndex]
		)
			((GkObject*)GkObjectParent)->OwnedMeshes[meshIndex]->reShapeMesh(meshToApply);
		t += frametime;
	}
	//~ virtual void SeparatedProcess() override;
	uint meshIndex;
	IndexedModel meshToApply;
	float frametime = 0.016666666;
	float t = 0;
};

class GkVoxelChunk{
	public:
	GkVoxelChunk(
		uint _xdim, uint _ydim, uint _zdim, 
		GkScene* _theScene, 
		btDiscreteDynamicsWorld* _world,
		Texture* _Tex,
		Transform _worldTransform,
		float _voxelScale,
		std::vector<glm::vec3>* _colors = nullptr
	);
	void Serialize(std::string chunkfilename);
	void Load(std::string chunkfilename);
	void update();
	~GkVoxelChunk(){destruct();}
	unsigned char& getBlock(uint x, uint y, uint z){return voxelData[x + y * xdim + z * ydim * xdim];}
	void destruct();
	bool needsUpdate = true;
	bool useColors = false;
	unsigned char* voxelData = nullptr;
	uint xdim = 0;
	uint ydim = 0;
	uint zdim = 0;
	Mesh* myMesh = nullptr;
	GkScene* theScene = nullptr;
	Texture* Tex = nullptr;
	Transform worldTransform;
	MeshInstance myMeshInstance;
	GkCollisionInfo colInfo;
	float voxelScale = 1.0f;
	std::vector<glm::vec3>* colors = nullptr;
	btBvhTriangleMeshShape* myColShape = nullptr;
	btRigidBody* myRigidBody = nullptr;
	btDiscreteDynamicsWorld* world = nullptr;
	private:
		GkVoxelChunk(const GkVoxelChunk& other){}
};

class GkVoxelWorld{
	public:
	GkVoxelWorld(
		uint _chunk_xdim, uint _chunk_ydim, uint _chunk_zdim, 
		uint _xnumchunks, uint _ynumchunks, uint _znumchunks,
		GkScene* _theScene, 
		btDiscreteDynamicsWorld* _world,
		Texture* _Tex,
		float _voxelScale,
		const std::vector<glm::vec3>& _colors,
		int _meshmask
	);
	~GkVoxelWorld();
	//Expects 
	void Save(std::string savedir);
	void Load(std::string savedir);
	void setPhongMaterial(Phong_Material phong);
	void destruct();
	void update();
	void setBlock (uint x, uint y, uint z, unsigned char value);
	void FrustumCull(std::vector<glm::vec4>& frust);
	float voxelScale = 1.0f;
	int meshmask;
	std::vector<glm::vec3> colors;
	std::vector<GkVoxelChunk*> chunks;
	uint chunk_xdim;
	uint chunk_ydim;
	uint chunk_zdim;
	uint xnumchunks;
	uint ynumchunks;
	uint znumchunks;
	GkScene* theScene = nullptr;
	Texture* Tex = nullptr;
	btDiscreteDynamicsWorld* world = nullptr;
};


// Useful for debugging multi-threaded code.
void lock_stdout();
void unLock_stdout();
class GkLockStepThread {
  public:
	GkLockStepThread() {
		auto* t = this;
		t->myMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		pthread_barrier_init(&t->myBarrier, NULL, 2);
		t->isThreadLive = 0;
		t->shouldKillThread = 0;
		t->state = 0;
		//t->execute = NULL;
	}
	~GkLockStepThread() {
		auto* t = this;
		pthread_mutex_destroy(&t->myMutex);
		pthread_barrier_destroy(&t->myBarrier);
	}
	void startThread() {
		auto* t = this;
		if(t->isThreadLive)return;
		t->isThreadLive = 1;
		t->shouldKillThread = 0;
		if(pthread_mutex_lock(&t->myMutex))
			puts("\nError locking mutex.");
		t->state = 1; //LOCKED
		pthread_create(
			&t->myThread,
			NULL,
			GkLockStepThread::thread_func,
			(void*)t
		);
	}
	void killThread() {
		auto* t = this;
		if(!t->isThreadLive)return;
	//puts("\nTime for the thread to die...");
		if(t->state != 1){
			t->Lock();
			//puts("\nPast lock!");
		}
		t->shouldKillThread = 1;
		
		t->Step();
		//puts("\nPast step!");
		pthread_join(t->myThread,NULL);
		//if(pthread_kill(t->myThread)){
		//	puts("\nError killing thread.");
		//}
		t->isThreadLive = 0;
		t->shouldKillThread = 0;
	}
	void Lock() { 
		auto* t = this;
		if(t->state == 1)return;//if already locked, nono
		if(!t->isThreadLive)return;
		//puts("\nLocking! <lock>");
		pthread_barrier_wait(&t->myBarrier);
		//puts("\nPast Wait!");
		if(pthread_mutex_lock(&t->myMutex))
			puts("\nError locking mutex.");
		t->state = 1;
	}
	void Step() {
		auto* t = this;
		if(t->state == -1)return; //if already stepping, nono
		if(!t->isThreadLive)return;
		//puts("\nStepping! <step>");
		if(pthread_mutex_unlock(&(t->myMutex)))
			puts("\nError unlocking mutex");
		//puts("\nDone Unlocking!");
		pthread_barrier_wait(&t->myBarrier);
		t->state = -1;
	}
	virtual void Execute() {} // replace with your code in an overriding function.
	static void* thread_func(void* me_void);
	pthread_mutex_t myMutex;
	pthread_barrier_t myBarrier;
	pthread_t myThread;
	int isThreadLive;
	int shouldKillThread;
	int state;

  protected:
};


/*
class GkLockStepThread {
  public:
	GkLockStepThread() {}
	~GkLockStepThread() {
		if (isThreadLive)
			killThread();
	}
	void startThread() {
		if (isThreadLive) // Thread already started. Don't fuck with it!!!!
			return;
		isThreadLive = true;
		shouldKillThread = false;
		pthread_create(&myThread, NULL, thread_func, (void*)this);
	}
	void killThread() {
		Lock();
		shouldKillThread = true;
		Step();
		pthread_join(myThread, NULL);
		isThreadLive = false;
		shouldKillThread = false;
	}
	void Lock() { pthread_mutex_lock(&myMutex); }
	void Step() {
		pthread_cond_signal(&myCond);
		pthread_mutex_unlock(&myMutex);
	}
	virtual void Execute() {} // replace with your code in an overriding function.

	static void* thread_func(void* me_void);
	bool isThreadLive = false;
	bool shouldKillThread = false;
	pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t myCond = PTHREAD_COND_INITIALIZER;
	pthread_t myThread;

  protected:
};
*/
class BulletPhysicsThreadProcessor : public GkLockStepThread {
  public:
	BulletPhysicsThreadProcessor() { world = nullptr; }
	void Construct(btDiscreteDynamicsWorld* _world, float _frametime) {
		frametime = _frametime;
		world = _world;
	}
	void Execute() override {
		if (world && !paused)
			world->stepSimulation(frametime);
	}
	bool paused = false;
	btDiscreteDynamicsWorld* world = nullptr;
	float frametime = 0.016666666;
};

class GkUIRenderingThread : public GkLockStepThread {
  public:
	void Construct(void* _Eng) { myEngine = _Eng; }
	void Execute() override;
	void* myEngine = nullptr;
};

class GkPthreadEntityComponentProcessor : public GkEntityComponentProcessor, public GkLockStepThread {
  public:
	GkPthreadEntityComponentProcessor() {
		ClassName = "GkPthreadEntityComponentProcessor";
		BackendType = "pthread";
	}
	void destruct() override { killThread(); }
	void init(void* Engine) override {
		myEngine = Engine;
		startThread();
	}
	void StartSplitProcessing() override { Step(); }
	void ConfirmProcessingHalted() override { Lock(); }
	void Execute() override {
		if (!paused)
			SeparatedProcess();
	}
	~GkPthreadEntityComponentProcessor() {}
};

class GkEngine {
  public:
	GkEngine();
	GkEngine(uint window_width, uint window_height, float scale, bool resizeable, bool fullscreen, const char* title, bool softbodies, bool _WBOIT,
			 uint samples = 0);
	virtual ~GkEngine();
	// Window event callbacks
	virtual void window_size_callback(GLFWwindow* window, int width,
									  int height); // Callback for the window being resized.
	// Input callbacks
	virtual void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	virtual void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	virtual void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	virtual void char_callback(GLFWwindow* window, unsigned int codepoint);
	virtual void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	virtual void joystickAxes_callback(int jid, const float* axes, int naxes);
	virtual void joystickButtons_callback(int jid, const unsigned char* buttons, int nbuttons);
	// initialize everything related to your game.
	// These functions don't have to be used
	virtual void init_game();
	virtual void tick_game();
	void AutoExecutor(bool needContextSetting = false);
	int getKey(int key);			// https://www.glfw.org/docs/latest/group__keys.html
	int getMouseButton(int button); // https://www.glfw.org/docs/latest/group__buttons.html
	virtual std::string GkObjectExporterHook(GkObject* me, std::string savedir);
	virtual bool GSGELineInterpreter(std::stringstream& file, std::string& line, std::vector<std::string>& tokens, bool isTemplate, GkObject* Template,
									 Transform& initTransform, GkObject*& RetVal, int& error_flag, IndexedModel& lastModel,
									 Mesh*& lastMesh, std::string& lastAnimationName, bool& RetValOwnsLastMesh, Texture*& lastTexture, CubeMap*& lastCubeMap,
									 MeshInstance*& lastMeshInstance, btRigidBody*& lastRigidBody, btCollisionShape*& lastCollisionShape,
									 std::vector<btCollisionShape*>& ShapesInOrder, std::vector<Mesh*>& MeshesInOrder, float& Scaling) {
		return false;
	}
	virtual bool GKMapLineInterpreter(std::string& savedir, std::string& ifilename, std::stringstream& file, std::string& line, std::vector<std::string>& tokens) {
		return false;
	}
	// world->stepSimulation(t) passthrough
	void tickPhysics(double t);
	GkObject* addObject(std::string filename, bool templated, std::string name, Transform init_transform, size_t id = std::numeric_limits<size_t>::max());
	void registerObject(GkObject* obj, std::string name, size_t id = std::numeric_limits<size_t>::max());
	void deregisterObject(GkObject* obj);
	void addConstraint(btTypedConstraint* constraint, bool ncollide = false);
	void removeConstraint(btTypedConstraint* constraint);
	GkObject* getObject(std::string name);
	GkObject* getObject(size_t ID);
	void removeObject(GkObject* obj);
	virtual void setPauseState(bool pauseState){}
	void PurgeUnusedTemplates(); // TODO: implement
	void Rendering_Sync();
	void ECS_Midframes();
	void ECS_Splits();
	void ECS_Halts();
	void registerECSProcessor(GkEntityComponentProcessor* b);
	void deregisterECSProcessor(GkEntityComponentProcessor* b);
	void updateCursorInfo(double _x, double _y);
	glm::vec2 getCursorDelta();
	glm::vec2 getNormalizedCursorDelta();
	glm::vec2 getNormalizedCursorPos();
	GkScene* getScene();
	Camera* getCamera();
	btDiscreteDynamicsWorld* getWorld();
	IODevice* getDevice();
	BMPFontRenderer* getFont();
	GkSoundHandler* getSoundHandler();
	// Helper function for deciding what world type you want.
	btDiscreteDynamicsWorld* makeWorld(
		// And you answer questions about what you want
		glm::vec3 GravityForce, // What's the magnitude of Gravity in this world?
		bool SoftBodyPhysics	// Do we care about soft body physics?
	);
	
	virtual void displayLoadingScreen(double percentage) {}
	void destroy_GkEngine();
	void PlayColSounds();
	void updateContactInfo();
	void resetContactInfo();
	void removeObjectsMarkedForDeletion();
	void guaranteeUniqueIDs();
	void renderShadowMaps(int meshmask = -1);
	void performCulling();
	virtual void setCursorLock(bool state);
	// ParticleRenderers
	void tickParticleRenderers(float t);
	void resetTimeParticleRenderers();
	void DrawAllParticles(Camera* RenderCamera);
	void addParticleRenderer(ParticleRenderer* p);
	void removeParticleRenderer(ParticleRenderer* p);
	void addSubWindow(GkUIWindow* b);
	void removeSubWindow(GkUIWindow* b);
	virtual std::string GkMapSaveHook(std::string& savedir, std::string& filename, std::ofstream& ofile) { return ""; }
	void Save(std::string savedir, std::string filename);
	void Load(std::string filename);
	bool shouldQuit_AutoExecutor = false;
	int argc;
	char** argv;
	double oldmousexy[2] = {0, 0};	 // Old mouse position
	double currentmousexy[2] = {0, 0}; // Why query the current mouse position multiple times?
	AdvancedResourceManager Resources;
	Camera MainCamera;
	bool AutoRebuildCameraProjectionMatrix = true;
	float UI_SCALE_FACTOR = 1.0;
	float RENDERING_SCALE = 1.0;
	float MainCamFOV = 70.0f;
	float MainCamMinClipPlane = 1;
	float MainCamMaxClipPlane = 1000;
	float timePassed = 0;
	bool paused = false;
	GkControlInterface* activeControlInterface = nullptr;
	ALuint audiosources[32];			 // 32 sounds can be playing at once.
	std::vector<GkUIWindow*> subWindows; // UI Windows.
	std::vector<GkEntityComponentProcessor*> ECSProcessors;
	std::vector<gekRender::PointLight*> Point_Lights;
	//~ std::vector<gekRender::DirectionalLight*> Dir_Lights;
	std::vector<gekRender::AmbientLight*> Amb_Lights;
	std::vector<gekRender::CameraLight*> Cam_Lights;
	std::vector<FBO*> Cam_Light_Shadowmap_FBOs;
	std::vector<gekRender::ParticleRenderer*> ParticleRenderers;
	GkVoxelWorld* myVoxelWorld = nullptr;
	BulletPhysicsThreadProcessor bullet_thread;
	GkUIRenderingThread UI_thread;
  protected:
	WorldType myWorldType = BTDISCRETEWORLD;
	bool WBOIT_ENABLED = true; // Weighted Blended Order Independent Transparency.
							   // Should we attempt to configure it?
	void init(uint window_width, uint window_height, float scale, bool resizeable, bool fullscreen, const char* title, bool softbodies = false,
			  uint samples = 0);
	// TODO: Write stuff to DELETE all of these in the destructor
	GkScene* theScene = nullptr;
	Shader* MainShad = nullptr;
	Shader* MainShaderShadows = nullptr;
	Shader* DisplayTexture = nullptr;
	Shader* SkyboxShad = nullptr;
	Shader* WBOITCompShader = nullptr;
	Shader* ParticleShader = nullptr;
	size_t id_ticker = 0;
	// Handles Input

	std::vector<btTypedConstraint*> Constraints;
	gekRender::BMPFontRenderer* myFont = nullptr;

	//~ std::vector<gekRender::ParticleRenderer*> ParticleRenderers;

	IODevice* myDevice = nullptr;
	GkSoundHandler soundHandler;
	btDiscreteDynamicsWorld* world = nullptr;
	btDispatcher* dispatcher = nullptr;
	btCollisionConfiguration* collisionConfig = nullptr;
	btBroadphaseInterface* broadphase = nullptr;
	btConstraintSolver* solver = nullptr;
	btSoftBodySolver* softbodysolver = nullptr;
	std::vector<GkObject*> Objects; // Actual objects REGISTERED in the scene.

}; // Eof class













}; //eof namespace gsge
#endif
