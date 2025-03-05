#pragma once

#include "FEWindow.h"

namespace FocalEngine
{
	class FEVirtualUI
	{
		friend class FEBasicApplication;
		ImGuiContext* ImguiContext = nullptr;

		std::string ID;
		std::string Name = "";

		GLuint Framebuffer = -1;
		int Width = 1280;
		int Height = 720;
		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.00f };
	public:
		float LastMouseX = 0.0f, LastMouseY = 0.0f;
		
		FEVirtualUI(int Width = 1280, int Height = 720, std::string Name = "UnNamed");
		void Initialize(GLuint FrameBuffer, int Width, int Height);
		void TerminateImGui();
		~FEVirtualUI();

		// User Callbacks
		std::vector<std::function<void(int, int)>> UserOnResizeCallbackFuncs;
		std::vector<std::function<void(int)>> UserOnMouseEnterCallbackFuncs;
		std::vector<std::function<void(int, int, int)>> UserOnMouseButtonCallbackFuncs;
		std::vector<std::function<void(double, double)>> UserOnMouseMoveCallbackFuncs;
		std::vector<std::function<void(unsigned int)>> UserOnCharCallbackFuncs;
		std::vector<std::function<void(int, int, int, int)>> UserOnKeyCallbackFuncs;
		std::vector<std::function<void(int, const char**)>> UserOnDropCallbackFuncs;
		std::vector<std::function<void(double, double)>> UserOnScrollCallbackFuncs;

		// User Render Function
		std::function<void(FEVirtualUI*)> UserRenderFunctionImpl;

		ImGuiContext* TempImguiContext = nullptr;
		void EnsureCorrectContextBegin();
		void EnsureCorrectContextEnd();

		std::vector<std::function<void()>> FunctionsToToAddFont;
		std::vector<std::function<void()>> CallbacksOnFontReady;
	public:
		// Prevent copying and assignment
		FEVirtualUI(const FEVirtualUI&) = delete;
		FEVirtualUI& operator=(const FEVirtualUI&) = delete;

		std::string GetID() const;

		// Properties Getters and Setters
		std::string GetName() const;
		void SetName(const std::string NewValue);

		void GetSize(int* Width, int* Height) const;
		void SetSize(int NewWidth, int NewHeight);
		int GetWidth() const;
		int GetHeight() const;

		void GetClearColor(float* R, float* G, float* B, float* A) const;
		void SetClearColor(float R, float G, float B, float A);

		std::function<void(FEVirtualUI*)> GetOnRenderFunction();
		void SetOnRenderFunction(std::function<void(FEVirtualUI*)> UserRenderFunction);
		void ClearOnRenderFunction();
		void Render();
		void BeginFrame();
		void EndFrame();

		// Event Handling
		void AddOnResizeCallback(std::function<void(int, int)> UserOnResizeCallback);
		void AddOnMouseEnterCallback(std::function<void(int)> UserOnMouseEnterCallback);
		void AddOnMouseButtonCallback(std::function<void(int, int, int)> UserOnMouseButtonCallback);
		void AddOnMouseMoveCallback(std::function<void(double, double)> UserOnMouseMoveCallback);
		void AddOnCharCallback(std::function<void(unsigned int)> UserOnCharCallback);
		void AddOnKeyCallback(std::function<void(int, int, int, int)> UserOnKeyCallback);
		void AddOnDropCallback(std::function<void(int, const char**)> UserOnDropCallback);
		void AddOnScrollCallback(std::function<void(double, double)> UserOnScrollCallback);

		// Functions that will be invoked by upper layers of abstraction to simulate events
		void InvokeResize(GLuint FrameBuffer, int Width, int Height);
		void InvokeMouseEnterCallback(int Entered);
		void InvokeMouseButton(int Button, int Action, int Mods = 0);
		void InvokeMouseMove(double Xpos, double Ypos);
		void InvokeCharInput(unsigned int Codepoint);
		void InvokeKeyInput(int Key, int Scancode, int Action, int Mods);
		void InvokeDropInput(int Count, const char** Paths);
		void InvokeScrollInput(double Xoffset, double Yoffset);

		ImGuiContext* GetImGuiContext() const;

		// Use that instead of directly adding fonts to ImGui
		// to ensure that font addition is done in the correct way.
		void ExecuteFunctionToAddFont(std::function<void()> Func, std::function<void()> CallbackOnFontReady);
	};
}