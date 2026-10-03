#pragma once

#include "FEConsoleWindow.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui/imgui_internal.h"

#include "Backends/WindowSystems/FEWindowSystemWindowInterface.h"
#include "Backends/GraphicsAPIs/FEDeviceSurfaceInterface.h"
#include "Backends/FEMonitorInfo.h"

#ifdef FE_GRAPHICS_API_OPENGL
	#include "GL/glew.h"
#endif
#include <GLFW/glfw3.h>

namespace FocalEngine
{
	// Since 1.92.8 ImGui OpenGL3 backend binds its own mipmap-less sampler object, which overrides texture filtering.
	// This function restores previous behavior.
	void RestoreImGuiTextureFilteringForCurrentFrame();

	class FEWindow
	{
		friend class FEBasicApplication;

		FEWindowSystemWindowInterface* WindowSystemWindow = nullptr;
		FEDeviceSurfaceInterface* DeviceSurface = nullptr;

		ImGuiContext* ImguiContext = nullptr;

		FEUUID ID;
		std::string Title = "FEWindow";

		bool bShouldClose = false;
		bool bShouldTerminate = false;
		bool bOverrideImGuiTextureFiltering = true;

		bool bDefaultDockspaceEnabled = false;
		ImGuiID DefaultDockspaceID = 0;

		FEWindow();

		void InitializeImGui();
		void TerminateImGui();
		~FEWindow();

		// User Callbacks
		std::vector<std::pair<FEUUID, std::function<void()>>> UserOnCloseCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(int)>>> UserOnFocusCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void()>>> UserOnTerminateCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(int, int)>>> UserOnResizeCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(int)>>> UserOnMouseEnterCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(int, int, int)>>> UserOnMouseButtonCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(double, double)>>> UserOnMouseMoveCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(unsigned int)>>> UserOnCharCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(int, int, int, int)>>> UserOnKeyCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(int, const char**)>>> UserOnDropCallbackFuncs;
		std::vector<std::pair<FEUUID, std::function<void(double, double)>>> UserOnScrollCallbackFuncs;
		std::vector< std::pair<FEUUID, std::function<void(GLFWmonitor*, int)>>> UserOnMonitorCallbackFuncs;

		// User Render Function
		std::function<void()> UserRenderFunctionImpl;

		ImGuiContext* TemporaryImguiContext = nullptr;
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

		FEUUID GetID() const;

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

		void SetClearColor(float R, float G, float B, float A);

		void EnableDefaultDockspace();
		bool HasDefaultDockspace() const;
		ImGuiID GetDefaultDockspaceID() const;

		void SetOverrideImGuiTextureFiltering(bool bNewValue);
		bool GetOverrideImGuiTextureFiltering() const;

		void CancelClose();
		void Terminate();

		// Event Handling
		FEUUID AddOnCloseCallback(std::function<void()> UserOnCloseCallback);
		FEUUID AddOnFocusCallback(std::function<void(int)> UserOnFocusCallback);
		FEUUID AddOnTerminateCallback(std::function<void()> UserOnTerminateCallback);
		FEUUID AddOnResizeCallback(std::function<void(int, int)> UserOnResizeCallback);
		FEUUID AddOnMouseEnterCallback(std::function<void(int)> UserOnMouseEnterCallback);
		FEUUID AddOnMouseButtonCallback(std::function<void(int, int, int)> UserOnMouseButtonCallback);
		FEUUID AddOnMouseMoveCallback(std::function<void(double, double)> UserOnMouseMoveCallback);
		FEUUID AddOnCharCallback(std::function<void(unsigned int)> UserOnCharCallback);
		FEUUID AddOnKeyCallback(std::function<void(int, int, int, int)> UserOnKeyCallback);
		FEUUID AddOnDropCallback(std::function<void(int, const char**)> UserOnDropCallback);
		FEUUID AddOnScrollCallback(std::function<void(double, double)> UserOnScrollCallback);
		FEUUID AddOnMonitorCallback(std::function<void(GLFWmonitor*, int)> UserOnMonitorCallback);

		// TO-DO: It is problematic to require the user to remove the callback or the application will crash. We should remove it automatically.
		void RemoveCallback(FEUUID CallbackID);

		GLFWwindow* GetGlfwWindow() const;
		ImGuiContext* GetImGuiContext() const;

		MonitorInfo DetermineCurrentMonitor();
	};
}