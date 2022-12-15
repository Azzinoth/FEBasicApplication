#pragma once

#include "FEThreadPool.h"

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

namespace FocalEngine
{
	class FEBasicApplication
	{
		int WindowW;
		int WindowH;
		std::string WindowTitle;
		
		GLFWwindow* Window;

		static void WindowCloseCallback(GLFWwindow* Window);
		static void(*ClientWindowCloseCallbackImpl)();

		static void WindowResizeCallback(GLFWwindow* Window, int Width, int Height);
		static void(*ClientWindowResizeCallbackImpl)(int, int);

		static void MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods);
		static void(*ClientMouseButtonCallbackImpl)(int, int, int);

		static void MouseMoveCallback(GLFWwindow* Window, double Xpos, double Ypos);
		static void(*ClientMouseMoveCallbackImpl)(double, double);

		static void KeyButtonCallback(GLFWwindow* Window, int Key, int Scancode, int Action, int Mods);
		static void(*ClientKeyButtonCallbackImpl)(int, int, int, int);

		static void DropCallback(GLFWwindow* Window, int Count, const char** Paths);
		static void(*ClientDropCallbackImpl)(int, const char**);

		static void ScrollCallback(GLFWwindow* Window, double Xoffset, double Yoffset);
		static void(*ClientScrollCallbackImpl)(double, double);

		std::string GetUniqueId();
	public:
		SINGLETON_PUBLIC_PART(FEBasicApplication)

		GLFWwindow* GetGlfwWindow() const;

		void InitWindow(int Width = 1920, int Height = 1080, std::string WindowTitle = "FEBasicApplication");
		void SetWindowCaption(std::string NewCaption) const;
		bool IsWindowOpened() const;

		void Terminate() const;
		void CancelTerination() const;

		void BeginFrame();
		void EndFrame() const;

		bool IsWindowInFocus() const;

		void SetWindowCloseCallback(void(*Func)());
		void SetWindowResizeCallback(void(*Func)(int, int));
		void SetMouseButtonCallback(void(*Func)(int, int, int));
		void SetKeyCallback(void(*Func)(int, int, int, int));
		void SetMouseMoveCallback(void(*Func)(double, double));
		void SetDropCallback(void(*Func)(int, const char**));
		void SetScrollCallback(void(*Func)(double, double));
		
		void GetWindowPosition(int* Xpos, int* Ypos) const;
		int GetWindowXPosition() const;
		int GetWindowYPosition() const;

		void GetWindowSize(int* Width, int* Height) const;
		int GetWindowWidth() const;
		int GetWindowHeight() const;

		void MinimizeWindow() const;
		void RestoreWindow() const;

		// This function can produce ID's that are "unique" with very rare collisions.
		// For most purposes it can be considered unique.
		// ID is a 24 long string.
		std::string GetUniqueHexID();

		bool SetClipboardText(std::string Text);
		std::string GetClipboardText();
	private:
		SINGLETON_PRIVATE_PART(FEBasicApplication)
	};

#define APPLICATION FEBasicApplication::getInstance()
}