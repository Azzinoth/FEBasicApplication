#include "FEBasicApplication.h"
using namespace FocalEngine;

FEBasicApplication* FEBasicApplication::Instance = nullptr;

FEBasicApplication::FEBasicApplication()
{
	glfwInit();
	glfwSetMonitorCallback(MonitorCallback);
	IMGUI_CHECKVERSION();
}

FEBasicApplication::~FEBasicApplication()
{
	for (size_t i = 0; i < Windows.size(); i++)
	{
		Windows[i]->InvokeTerminateCallback();
	}

	OnTerminate();
}

void FEBasicApplication::SetWindowCallbacks(FEWindow* Window)
{
	if (Window == nullptr)
		return;

	glfwSetWindowCloseCallback(Window->GetGlfwWindow(), WindowCloseCallback);
	glfwSetWindowFocusCallback(Window->GetGlfwWindow(), WindowFocusCallback);
	glfwSetWindowSizeCallback(Window->GetGlfwWindow(), WindowResizeCallback);
	glfwSetMouseButtonCallback(Window->GetGlfwWindow(), MouseButtonCallback);
	glfwSetCursorEnterCallback(Window->GetGlfwWindow(), MouseEnterCallback);
	glfwSetCursorPosCallback(Window->GetGlfwWindow(), MouseMoveCallback);
	glfwSetCharCallback(Window->GetGlfwWindow(), CharCallback);
	glfwSetKeyCallback(Window->GetGlfwWindow(), KeyButtonCallback);
	glfwSetDropCallback(Window->GetGlfwWindow(), DropCallback);
	glfwSetScrollCallback(Window->GetGlfwWindow(), ScrollCallback);
}

void FEBasicApplication::InitializeWindow(FEWindow* Window)
{
	if (Window == nullptr)
		return;

	glfwMakeContextCurrent(Window->GetGlfwWindow());
	// It should be called for each window.
	glewInit();

	SetWindowCallbacks(Window);
	Window->InitializeImGui();
}

FEWindow* FEBasicApplication::AddWindow(const int Width, const int Height, std::string WindowTitle)
{
	FEWindow* NewWindow = new FEWindow(Width, Height, WindowTitle);
	if (NewWindow->GetGlfwWindow() == nullptr)
		return nullptr;

	Windows.push_back(NewWindow);
	InitializeWindow(NewWindow);

	return NewWindow;
}

FEWindow* FEBasicApplication::AddFullScreenWindow(size_t MonitorIndex)
{
	std::vector<MonitorInfo> Monitors = APPLICATION.GetMonitors();
	if (MonitorIndex >= Monitors.size())
		return nullptr;

	return AddFullScreenWindow(&Monitors[MonitorIndex]);
}

FEWindow* FEBasicApplication::AddFullScreenWindow(MonitorInfo* Monitor)
{
	FEWindow* NewWindow = new FEWindow(Monitor);
	if (NewWindow->GetGlfwWindow() == nullptr)
		return nullptr;

	Windows.push_back(NewWindow);
	InitializeWindow(NewWindow);

	return NewWindow;
}

FEWindow* FEBasicApplication::GetWindow(std::string WindowID)
{
	for (size_t i = 0; i < Windows.size(); i++)
	{
		if (Windows[i]->GetID() == WindowID)
			return Windows[i];
	}

	return nullptr;
}

FEWindow* FEBasicApplication::GetWindow(size_t WindowIndex)
{
	if (WindowIndex < 0 || WindowIndex >= Windows.size())
		return nullptr;

	return Windows[WindowIndex];
}

FEWindow* FEBasicApplication::GetWindow(int WindowIndex)
{
	return GetWindow(size_t(WindowIndex));
}

FEWindow* FEBasicApplication::GetWindow(GLFWwindow* GLFWwindow)
{
	for (size_t i = 0; i < Windows.size(); i++)
	{
		if (Windows[i]->GetGlfwWindow() == GLFWwindow)
			return Windows[i];
	}

	return nullptr;
}

FEWindow* FEBasicApplication::GetMainWindow()
{
	if (Windows.empty())
		return nullptr;

	// For now, I will return the first window
	return Windows[0];
}

void FEBasicApplication::BeginFrame()
{
	if (APPLICATION.HasToTerminate)
		return;

	THREAD_POOL.Update();
}

void FEBasicApplication::EndFrame() const
{
	glfwPollEvents();

	if (APPLICATION.HasToTerminate)
	{
		APPLICATION.OnTerminate();
		return;
	}

	for (size_t i = 0; i < Windows.size(); i++)
	{
		if (Windows[i]->bShouldTerminate)
		{
			APPLICATION.CloseWindow(i);
			return;
		}
	}
}

void FEBasicApplication::RenderWindows()
{
	for (size_t i = 0; i < Windows.size(); i++)
	{
		Windows[i]->BeginFrame();
		Windows[i]->Render();
		Windows[i]->EndFrame();
	}
}

bool FEBasicApplication::IsNotTerminated() const
{
	if (bShouldClose)
		return false;

	if ((Windows.empty() && (ConsoleWindow == nullptr && !bConsoleInitializationStarted)) || ReadToTerminate)
		return false;

	if (Windows.size() == 1 && (ConsoleWindow == nullptr && !bConsoleInitializationStarted) && APPLICATION.Windows[0]->bShouldClose)
		return false;

	return true;
}

void FEBasicApplication::MonitorCallback(GLFWmonitor* Monitor, int Event)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		APPLICATION.Windows[i]->InvokeMonitorCallback(Monitor, Event);
	}
}

void FEBasicApplication::WindowCloseCallback(GLFWwindow* Window)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
		{
			APPLICATION.CloseWindow(i);
			return;
		}
	}
}

void FEBasicApplication::WindowFocusCallback(GLFWwindow* Window, int Focused)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeOnFocusCallback(Focused);
	}
}

void FEBasicApplication::WindowResizeCallback(GLFWwindow* Window, int Width, int Height)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeResizeCallback(Width, Height);
	}
}

void FEBasicApplication::MouseEnterCallback(GLFWwindow* Window, int Entered)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeMouseEnterCallback(Entered);
	}
}

void FEBasicApplication::MouseButtonCallback(GLFWwindow* Window, const int Button, const int Action, const int Mods)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeMouseButtonCallback(Button, Action, Mods);
	}
}

void FEBasicApplication::MouseMoveCallback(GLFWwindow* Window, const double Xpos, const double Ypos)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeMouseMoveCallback(Xpos, Ypos);
	}
}

void FEBasicApplication::CharCallback(GLFWwindow* Window, unsigned int Codepoint)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeCharCallback(Codepoint);
	}
}

void FEBasicApplication::KeyButtonCallback(GLFWwindow* Window, const int Key, const int Scancode, const int Action, const int Mods)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeKeyCallback(Key, Scancode, Action, Mods);
	}
}

void FEBasicApplication::DropCallback(GLFWwindow* Window, const int Count, const char** Paths)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeDropCallback(Count, Paths);
	}
}

void FEBasicApplication::ScrollCallback(GLFWwindow* Window, const double Xoffset, const double Yoffset)
{
	for (size_t i = 0; i < APPLICATION.Windows.size(); i++)
	{
		if (APPLICATION.Windows[i]->GetGlfwWindow() == Window)
			APPLICATION.Windows[i]->InvokeScrollCallback(Xoffset, Yoffset);
	}
}

std::string FEBasicApplication::GetUniqueHexID()
{
	return UNIQUE_ID.GetUniqueHexID();
}

bool FEBasicApplication::SetClipboardText(const std::string Text)
{
	if (OpenClipboard(nullptr))
	{
		EmptyClipboard();

		const HGLOBAL HMem = GlobalAlloc(GMEM_MOVEABLE, Text.size() + 1);
		memcpy(GlobalLock(HMem), Text.c_str(), Text.size() + 1);
		GlobalUnlock(HMem);

		SetClipboardData(CF_TEXT, HMem);

		CloseClipboard();
		return true;
	}

	return false;
}

std::string FEBasicApplication::GetClipboardText()
{
	std::string text;

	if (OpenClipboard(nullptr))
	{
		HANDLE data = nullptr;
		data = GetClipboardData(CF_TEXT);
		if (data != nullptr)
		{
			const char* PszText = static_cast<char*>(GlobalLock(data));
			if (PszText != nullptr)
				text = PszText;
		}

		CloseClipboard();
	}

	return text;
}

bool FEBasicApplication::HasConsoleWindow() const
{
	return !(APPLICATION.ConsoleWindow == nullptr);
}

BOOL WINAPI FEBasicApplication::ConsoleHandler(DWORD dwType)
{
	switch (dwType)
	{
		case CTRL_CLOSE_EVENT:
			// This will be a call from different thread, so I can't call the OnTerminate() directly.
			// Instead, I will use flag to indicate the termination
			APPLICATION.HasToTerminate = true;

			// Here we will try to wait for the main thread to terminate
			while (!APPLICATION.ReadToTerminate)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			};

			return TRUE;
		default:
			return FALSE;
	}
}

void FEBasicApplication::ConsoleMainFunc()
{
	// Allocate a console
	AllocConsole();

	// Get a handle to the console window
	APPLICATION.ConsoleWindow = GetConsoleWindow();
	APPLICATION.bConsoleInitializationStarted = false;

	// Redirect standard I/O to the console
	FILE* pCout;
	freopen_s(&pCout, "CONOUT$", "w", stdout);
	FILE* pCin;
	freopen_s(&pCin, "CONIN$", "r", stdin);

	// Set the console control handler to intercept the close event
	if (!SetConsoleCtrlHandler(FEBasicApplication::ConsoleHandler, TRUE))
	{
		LOG.Add("Failed to set console control handler", "Console");
	}

	APPLICATION.UserConsoleMainFunc(APPLICATION.UserConsoleMainFuncData);

	fclose(pCout);
	fclose(pCin);
	FreeConsole();

	APPLICATION.ConsoleWindow = nullptr;
	APPLICATION.bConsoleActive = false;
}

bool FEBasicApplication::DisableConsoleWindowCloseButton() const
{
	if (APPLICATION.ConsoleWindow == nullptr)
		return false;

	// Get the system menu of the console window
	HMENU SysMenu = GetSystemMenu(APPLICATION.ConsoleWindow, FALSE);

	// Disable the close option
	if (SysMenu != NULL)
	{
		RemoveMenu(SysMenu, SC_CLOSE, MF_BYCOMMAND);
		return true;
	}
	
	return false;
}

void FEBasicApplication::CreateConsoleWindow(std::function<void(void* UserData)> MainFunc, void* UserData)
{
	if (MainFunc == nullptr)
		return;

	bConsoleActive = true;
	bConsoleInitializationStarted = true;
	UserConsoleMainFunc = MainFunc;
	UserConsoleMainFuncData = UserData;

	ConsoleThreadHandler = std::thread(FEBasicApplication::ConsoleMainFunc);
	ConsoleThreadHandler.detach();
}

bool FEBasicApplication::IsConsoleWindowCreated() const
{
	return APPLICATION.ConsoleWindow != nullptr;
}

bool FEBasicApplication::HideConsoleWindow() const
{
	if (APPLICATION.ConsoleWindow == nullptr)
		return false;

	ShowWindow(APPLICATION.ConsoleWindow, SW_HIDE);
	return true;
}

bool FEBasicApplication::ShowConsoleWindow() const
{
	if (APPLICATION.ConsoleWindow == nullptr)
		return false;

	ShowWindow(APPLICATION.ConsoleWindow, SW_SHOW);
	return true;
}

bool FEBasicApplication::IsConsoleWindowHidden() const
{
	if (APPLICATION.ConsoleWindow == nullptr)
		return false;

	WINDOWPLACEMENT Placement;
	Placement.length = sizeof(WINDOWPLACEMENT);

	if (GetWindowPlacement(APPLICATION.ConsoleWindow, &Placement))
	{
		if (Placement.showCmd == SW_HIDE)
			return true;
	}
	return false;
}

bool FEBasicApplication::SetConsoleWindowTitle(const std::string Title) const
{
	if (APPLICATION.ConsoleWindow == nullptr)
		return false;

	SetConsoleTitleA(Title.c_str());
	return true;
}

void FEBasicApplication::AddOnTerminateCallback(std::function<void()> UserOnTerminateCallback)
{
	UserOnTerminateCallbackFunc.push_back(UserOnTerminateCallback);
}

void FEBasicApplication::OnTerminate()
{
	// Call the user callbacks
	for (auto& Func : UserOnTerminateCallbackFunc)
		Func();

	// If the console is active, then terminate it
	if (ConsoleWindow != nullptr)
	{
		// Post a message to the console window to close it
		PostMessage(APPLICATION.ConsoleWindow, WM_CLOSE, 0, 0);
	}

	for (size_t i = 0; i < Windows.size(); i++)
	{
		Windows[i]->InvokeTerminateCallback();
		delete Windows[i];
		if (i + 1 < Windows.size())
			SwitchToImGuiContextOfWindow(i + 1);
	}
	Windows.clear();

	glfwTerminate();

	ReadToTerminate = true;
}

void FEBasicApplication::AddOnCloseCallback(std::function<void()> UserOnCloseCallback)
{
	UserOnCloseCallbackFuncs.push_back(UserOnCloseCallback);
}

void FEBasicApplication::Close()
{
	bShouldClose = true;
	TryToClose();
}

void FEBasicApplication::TryToClose()
{
	// Call the user callbacks
	// In these callbacks, the user can set the flag to false to prevent the application from closing
	for (auto& Func : UserOnCloseCallbackFuncs)
		Func();
}

void FEBasicApplication::CancelClose()
{
	bShouldClose = false;
}

void FEBasicApplication::CloseWindow(std::string WindowID)
{
	for (size_t i = 0; i < Windows.size(); i++)
	{
		if (Windows[i]->GetID() == WindowID)
			CloseWindow(i);
	}
}

void FEBasicApplication::CloseWindow(FEWindow* WindowToClose)
{
	if (WindowToClose == nullptr)
		return;

	for (size_t i = 0; i < Windows.size(); i++)
	{
		if (Windows[i] == WindowToClose)
		{
			CloseWindow(i);
			break;
		}
	}
}

// It could happen that ImGui_ImplGlfw_WndProc would be called in glfwPollEvents() for example;
// And if old window is destroyed, then it will cause crash.
// So, I will set ImGui context to the first window.
// It coudl be not the best solution, but it is the easiest one for now.
void FEBasicApplication::SwitchToImGuiContextOfWindow(size_t WindowIndex)
{
	if (!Windows.empty())
	{
		ImGui::SetCurrentContext(Windows[WindowIndex]->GetImGuiContext());
	}
}

void FEBasicApplication::CloseWindow(size_t WindowIndex)
{
	if (WindowIndex < 0 || WindowIndex >= Windows.size())
		return;

	// Call the user callbacks
	// In these callbacks, the user can cancel the close operation
	Windows[WindowIndex]->InvokeCloseCallback();

	// If the user callback did not cancel the close operation, then close and destroy the window
	if (Windows[WindowIndex]->bShouldClose)
	{
		Windows[WindowIndex]->InvokeTerminateCallback();
		delete Windows[WindowIndex];
		Windows.erase(Windows.begin() + WindowIndex);

		SwitchToImGuiContextOfWindow();
	}

	// If there are no windows and console is hidden or not active, then terminate the application
	if (!HaveAnyVisibleWindow())
	{
		OnTerminate();
	}
}

void FEBasicApplication::TerminateWindow(FEWindow* WindowToTerminate)
{
	if (WindowToTerminate == nullptr)
		return;

	WindowToTerminate->InvokeTerminateCallback();

	size_t WindowIndex = 0;
	for (size_t i = 0; i < Windows.size(); i++)
	{
		if (Windows[i] == WindowToTerminate)
		{
			WindowIndex = i;
			break;
		}
	}

	delete WindowToTerminate;
	Windows.erase(Windows.begin() + WindowIndex);

	SwitchToImGuiContextOfWindow();
	
	// If there are no windows and console is hidden or not active, then terminate the application
	if (!HaveAnyVisibleWindow())
	{
		OnTerminate();
	}
}

bool FEBasicApplication::HaveAnyVisibleWindow() const
{
	if (Windows.empty() && (ConsoleHandler == nullptr || ConsoleHandler != nullptr && IsConsoleWindowHidden()))
	{
		return false;
	}

	return true;
}

bool FEBasicApplication::HaveAnyWindow() const
{
	if (Windows.empty() && ConsoleHandler == nullptr)
	{
		return false;
	}

	return true;
}

std::vector<MonitorInfo> FEBasicApplication::GetMonitors()
{
	std::vector<MonitorInfo> Result;

	int MonitorCount;
	GLFWmonitor** Monitors = glfwGetMonitors(&MonitorCount);

	if (Monitors == nullptr || MonitorCount == 0)
		return Result;

	for (int i = 0; i < MonitorCount; i++)
	{
		if (Monitors[i] != nullptr)
		{
			MonitorInfo Info;

			Info.Monitor = Monitors[i];
			Info.VideoMode = glfwGetVideoMode(Monitors[i]);

			Result.push_back(Info);
		}
	}

	return Result;
}

size_t FEBasicApplication::MonitorInfoToMonitorIndex(MonitorInfo* Monitor)
{
	std::vector<MonitorInfo> Monitors = GetMonitors();
	for (size_t i = 0; i < Monitors.size(); i++)
	{
		if (Monitors[i].Monitor == Monitor->Monitor)
			return i;
	}

	return 0;
}