#pragma once

#include "string"
#include <random>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include "GL/glew.h"
#include "GL/wglew.h"

#include <GLFW/glfw3.h>
#include <GL/GL.h>

#define SINGLETON_PUBLIC_PART(CLASS_NAME)  \
static CLASS_NAME& getInstance()           \
{										   \
	if (!_instance)                        \
		_instance = new CLASS_NAME();      \
	return *_instance;				       \
}                                          \
										   \
~CLASS_NAME();

#define SINGLETON_PRIVATE_PART(CLASS_NAME) \
static CLASS_NAME* _instance;              \
CLASS_NAME();                              \
CLASS_NAME(const CLASS_NAME &);            \
void operator= (const CLASS_NAME &);

namespace FocalEngine
{
	class FEBasicApplication
	{
		int windowW;
		int windowH;
		std::string windowTitle;
		
		GLFWwindow* window;

		static void windowCloseCallback(GLFWwindow* window);
		static void(*clientWindowCloseCallbackImpl)();

		static void windowResizeCallback(GLFWwindow* window, int width, int height);
		static void(*clientWindowResizeCallbackImpl)(int, int);

		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void(*clientMouseButtonCallbackImpl)(int, int, int);

		static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
		static void(*clientMouseMoveCallbackImpl)(double, double);

		static void keyButtonCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void(*clientKeyButtonCallbackImpl)(int, int, int, int);

		static void dropCallback(GLFWwindow* window, int count, const char** paths);
		static void(*clientDropCallbackImpl)(int, const char**);

		std::string getUniqueId();
	public:
		SINGLETON_PUBLIC_PART(FEBasicApplication)

		GLFWwindow* getGLFWWindow();

		void createWindow(int width = 1920, int height = 1080, std::string WindowTitle = "FEBasicApplication");
		void setWindowCaption(std::string newCaption);
		bool isWindowOpened();

		void terminate();
		void cancelTerination();

		void beginFrame();
		void endFrame();

		bool isWindowInFocus();

		void setWindowCloseCallback(void(*func)());
		void setWindowResizeCallback(void(*func)(int, int));
		void setMouseButtonCallback(void(*func)(int, int, int));
		void setKeyCallback(void(*func)(int, int, int, int));
		void setMouseMoveCallback(void(*func)(double, double));
		void setDropCallback(void(*func)(int, const char**));
		
		void getWindowPosition(int* xpos, int* ypos);
		void getWindowSize(int* width, int* height);

		void minimizeWindow();
		void restoreWindow();

		// This function can produce ID's that are "unique" with very rare collisions.
		// For most purposes it can be considered unique.
		// ID is a 24 long string.
		std::string getUniqueHexID();

		bool setClipboardText(std::string text);
		std::string getClipboardText();
	private:
		SINGLETON_PRIVATE_PART(FEBasicApplication)
	};

#define APPLICATION FEBasicApplication::getInstance()
}