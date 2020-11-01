#ifndef IODEVICE_H
#define IODEVICE_H
#include <iostream>
#include <vector>

#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <GLFW/glfw3.h>
#include <string>

namespace gekRender {
class IODevice // This class exists so that my engine will support fancy window
			   // shit, like drawing all the different stages of rendering to
			   // different windows. Will help a lot with shader development.
{
  public:
	IODevice();
	~IODevice();
	void setWindowCloseCallback(int index, GLFWwindowclosefun cbfun);
	void setDropCallback(int index, GLFWdropfun cbfun);
	void setCursorPositionCallback(int index, GLFWcursorposfun cbfun);
	void setCharCallback(int index, GLFWcharfun charfun);
	void setScrollCallback(int index, GLFWscrollfun callback);
	int getKey(int index, int key);
	//~ void enableRawMouseInput(int index);
	//~ void disableRawMouseInput(int index);
	int getMouseButton(int index, int key);
	void setMouseButtonCallback(int index, GLFWmousebuttonfun func);
	void setKeyCallBack(int index, GLFWkeyfun cbfun);
	void addWindow(int width, int height, const char* title);
	void addFullScreenWindow(int width, int height, const char* title);
	void pushWindowCreationHint(int hint, int value);
	void hintNextWindowResizeable(bool input);
	void defaultWindowHints();
	void setWindowSizeCallback(int index, GLFWwindowsizefun cb);
	void setWindowFramebufferSizeCallback (int index, GLFWframebuffersizefun cb);
	void removeWindow(int index);
	void setContext(int index); // This is the command you have to use in Multithreaded
								// programs to bind the GL context to the current thread.

	void swapBuffers(int index);
	void getCursorPosition(int index, double* x, double* y);
	void setCursorPosition(int index, double xpos, double ypos);
	void setInputMode(int index, int mode, int value);
	int getInputMode(int index, int mode);
	const char* getClipboardString(int index);
	void setClipboardString(int index, const char* string);
	void getWindowSize(int index, int* width, int* height);
	void setWindowSize(int index, int width, int height);
	
	int shouldClose(int index);
	void checkWindows(int index);
	int getWindowAttrib(int index, int attribute);
	static void pollevents();
	static void swapInterval(int interval);
	void initGLFW();
	static void initGL3W();
	static double getTime();
	static void setTime(double newtime);
	//~ int UpdateGamepadMappings(const char* string);
	//~ int getGamepadState(int joy, GLFWgamepadstate* state);
	//~ int isJoystickGamepad(int joy);
	const char* getJoystickName(int joy);

	static int getjoyStickPresent(int joy);
	static const float* getJoystickAxes(int joy, int* count);
	static const unsigned char* getJoystickButtons(int joy, int* count);
	//~ const unsigned char* getJoystickHats(int joy, int* count);
	GLFWwindow* getWindow(int input);
	void setWindowTitle(int window, const char* title);
	void setWindowIcon(int window, int count, const GLFWimage* images);
	ALCdevice* OpenALDevice = 0;
	ALCcontext* OpenALContext = 0;
	void fastInitOpenAL();

  protected:
  private:
	IODevice(const IODevice& other);
	IODevice& operator=(const IODevice& other);
	std::vector<GLFWwindow*> ourWindows;
};
};	 // namespace gekRender
#endif // IODevice_H
