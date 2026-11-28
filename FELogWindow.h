#pragma once

#include "FELog.h"
#include "FEWindow.h"

namespace FocalEngine
{
	// Colors of the log window. Until they are set, the window takes them from the current ImGui style.
	struct FELogWindowColors
	{
		ImU32 Text = 0;
		ImU32 TextMuted = 0;
		ImU32 TextDisabled = 0;

		ImU32 Button = 0;
		ImU32 ButtonHovered = 0;
		ImU32 Border = 0;

		ImU32 ChannelListBackground = 0;
		ImU32 SelectedChannel = 0;
		ImU32 HoveredChannel = 0;

		ImU32 TableHeader = 0;
		ImU32 TableBorder = 0;
		ImU32 TableAlternateRow = 0;

		ImU32 Info = 0;
		ImU32 Debug = 0;
		ImU32 Warning = 0;
		ImU32 Error = 0;
	};

	struct FELogWindowData;

	// Window with messages of the log. It is optional: only an application that calls Render() in its frame has it.
	class FEBASICAPPLICATION_API FELogWindow
	{
	public:
		SINGLETON_PUBLIC_PART(FELogWindow)

		void Render();

		bool IsVisible() const;
		void SetVisible(bool NewValue);

		// With a close button the user can hide the window, it is shown again with SetVisible.
		bool IsCloseButtonShown() const;
		void SetCloseButtonShown(bool NewValue);

		FELogWindowColors GetColors() const;
		void SetColors(const FELogWindowColors& NewValue);
		void UseColorsOfImGuiStyle();
	private:
		SINGLETON_PRIVATE_PART(FELogWindow)

		bool bVisible = true;
		bool bCloseButtonShown = true;
		bool bOwnColors = false;
		FELogWindowColors Colors;
		FELogWindowData* Data = nullptr;
	};

#ifdef FEBASICAPPLICATION_SHARED
	extern "C" __declspec(dllexport) void* GetLogWindow();
	#define LOG_WINDOW (*static_cast<FELogWindow*>(GetLogWindow()))
#else
	#define LOG_WINDOW FELogWindow::GetInstance()
#endif
}
