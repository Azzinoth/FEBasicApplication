#pragma once

#include "FEWindow.h"

namespace FocalEngine
{
	class FEBasicApplication
	{
		SINGLETON_PRIVATE_PART(FEBasicApplication)

		std::vector<FEWindow*> Windows;

		void SetWindowCallbacks(FEWindow* Window);
		void InitializeWindow(FEWindow* Window);

		static void WindowCloseCallback(GLFWwindow* Window);
		static void WindowFocusCallback(GLFWwindow* Window, int Focused);
		static void MouseEnterCallback(GLFWwindow* Window, int Entered);
		static void WindowResizeCallback(GLFWwindow* Window, int Width, int Height);
		static void MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods);
		static void MouseMoveCallback(GLFWwindow* Window, double Xpos, double Ypos);
		static void CharCallback(GLFWwindow* Window, unsigned int Codepoint);
		static void KeyButtonCallback(GLFWwindow* Window, int Key, int Scancode, int Action, int Mods);
		static void DropCallback(GLFWwindow* Window, int Count, const char** Paths);
		static void ScrollCallback(GLFWwindow* Window, double Xoffset, double Yoffset);
		static void MonitorCallback(GLFWmonitor* Monitor, int Event);

		HWND ConsoleWindow = nullptr;
		bool bConsoleInitializationStarted = false;
		bool bConsoleActive = false;
		static void ConsoleMainFunc();
		std::function<void(void* UserData)> UserConsoleMainFunc = nullptr;
		void* UserConsoleMainFuncData = nullptr;
		std::thread ConsoleThreadHandler;
		static BOOL WINAPI ConsoleHandler(DWORD dwType);

		std::vector<std::function<void()>> UserOnTerminateCallbackFunc;
		bool HasToTerminate = false;
		bool ReadyToTerminate = false;
		void OnTerminate();

		std::vector<std::function<void()>> UserOnCloseCallbackFuncs;
		bool bShouldClose = false;
		void TryToClose();
		void CancelClose();

		void CloseWindow(size_t WindowIndex);
		void SwitchToImGuiContextOfWindow(size_t WindowIndex = 0);
		void TerminateWindow(FEWindow* WindowToClose);

		bool HaveAnyWindow() const;
		bool HaveAnyVisibleWindow() const;
	public:
		SINGLETON_PUBLIC_PART(FEBasicApplication)

		FEWindow* AddWindow(int Width = 1920, int Height = 1080, std::string WindowTitle = "FEBasicApplication");
		FEWindow* AddFullScreenWindow(size_t MonitorIndex);
		FEWindow* AddFullScreenWindow(MonitorInfo* Monitor);
		FEWindow* GetWindow(GLFWwindow* GLFWwindow);
		FEWindow* GetWindow(std::string WindowID);
		FEWindow* GetWindow(size_t WindowIndex);
		FEWindow* GetWindow(int WindowIndex);

		FEWindow* GetMainWindow();

		void CloseWindow(std::string WindowID);
		void CloseWindow(FEWindow* WindowToClose);

		bool IsNotTerminated() const;
		void Close();

		bool HasConsoleWindow() const;
		void CreateConsoleWindow(std::function<void(void* UserData)> FuncForConsoleThread, void* UserData = nullptr);
		void WaitForConsoleWindowCreation();
		bool SetConsoleWindowTitle(const std::string Title) const;
		bool DisableConsoleWindowCloseButton() const;
		bool HideConsoleWindow() const;
		bool IsConsoleWindowHidden() const;
		
		bool ShowConsoleWindow() const;

		void BeginFrame();
		void EndFrame() const;
		void RenderWindows();

		void AddOnCloseCallback(std::function<void()> UserOnCloseCallback);
		void AddOnTerminateCallback(std::function<void()> UserOnTerminateCallback);

		// This function can produce ID's that are "unique" with very rare collisions.
		// For most purposes it can be considered unique.
		// ID is a 24 long string.
		std::string GetUniqueHexID();

		bool SetClipboardText(std::string Text);
		std::string GetClipboardText();

		std::vector<MonitorInfo> GetMonitors();
		size_t MonitorInfoToMonitorIndex(MonitorInfo* Monitor);
	};

#define APPLICATION FEBasicApplication::getInstance()
}