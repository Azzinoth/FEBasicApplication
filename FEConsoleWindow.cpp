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

WORD FEConsoleWindow::RGBToConsoleColor(int R, int G, int B) const
{
	// Define the basic console colors with their RGB values
	static const std::tuple<int, int, int, WORD> consoleColors[] = {
		{0, 0, 0, 0}, // Black
		{0, 0, 128, FOREGROUND_BLUE}, // Dark Blue
		{0, 128, 0, FOREGROUND_GREEN}, // Dark Green
		{0, 128, 128, FOREGROUND_GREEN | FOREGROUND_BLUE}, // Dark Cyan
		{128, 0, 0, FOREGROUND_RED}, // Dark Red
		{128, 0, 128, FOREGROUND_RED | FOREGROUND_BLUE}, // Dark Magenta
		{128, 128, 0, FOREGROUND_RED | FOREGROUND_GREEN}, // Dark Yellow
		{192, 192, 192, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE}, // Light Gray
		{128, 128, 128, FOREGROUND_INTENSITY}, // Dark Gray (Bright Black)
		{0, 0, 255, FOREGROUND_BLUE | FOREGROUND_INTENSITY}, // Bright Blue
		{0, 255, 0, FOREGROUND_GREEN | FOREGROUND_INTENSITY}, // Bright Green
		{0, 255, 255, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY}, // Bright Cyan
		{255, 0, 0, FOREGROUND_RED | FOREGROUND_INTENSITY}, // Bright Red
		{255, 0, 255, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY}, // Bright Magenta
		{255, 255, 0, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY}, // Bright Yellow
		{255, 255, 255, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY} // White
	};

	WORD nearestColor = 0; // Default to black if no match is found
	double minDistance = DBL_MAX;

	for (const auto& color : consoleColors)
	{
		// Calculate the Euclidean distance between the two colors
		double distance = std::sqrt(std::pow(std::get<0>(color) - R, 2) +
			std::pow(std::get<1>(color) - G, 2) +
			std::pow(std::get<2>(color) - B, 2));

		if (distance < minDistance)
		{
			minDistance = distance;
			nearestColor = std::get<3>(color);
		}
	}

	return nearestColor;
}

std::vector<char> FEConsoleWindow::GetConsoleTextColor() const
{
	// Get the current console screen buffer info, which contains the color attributes
	CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ConsoleInfo);

	// Extract the foreground color
	WORD ColorWord = ConsoleInfo.wAttributes & 0x0F;

	// Map the console foreground color to a rough RGB representation
	// Note: This mapping is quite arbitrary and simplistic
	std::vector<char> ColorRGB(3, 0); // Initialize RGB vector with 0s

	// Mapping the color attributes to RGB values
	if (ColorWord & FOREGROUND_RED)
		ColorRGB[0] = static_cast<char>(255); // R

	if (ColorWord & FOREGROUND_GREEN)
		ColorRGB[1] = static_cast<char>(255); // G

	if (ColorWord & FOREGROUND_BLUE)
		ColorRGB[2] = static_cast<char>(255); // B

	// Adjust brightness based on FOREGROUND_INTENSITY
	if (!(ColorWord & FOREGROUND_INTENSITY))
	{
		for (auto& Component : ColorRGB)
		{
			Component = Component > 0 ? 128 : 0; // Dim the color if intensity is not set
		}
	}

	return ColorRGB;
}

void FEConsoleWindow::SetNearestConsoleTextColor(int R, int G, int B) const
{
	WORD Color = RGBToConsoleColor(R, G, B);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Color);
}