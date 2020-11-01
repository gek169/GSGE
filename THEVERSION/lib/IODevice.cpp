
#include "IODevice.h"

namespace gekRender {

static void error_callback(int error, const char* description) { std::cout << "\n GLFW ERROR: \n" << description; }
void IODevice::initGLFW() {
	glfwInit();
	glfwSetErrorCallback(error_callback);
}
void IODevice::initGL3W() { gl3wInit(); }
double IODevice::getTime() { return glfwGetTime(); }
void IODevice::setTime(double newtime) { glfwSetTime(newtime); }
void IODevice::pollevents() { glfwPollEvents(); }
void IODevice::swapInterval(int interval) { glfwSwapInterval(interval); }

int IODevice::getjoyStickPresent(int joy) { return glfwJoystickPresent(joy); }
const float* IODevice::getJoystickAxes(int joy, int* count) { return glfwGetJoystickAxes(joy, count); }
const unsigned char* IODevice::getJoystickButtons(int joy, int* count) { return glfwGetJoystickButtons(joy, count); }
IODevice::IODevice(){

};
IODevice::~IODevice() {
	if (OpenALDevice)
		alcCloseDevice(OpenALDevice);
	for (int i = 0; i < ourWindows.size(); i++) {
		removeWindow(i);
	}
}
void IODevice::setWindowCloseCallback(int index, GLFWwindowclosefun cbfun) {
	if (ourWindows.size() > index)
		glfwSetWindowCloseCallback(ourWindows[index], cbfun);
}
void IODevice::setScrollCallback(int index, GLFWscrollfun callback) {
	if (ourWindows.size() > index)
		glfwSetScrollCallback(ourWindows[index], callback);
}
void IODevice::setDropCallback(int index, GLFWdropfun cbfun) {
	if (ourWindows.size() > index)
		glfwSetDropCallback(ourWindows[index], cbfun);
}
void IODevice::setCursorPositionCallback(int index, GLFWcursorposfun cbfun) {
	if (ourWindows.size() > index)
		glfwSetCursorPosCallback(ourWindows[index], cbfun);
}
void IODevice::setCharCallback(int index, GLFWcharfun charfun) {
	if (ourWindows.size() > index)
		glfwSetCharCallback(ourWindows[index], charfun);
}
int IODevice::getKey(int index, int key) {
	if (ourWindows.size() > index)
		return glfwGetKey(ourWindows[index], key);
	else
		return -1;
}
//~ void IODevice::enableRawMouseInput(int index){
//~ if (ourWindows.size() > index && glfwRawMouseMotionSupported())
//~ glfwSetInputMode(ourWindows[index], GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

//~ }
//~ void IODevice::disableRawMouseInput(int index){
//~ if (ourWindows.size() > index && glfwRawMouseMotionSupported())
//~ glfwSetInputMode(ourWindows[index], GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

//~ }
int IODevice::getMouseButton(int index, int key) {
	if (ourWindows.size() > index)
		return glfwGetMouseButton(ourWindows[index], key);
	else
		return -1;
}
void IODevice::setMouseButtonCallback(int index, GLFWmousebuttonfun func) {
	if (ourWindows.size() > index)
		glfwSetMouseButtonCallback(ourWindows[index], func);
}
void IODevice::setKeyCallBack(int index, GLFWkeyfun cbfun) {
	if (ourWindows.size() > index)
		glfwSetKeyCallback(ourWindows[index], cbfun);
}
void IODevice::addWindow(int width, int height, const char* title) {
	ourWindows.push_back(glfwCreateWindow(width, height, title, NULL, NULL));
	if (!ourWindows[ourWindows.size() - 1]) {
		glfwTerminate();
		std::cout << "\nFailed to create the window for some reason.";
		exit(EXIT_FAILURE);
	}
}
void IODevice::addFullScreenWindow(int width, int height, const char* title) {
	ourWindows.push_back(glfwCreateWindow(width, height, title, glfwGetPrimaryMonitor(), NULL));
	if (!ourWindows[ourWindows.size() - 1]) {
		glfwTerminate();
		std::cout << "\nFailed to create the window for some reason.";
		exit(EXIT_FAILURE);
	}
}
void IODevice::pushWindowCreationHint(int hint, int value) {
	glfwWindowHint(hint, value); // This is where I left off.
}
void IODevice::hintNextWindowResizeable(bool input) {
	if (input)
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	else
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
}
void IODevice::defaultWindowHints() { glfwDefaultWindowHints(); }
void IODevice::setWindowSizeCallback(int index, GLFWwindowsizefun cb) {
	if (ourWindows.size() > index) {
		glfwSetWindowSizeCallback(ourWindows[index], cb);
	}
}
void IODevice::setWindowFramebufferSizeCallback (int index, GLFWframebuffersizefun cb){
	if (ourWindows.size() > index) {
		glfwSetFramebufferSizeCallback(ourWindows[index], cb);
	}
}
void IODevice::removeWindow(int index) {
	if (ourWindows.size() > index) {
		glfwDestroyWindow(ourWindows[index]);
		ourWindows.erase(ourWindows.begin() + index);
	}
}
void IODevice::setContext(int index) // This is the command you have to use in Multithreaded programs
									 // to bind the GL context to the current thread.
{
	if (ourWindows.size() > index)
		glfwMakeContextCurrent(ourWindows[index]);
	if (OpenALContext)
		alcMakeContextCurrent(OpenALContext);
}
void IODevice::swapBuffers(int index) {
	if (ourWindows.size() > index)
		glfwSwapBuffers(ourWindows[index]);
}
void IODevice::getCursorPosition(int index, double* x, double* y) {
	if (ourWindows.size() > index)
		glfwGetCursorPos(ourWindows[index], x, y);
}
void IODevice::setCursorPosition(int index, double xpos, double ypos) {
	if (ourWindows.size() > index)
		glfwSetCursorPos(ourWindows[index], xpos, ypos);
}
void IODevice::setInputMode(int index, int mode, int value) {
	if (ourWindows.size() > index)
		glfwSetInputMode(ourWindows[index], mode, value);
}
int IODevice::getInputMode(int index, int mode) {
	if (ourWindows.size() > index)
		return glfwGetInputMode(ourWindows[index], mode);
	else
		return 0;
}
const char* IODevice::getClipboardString(int index) {
	if (ourWindows.size() > index)
		return glfwGetClipboardString(ourWindows[index]);
	else
		return nullptr;
}
void IODevice::setClipboardString(int index, const char* string) {
	if (ourWindows.size() > index)
		glfwSetClipboardString(ourWindows[index], string);
}
void IODevice::getWindowSize(int index, int* width, int* height) {
	if (ourWindows.size() > index)
		glfwGetWindowSize(ourWindows[index], width, height);
}
void IODevice::setWindowSize(int index, int width, int height) {
	if (ourWindows.size() > index)
		glfwSetWindowSize(ourWindows[index], width, height);
}
int IODevice::shouldClose(int index) {
	if (ourWindows.size() > index && !glfwWindowShouldClose(ourWindows[index]))
		return glfwWindowShouldClose(ourWindows[index]);
	else if (ourWindows.size() > index) {
		checkWindows(index);
		return 1;
	} else
		return 1;
}
void IODevice::checkWindows(int index) {
	if (ourWindows.size() > index) {
		if (glfwWindowShouldClose(ourWindows[index])) {
			removeWindow(index);
		}
	}
}
int IODevice::getWindowAttrib(int index, int attribute) {
	if (ourWindows.size() > index)
		return glfwGetWindowAttrib(ourWindows[index], attribute);
	else
		return -99999;
}

//~ int IODevice::UpdateGamepadMappings(const char* string){
//~ return glfwUpdateGamepadMappings(string);
//~ }
//~ int IODevice::getGamepadState(int joy, GLFWgamepadstate* state){
//~ return glfwGetGamepadState(joy, state);
//~ }
//~ int IODevice::isJoystickGamepad(int joy){
//~ return glfwJoystickIsGamepad(joy);
//~ }
const char* IODevice::getJoystickName(int joy) { return glfwGetJoystickName(joy); }

//~ const unsigned char* getJoystickHats(int joy, int* count){
//~ return glfwGetJoystickHats(joy, count);
//~ }
GLFWwindow* IODevice::getWindow(int input) {
	if (ourWindows.size() > input)
		return ourWindows[input];
	else
		return nullptr;
}
void IODevice::setWindowTitle(int window, const char* title) {
	if (ourWindows.size() > window)
		glfwSetWindowTitle(ourWindows[window], title);
}
void IODevice::setWindowIcon(int window, int count, const GLFWimage* images) {
	if (ourWindows.size() > window)
		glfwSetWindowIcon(ourWindows[window], count, images);
}
void IODevice::fastInitOpenAL() {
	if (OpenALDevice) {
		alcCloseDevice(OpenALDevice);
		OpenALDevice = 0;
	}
	OpenALDevice = alcOpenDevice(NULL);
	if (OpenALDevice) {
		// Enable these printouts for debugging
		// std::cout << "\nUsing Device: " << alcgetString(OpenALDevice,
		// ALC_DEVICE_SPECIFIER) << "\n";
		OpenALContext = alcCreateContext(OpenALDevice, 0);
		if (alcMakeContextCurrent(OpenALContext)) {
			// std::cout<<"\nSuccessfully Made Context!!!";
		}
	}
	alGetError();
}

}; // namespace gekRender
