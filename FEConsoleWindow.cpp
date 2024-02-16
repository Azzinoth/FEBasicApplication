#include "FEConsoleWindow.h"
using namespace FocalEngine;

FEConsoleWindow::FEConsoleWindow(std::function<void(void* UserData)> MainFunc, void* UserData)
{
	bConsoleActive = true;
	UserConsoleMainFunc = MainFunc;
	UserConsoleMainFuncData = UserData;
	bConsoleInitializationStarted = true;

	ConsoleThreadHandler = std::thread(&FEConsoleWindow::ConsoleMainFunc, this);
	ConsoleThreadHandler.detach();
}

HWND FEConsoleWindow::GetHandle() const
{
	return ConsoleWindow;
}

void FEConsoleWindow::ConsoleMainFunc()
{
	// Allocate a console
	AllocConsole();

	// Get a handle to the console window
	ConsoleWindow = GetConsoleWindow();
	bConsoleInitializationStarted = false;

	// Redirect standard I/O to the console
	FILE* pCout;
	freopen_s(&pCout, "CONOUT$", "w", stdout);
	FILE* pCin;
	freopen_s(&pCin, "CONIN$", "r", stdin);

	UserConsoleMainFunc(UserConsoleMainFuncData);

	fclose(pCout);
	fclose(pCin);
	FreeConsole();

	bConsoleActive = false;
}

bool FEConsoleWindow::DisableCloseButton() const
{
	if (ConsoleWindow == nullptr)
		return false;

	// Get the system menu of the console window
	HMENU SysMenu = GetSystemMenu(ConsoleWindow, FALSE);

	// Disable the close option
	if (SysMenu != NULL)
	{
		RemoveMenu(SysMenu, SC_CLOSE, MF_BYCOMMAND);
		return true;
	}

	return false;
}

void FEConsoleWindow::WaitForCreation()
{
	if (!bConsoleInitializationStarted)
		return;

	while (ConsoleWindow == nullptr)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

bool FEConsoleWindow::Hide() const
{
	if (ConsoleWindow == nullptr)
		return false;

	ShowWindow(ConsoleWindow, SW_HIDE);
	return true;
}

bool FEConsoleWindow::Show() const
{
	if (ConsoleWindow == nullptr)
		return false;

	ShowWindow(ConsoleWindow, SW_SHOW);
	return true;
}

bool FEConsoleWindow::IsHidden() const
{
	if (ConsoleWindow == nullptr)
		return false;

	WINDOWPLACEMENT Placement;
	Placement.length = sizeof(WINDOWPLACEMENT);

	if (GetWindowPlacement(ConsoleWindow, &Placement))
	{
		if (Placement.showCmd == SW_HIDE)
			return true;
	}
	return false;
}

bool FEConsoleWindow::SetTitle(const std::string Title) const
{
	if (ConsoleWindow == nullptr)
		return false;

	SetConsoleTitleA(Title.c_str());
	return true;
}