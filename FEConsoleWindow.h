#pragma once

#include "SubSystems/Networking/FENetworking.h"
#include "SubSystems/Profiling/FEScopedTimer.h"

namespace FocalEngine
{
	class FEConsoleWindow
	{
		friend class FEBasicApplication;

		FEConsoleWindow(std::function<void(void* UserData)> MainFunc, void* UserData);

		HWND ConsoleWindow = nullptr;
		bool bConsoleInitializationStarted = false;
		bool bConsoleActive = false;
		void ConsoleMainFunc();
		std::function<void(void* UserData)> UserConsoleMainFunc = nullptr;
		void* UserConsoleMainFuncData = nullptr;
		std::thread ConsoleThreadHandler;

		WORD RGBToConsoleColor(int R, int G, int B) const;
	public:
		// Prevent copying and assignment
		FEConsoleWindow(const FEConsoleWindow&) = delete;
		FEConsoleWindow& operator=(const FEConsoleWindow&) = delete;

		void WaitForCreation();
		bool SetTitle(const std::string Title) const;
		bool DisableCloseButton() const;
		bool Show() const;
		bool Hide() const;
		bool IsHidden() const;

		HWND GetHandle() const;

		std::vector<char> GetConsoleTextColor() const;
		void SetNearestConsoleTextColor(int R, int G, int B) const;
	};
}