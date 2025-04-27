#pragma once

#include "FEConsoleWindow.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"

#include "GL/glew.h"
#include "GL/wglew.h"

#include <GLFW/glfw3.h>
#include <GL/GL.h>

namespace FocalEngine
{
	struct MonitorInfo
	{
		GLFWmonitor* Monitor = nullptr;
		std::string Name;
		int VirtualX = 0;
		int VirtualY = 0;
		const GLFWvidmode* VideoMode;
	};

	class FEWindow
	{
		friend class FEBasicApplication;
		GLFWwindow* GLFWWindow = nullptr;
		ImGuiContext* ImguiContext = nullptr;

		std::string ID;
		std::string Title = "FEWindow";

		bool bShouldClose = false;
		bool bShouldTerminate = false;
		
		FEWindow(int Width = 1280, int Height = 720, std::string WindowTitle = "FEWindow");
		FEWindow(MonitorInfo* Monitor);

		void InitializeImGui();
		void TerminateImGui();
		~FEWindow();

		// User Callbacks
		std::vector<std::pair<std::string, std::function<void()>>> UserOnCloseCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(int)>>> UserOnFocusCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void()>>> UserOnTerminateCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(int, int)>>> UserOnResizeCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(int)>>> UserOnMouseEnterCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(int, int, int)>>> UserOnMouseButtonCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(double, double)>>> UserOnMouseMoveCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(unsigned int)>>> UserOnCharCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(int, int, int, int)>>> UserOnKeyCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(int, const char**)>>> UserOnDropCallbackFuncs;
		std::vector<std::pair<std::string, std::function<void(double, double)>>> UserOnScrollCallbackFuncs;
		std::vector< std::pair<std::string, std::function<void(GLFWmonitor*, int)>>> UserOnMonitorCallbackFuncs;

		// User Render Function
		std::function<void()> UserRenderFunctionImpl;

		ImGuiContext* TempImguiContext = nullptr;
		GLFWwindow* TempGLFWContext = nullptr;
		void EnsureCorrectContextBegin();
		void EnsureCorrectContextEnd();

		// Callback functions that will be invoked from APPLICATION
		void InvokeCloseCallback();
		void InvokeOnFocusCallback(int Focused);
		void InvokeTerminateCallback();
		void InvokeResizeCallback(int Width, int Height);
		void InvokeMouseEnterCallback(int Entered);
		void InvokeMouseButtonCallback(int Button, int Action, int Mods);
		void InvokeMouseMoveCallback(double Xpos, double Ypos);
		void InvokeCharCallback(unsigned int Codepoint);
		void InvokeKeyCallback(int Key, int Scancode, int Action, int Mods);
		void InvokeDropCallback(int Count, const char** Paths);
		void InvokeScrollCallback(double Xoffset, double Yoffset);
		void InvokeMonitorCallback(GLFWmonitor* Monitor, int Event);
	public:
		// Prevent copying and assignment
		FEWindow(const FEWindow&) = delete;
		FEWindow& operator=(const FEWindow&) = delete;

		std::string GetID() const;

		// Properties Getters and Setters
		std::string GetTitle() const;
		void SetTitle(const std::string NewValue);
		void GetPosition(int* Xpos, int* Ypos) const;
		int GetXPosition() const;
		int GetYPosition() const;

		void GetSize(int* Width, int* Height) const;
		void SetSize(int NewWidth, int NewHeight);
		int GetWidth() const;
		int GetHeight() const;
		
		std::function<void()> GetRenderFunction();
		void SetRenderFunction(std::function<void()> UserRenderFunction);
		void ClearRenderFunction();
		void Render();
		void BeginFrame();
		void EndFrame();

		// Window state
		bool IsInFocus() const;
		void Minimize() const;
		void Restore() const;

		void CancelClose();
		void Terminate();

		// Event Handling
		std::string AddOnCloseCallback(std::function<void()> UserOnCloseCallback);
		std::string AddOnFocusCallback(std::function<void(int)> UserOnFocusCallback);
		std::string AddOnTerminateCallback(std::function<void()> UserOnTerminateCallback);
		std::string AddOnResizeCallback(std::function<void(int, int)> UserOnResizeCallback);
		std::string AddOnMouseEnterCallback(std::function<void(int)> UserOnMouseEnterCallback);
		std::string AddOnMouseButtonCallback(std::function<void(int, int, int)> UserOnMouseButtonCallback);
		std::string AddOnMouseMoveCallback(std::function<void(double, double)> UserOnMouseMoveCallback);
		std::string AddOnCharCallback(std::function<void(unsigned int)> UserOnCharCallback);
		std::string AddOnKeyCallback(std::function<void(int, int, int, int)> UserOnKeyCallback);
		std::string AddOnDropCallback(std::function<void(int, const char**)> UserOnDropCallback);
		std::string AddOnScrollCallback(std::function<void(double, double)> UserOnScrollCallback);
		std::string AddOnMonitorCallback(std::function<void(GLFWmonitor*, int)> UserOnMonitorCallback);

		// TO-DO: It is problematic to require the user to remove the callback or the application will crash. We should remove it automatically.
		void RemoveCallback(std::string CallbackID);

		GLFWwindow* GetGlfwWindow() const;
		ImGuiContext* GetImGuiContext() const;

		MonitorInfo DetermineCurrentMonitor();
	};
}