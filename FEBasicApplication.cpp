#include "FEBasicApplication.h"
using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetBasicApplication()
{
	return FEBasicApplication::GetInstancePointer();
}
#endif

FEBasicApplication::FEBasicApplication()
{
	glfwInit();
	glfwSetMonitorCallback(MonitorCallback);
	IMGUI_CHECKVERSION();

	// Set the console control handler to intercept the close event
	if (!SetConsoleCtrlHandler(APPLICATION.ConsoleHandler, TRUE))
	{
		LOG.Add("Failed to set console control handler", "Console");
	}
}

FEBasicApplication::~FEBasicApplication()
{
	for (size_t i = 0; i < Windows.size(); i++)
	{
		Windows[i]->InvokeTerminateCallback();
	}

	OnTerminate();
}

std::string FEBasicApplication::GetVersion()
{
	return std::to_string(FEBASICAPPLICATION_VERSION_MAJOR) + "."
		   + std::to_string(FEBASICAPPLICATION_VERSION_MINOR) + "."
		   + std::to_string(FEBASICAPPLICATION_VERSION_PATCH);
}

int FEBasicApplication::GetBuildNumber()
{
	return FEBASICAPPLICATION_BUILD_NUMBER;
}

std::string FEBasicApplication::GetBuildTimestamp()
{
	return FEBASICAPPLICATION_BUILD_TIMESTAMP;
}

std::string FEBasicApplication::GetBuildInfo()
{
	std::string Result = "build " + std::to_string(FEBASICAPPLICATION_BUILD_NUMBER) + " (" + std::string(FEBASICAPPLICATION_GIT_HASH);

	if (FEBASICAPPLICATION_BUILD_BRANCH_OFFSET > 0)
		Result += " " + std::string(FEBASICAPPLICATION_GIT_BRANCH) + " +" + std::to_string(FEBASICAPPLICATION_BUILD_BRANCH_OFFSET) + " from master";

	if (FEBASICAPPLICATION_GIT_DIRTY)
		Result += ", dirty";

	Result += ")";
	return Result;
}

std::string FEBasicApplication::GetFullVersion()
{
	return "FEBasicApplication " + GetVersion() + " " + GetBuildInfo();
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
	if (APPLICATION.bHasToTerminate)
		return;

	THREAD_POOL.Update();

	for (size_t i = 0; i < VirtualUIs.size(); i++)
	{
		VirtualUIs[i]->BeginFrame();
		VirtualUIs[i]->Render();
		VirtualUIs[i]->EndFrame();
	}
}

void FEBasicApplication::EndFrame() const
{
	glfwPollEvents();

	if (APPLICATION.bHasToTerminate)
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

	if ((Windows.empty() && (ConsoleWindow == nullptr)) || bIsReadyToTerminate)
		return false;

	if (Windows.size() == 1 && (ConsoleWindow == nullptr) && APPLICATION.Windows[0]->bShouldClose)
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

		const HGLOBAL MemoryHandle = GlobalAlloc(GMEM_MOVEABLE, Text.size() + 1);
		if (MemoryHandle == nullptr)
		{
			LOG.Add("Failed to allocate memory for clipboard", "FE_BASIC_APPLICATION", FE_LOG_ERROR);
			CloseClipboard();
			return false;
		}

		memcpy(GlobalLock(MemoryHandle), Text.c_str(), Text.size() + 1);
		GlobalUnlock(MemoryHandle);

		SetClipboardData(CF_TEXT, MemoryHandle);

		CloseClipboard();
		return true;
	}

	return false;
}

std::string FEBasicApplication::GetClipboardText()
{
	std::string Result;

	if (OpenClipboard(nullptr))
	{
		HANDLE ClipboardData = nullptr;
		ClipboardData = GetClipboardData(CF_TEXT);
		if (ClipboardData != nullptr)
		{
			const char* ClipboardText = static_cast<char*>(GlobalLock(ClipboardData));
			if (ClipboardText != nullptr)
				Result = ClipboardText;
		}

		CloseClipboard();
	}

	return Result;
}

BOOL WINAPI FEBasicApplication::ConsoleHandler(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		// This will be a call from different thread, so I can't call the OnTerminate() directly.
		// Instead, I will use flag to indicate the termination
		APPLICATION.bHasToTerminate = true;

		// Here we will try to wait for the main thread to terminate
		while (!APPLICATION.bIsReadyToTerminate)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		};

		return TRUE;
	default:
		return FALSE;
	}
}

bool FEBasicApplication::HasConsoleWindow() const
{
	return !(APPLICATION.ConsoleWindow == nullptr);
}

FEConsoleWindow* FEBasicApplication::CreateConsoleWindow(std::function<void(void* UserData)> MainFunc, void* UserData)
{
	if (MainFunc == nullptr)
		return nullptr;

	ConsoleWindow = new FEConsoleWindow(MainFunc, UserData);
	return ConsoleWindow;
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
		PostMessage(ConsoleWindow->GetHandle(), WM_CLOSE, 0, 0);
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

	bIsReadyToTerminate = true;
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
	// In these callbacks, the user can set the flag to false in order to prevent the application from closing
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
// It could be not the best solution, but it is the easiest one for now.
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
	if (Windows.empty() && (ConsoleWindow == nullptr || ConsoleWindow != nullptr && ConsoleWindow->IsHidden()))
		return false;

	return true;
}

bool FEBasicApplication::HaveAnyWindow() const
{
	if (Windows.empty() && ConsoleWindow == nullptr)
		return false;

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
			Info.Name = glfwGetMonitorName(Monitors[i]);
			glfwGetMonitorPos(Monitors[i], &Info.VirtualX, &Info.VirtualY);

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

std::vector<CommandLineAction> FEBasicApplication::ParseCommandLine(std::string CommandLine, const std::string ActionPrefix, const std::string SettingEqualizer)
{
	auto Split = [](const std::string& S, char Delimiter) {
		std::vector<std::string> Tokens;
		std::string Token;
		std::istringstream TokenStream(S);
		bool InsideQuotes = false;
		char CurrentChar;

		while (TokenStream.get(CurrentChar))
		{
			if (CurrentChar == '\"')
			{
				InsideQuotes = !InsideQuotes; // Toggle the state
			}
			else if (CurrentChar == Delimiter && !InsideQuotes)
			{
				if (!Token.empty())
				{
					Tokens.push_back(Token);
					Token.clear();
				}
			}
			else
			{
				Token += CurrentChar;
			}
		}

		if (!Token.empty())
			Tokens.push_back(Token); // Add the last token

		return Tokens;
	};

	// Remove tabs
	CommandLine.erase(std::remove(CommandLine.begin(), CommandLine.end(), '\t'), CommandLine.end());

	// Replace new lines with spaces
	std::replace(CommandLine.begin(), CommandLine.end(), '\n', ' ');
	std::vector<std::string> Tokens = Split(CommandLine, ' ');

	std::vector<CommandLineAction> Actions;
	CommandLineAction* CurrentAction = nullptr;

	for (const auto& Token : Tokens)
	{
		if (Token.substr(0, ActionPrefix.length()) == ActionPrefix)
		{
			std::string ActionName = Token.substr(ActionPrefix.length()); // Remove ActionPrefix
			Actions.push_back(CommandLineAction{ ActionName, {} });
			CurrentAction = &Actions.back(); // Point to the newly added action
		}
		else if (CurrentAction && Token.find(SettingEqualizer) != std::string::npos)
		{
			auto DelimPos = Token.find(SettingEqualizer);
			std::string Key = Token.substr(0, DelimPos);
			std::string Value = Token.substr(DelimPos + ActionPrefix.length());
			CurrentAction->Settings[Key] = Value;
		}
	}

	return Actions;
}

FEConsoleWindow* FEBasicApplication::GetConsoleWindow()
{
	return ConsoleWindow;
}

FEVirtualUI* FEBasicApplication::AddVirtualUI(GLuint FrameBuffer, int Width, int Height, std::string Name)
{
	if (Width <= 0 || Height <= 0)
		return nullptr;

	if (FrameBuffer == GLuint(-1))
		return nullptr;

	FEVirtualUI* NewVirtualUI = new FEVirtualUI(Width, Height, Name);
	NewVirtualUI->Initialize(FrameBuffer, Width, Height);
	NewVirtualUI->SetName(Name);
	APPLICATION.VirtualUIs.push_back(NewVirtualUI);
	
	return NewVirtualUI;
}

void FEBasicApplication::RemoveVirtualUI(FEVirtualUI* VirtualUI)
{
	if (VirtualUI == nullptr)
		return;

	for (size_t i = 0; i < APPLICATION.VirtualUIs.size(); i++)
	{
		if (APPLICATION.VirtualUIs[i] == VirtualUI)
		{
			delete VirtualUI;
			APPLICATION.VirtualUIs.erase(APPLICATION.VirtualUIs.begin() + i);
			break;
		}
	}
}