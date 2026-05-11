#include "FEWindow.h"
#include "FEBasicApplication.h"
using namespace FocalEngine;

void FEWindow::InitializeImGui()
{
	ImguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(ImguiContext);

	PlatformWindow->ImGuiPlatformInit(ImguiContext);
	DeviceSurface->ImGuiInit();
}

ImGuiContext* FEWindow::GetImGuiContext() const
{
	return ImguiContext;
}

void FEWindow::TerminateImGui()
{
	ImGui::SetCurrentContext(ImguiContext);

	DeviceSurface->ImGuiShutdown();
	PlatformWindow->ImGuiPlatformShutdown();

	ImGui::DestroyContext(ImguiContext);
	ImGui::SetCurrentContext(nullptr);
}

FEWindow::~FEWindow()
{
	if (PlatformWindow != nullptr)
		PlatformWindow->MakeContextCurrent();
	TerminateImGui();

	delete DeviceSurface;
	DeviceSurface = nullptr;
	delete PlatformWindow;
	PlatformWindow = nullptr;
}

std::string FEWindow::GetTitle() const
{
	return Title;
}

void FEWindow::SetTitle(const std::string NewValue)
{
	Title = NewValue;
	PlatformWindow->SetTitle(Title);
}

GLFWwindow* FEWindow::GetGlfwWindow() const
{
	return static_cast<GLFWwindow*>(PlatformWindow->GetNativeHandle());
}

std::function<void()> FEWindow::GetRenderFunction()
{
	return UserRenderFunctionImpl;
}

void FEWindow::SetRenderFunction(std::function<void()> UserRenderFunction)
{
	UserRenderFunctionImpl = UserRenderFunction;
}

void FEWindow::ClearRenderFunction()
{
	UserRenderFunctionImpl = nullptr;
}

void FEWindow::BeginFrame()
{
	PlatformWindow->MakeContextCurrent();
	ImGui::SetCurrentContext(ImguiContext);

	ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
	DeviceSurface->BeginFrame();
	DeviceSurface->ImGuiNewFrame();
	PlatformWindow->ImGuiPlatformNewFrame();
	ImGui::NewFrame();

	if (bDefaultDockspaceEnabled)
		DefaultDockspaceID = ImGui::DockSpaceOverViewport(0U, ImGui::GetMainViewport());
}

void FEWindow::Render()
{
	if (UserRenderFunctionImpl != nullptr)
		UserRenderFunctionImpl();
}

void FEWindow::EndFrame()
{
	ImGui::Render();
	DeviceSurface->ImGuiRenderDrawData();
	DeviceSurface->EndFrame();
	PlatformWindow->SwapBuffers();
}

bool FEWindow::IsInFocus() const
{
	return PlatformWindow->IsInFocus();
}

void FEWindow::SetClearColor(float R, float G, float B, float A)
{
	DeviceSurface->SetClearColor(R, G, B, A);
}

void FEWindow::EnableDefaultDockspace()
{
	bDefaultDockspaceEnabled = true;
}

bool FEWindow::HasDefaultDockspace() const
{
	return bDefaultDockspaceEnabled;
}

ImGuiID FEWindow::GetDefaultDockspaceID() const
{
	return DefaultDockspaceID;
}

void FEWindow::EnsureCorrectContextBegin()
{
	if (ImguiContext == nullptr)
		return;

	TemporaryImguiContext = ImGui::GetCurrentContext();
	if (TemporaryImguiContext != ImguiContext)
		ImGui::SetCurrentContext(ImguiContext);

	PlatformWindow->PushContext();
}

void FEWindow::EnsureCorrectContextEnd()
{
	if (ImguiContext == nullptr)
		return;

	if (TemporaryImguiContext != ImguiContext)
		ImGui::SetCurrentContext(TemporaryImguiContext);

	PlatformWindow->PopContext();
}

std::string FEWindow::AddOnMonitorCallback(std::function<void(GLFWmonitor*, int)> UserOnMonitorCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnMonitorCallback);
	UserOnMonitorCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeMonitorCallback(GLFWmonitor* Monitor, int Event)
{
	EnsureCorrectContextBegin();

	PlatformWindow->ImGuiForwardMonitor(Monitor, Event);

	for (int i = 0; i < UserOnMonitorCallbackFuncs.size(); i++)
		UserOnMonitorCallbackFuncs[i].second(Monitor, Event);

	EnsureCorrectContextEnd();
}

void FEWindow::InvokeCloseCallback()
{
	bShouldClose = true;

	for (int i = 0; i < UserOnCloseCallbackFuncs.size(); i++)
		UserOnCloseCallbackFuncs[i].second();
}

void FEWindow::InvokeOnFocusCallback(int Focused)
{
	EnsureCorrectContextBegin();

	PlatformWindow->ImGuiForwardFocus(Focused);

	for (int i = 0; i < UserOnFocusCallbackFuncs.size(); i++)
		UserOnFocusCallbackFuncs[i].second(Focused);

	EnsureCorrectContextEnd();
}

std::string FEWindow::AddOnFocusCallback(std::function<void(int)> UserOnFocusCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnFocusCallback);
	UserOnFocusCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeTerminateCallback()
{
	for (int i = 0; i < UserOnTerminateCallbackFuncs.size(); i++)
		UserOnTerminateCallbackFuncs[i].second();
}

std::string FEWindow::AddOnResizeCallback(std::function<void(int, int)> UserOnResizeCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnResizeCallback);
	UserOnResizeCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeResizeCallback(int Width, int Height)
{
	EnsureCorrectContextBegin();

	if (Width == 0 || Height == 0)
	{
		EnsureCorrectContextEnd();
		return;
	}
	
	DeviceSurface->Resize(Width, Height);
	ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));

	for (int i = 0; i < UserOnResizeCallbackFuncs.size(); i++)
		UserOnResizeCallbackFuncs[i].second(Width, Height);

	EnsureCorrectContextEnd();
}

std::string FEWindow::AddOnMouseEnterCallback(std::function<void(int)> UserOnMouseEnterCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnMouseEnterCallback);
	UserOnMouseEnterCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeMouseEnterCallback(int Entered)
{
	EnsureCorrectContextBegin();

	PlatformWindow->ImGuiForwardMouseEnter(Entered);

	for (int i = 0; i < UserOnMouseEnterCallbackFuncs.size(); i++)
		UserOnMouseEnterCallbackFuncs[i].second(Entered);

	EnsureCorrectContextEnd();
}

std::string FEWindow::AddOnMouseButtonCallback(std::function<void(int, int, int)> UserOnMouseButtonCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnMouseButtonCallback);
	UserOnMouseButtonCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeMouseButtonCallback(const int Button, const int Action, const int Mods)
{
	EnsureCorrectContextBegin();

	PlatformWindow->ImGuiForwardMouseButton(Button, Action, Mods);

	for (int i = 0; i < UserOnMouseButtonCallbackFuncs.size(); i++)
		UserOnMouseButtonCallbackFuncs[i].second(Button, Action, Mods);

	EnsureCorrectContextEnd();
}

std::string FEWindow::AddOnMouseMoveCallback(std::function<void(double, double)> UserOnMouseMoveCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnMouseMoveCallback);
	UserOnMouseMoveCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeMouseMoveCallback(const double Xpos, const double Ypos)
{
	EnsureCorrectContextBegin();

	PlatformWindow->ImGuiForwardMouseMove(Xpos, Ypos);

	for (int i = 0; i < UserOnMouseMoveCallbackFuncs.size(); i++)
		UserOnMouseMoveCallbackFuncs[i].second(Xpos, Ypos);

	EnsureCorrectContextEnd();
}

std::string FEWindow::AddOnCharCallback(std::function<void(unsigned int)> UserOnCharCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnCharCallback);
	UserOnCharCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeCharCallback(unsigned int Codepoint)
{
	EnsureCorrectContextBegin();

	PlatformWindow->ImGuiForwardChar(Codepoint);

	for (int i = 0; i < UserOnCharCallbackFuncs.size(); i++)
		UserOnCharCallbackFuncs[i].second(Codepoint);

	EnsureCorrectContextEnd();
}

std::string FEWindow::AddOnKeyCallback(std::function<void(int, int, int, int)> UserOnKeyCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnKeyCallback);
	UserOnKeyCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeKeyCallback(const int Key, const int Scancode, const int Action, const int Mods)
{
	EnsureCorrectContextBegin();

	PlatformWindow->ImGuiForwardKey(Key, Scancode, Action, Mods);

	for (int i = 0; i < UserOnKeyCallbackFuncs.size(); i++)
		UserOnKeyCallbackFuncs[i].second(Key, Scancode, Action, Mods);

	EnsureCorrectContextEnd();
}

std::string FEWindow::AddOnDropCallback(std::function<void(int, const char**)> UserOnDropCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnDropCallback);
	UserOnDropCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeDropCallback(const int Count, const char** Paths)
{
	EnsureCorrectContextBegin();

	for (int i = 0; i < UserOnDropCallbackFuncs.size(); i++)
		UserOnDropCallbackFuncs[i].second(Count, Paths);

	EnsureCorrectContextEnd();
}

std::string FEWindow::AddOnScrollCallback(std::function<void(double, double)> UserOnScrollCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnScrollCallback);
	UserOnScrollCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::InvokeScrollCallback(const double Xoffset, const double Yoffset)
{
	EnsureCorrectContextBegin();

	PlatformWindow->ImGuiForwardScroll(Xoffset, Yoffset);

	for (int i = 0; i < UserOnScrollCallbackFuncs.size(); i++)
		UserOnScrollCallbackFuncs[i].second(Xoffset, Yoffset);

	EnsureCorrectContextEnd();
}

void FEWindow::GetPosition(int* Xpos, int* Ypos) const
{
	PlatformWindow->GetPosition(Xpos, Ypos);
}

int FEWindow::GetXPosition() const
{
	int X, Y;
	PlatformWindow->GetPosition(&X, &Y);
	return X;
}

int FEWindow::GetYPosition() const
{
	int X, Y;
	PlatformWindow->GetPosition(&X, &Y);
	return Y;
}

void FEWindow::GetSize(int* Width, int* Height) const
{
	PlatformWindow->GetSize(Width, Height);
}

void FEWindow::SetSize(int NewWidth, int NewHeight)
{
	PlatformWindow->SetSize(NewWidth, NewHeight);
}

int FEWindow::GetWidth() const
{
	int Width, Height;
	PlatformWindow->GetSize(&Width, &Height);
	return Width;
}

int FEWindow::GetHeight() const
{
	int Width, Height;
	PlatformWindow->GetSize(&Width, &Height);
	return Height;
}

void FEWindow::Minimize() const
{
	PlatformWindow->Minimize();
}

void FEWindow::Restore() const
{
	PlatformWindow->Restore();
}

std::string FEWindow::GetID() const
{
	return ID;
}

std::string FEWindow::AddOnTerminateCallback(std::function<void()> UserOnTerminateCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnTerminateCallback);
	UserOnTerminateCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

std::string FEWindow::AddOnCloseCallback(std::function<void()> UserOnCloseCallback)
{
	std::pair NewCallback = std::make_pair(UNIQUE_ID.GetUniqueHexID(), UserOnCloseCallback);
	UserOnCloseCallbackFuncs.push_back(NewCallback);

	return NewCallback.first;
}

void FEWindow::CancelClose()
{
	bShouldClose = false;
}

void FEWindow::Terminate()
{
	bShouldTerminate = true;
}

MonitorInfo FEWindow::DetermineCurrentMonitor()
{
	MonitorInfo BestMonitor = APPLICATION.GetMonitorContainingWindow(this);

	return BestMonitor;
}

void FEWindow::RemoveCallback(std::string CallbackID)
{
	for (int i = 0; i < UserOnCloseCallbackFuncs.size(); i++)
	{
		if (UserOnCloseCallbackFuncs[i].first == CallbackID)
		{
			UserOnCloseCallbackFuncs.erase(UserOnCloseCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnFocusCallbackFuncs.size(); i++)
	{
		if (UserOnFocusCallbackFuncs[i].first == CallbackID)
		{
			UserOnFocusCallbackFuncs.erase(UserOnFocusCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnTerminateCallbackFuncs.size(); i++)
	{
		if (UserOnTerminateCallbackFuncs[i].first == CallbackID)
		{
			UserOnTerminateCallbackFuncs.erase(UserOnTerminateCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnResizeCallbackFuncs.size(); i++)
	{
		if (UserOnResizeCallbackFuncs[i].first == CallbackID)
		{
			UserOnResizeCallbackFuncs.erase(UserOnResizeCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnMouseEnterCallbackFuncs.size(); i++)
	{
		if (UserOnMouseEnterCallbackFuncs[i].first == CallbackID)
		{
			UserOnMouseEnterCallbackFuncs.erase(UserOnMouseEnterCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnMouseButtonCallbackFuncs.size(); i++)
	{
		if (UserOnMouseButtonCallbackFuncs[i].first == CallbackID)
		{
			UserOnMouseButtonCallbackFuncs.erase(UserOnMouseButtonCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnMouseMoveCallbackFuncs.size(); i++)
	{
		if (UserOnMouseMoveCallbackFuncs[i].first == CallbackID)
		{
			UserOnMouseMoveCallbackFuncs.erase(UserOnMouseMoveCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnCharCallbackFuncs.size(); i++)
	{
		if (UserOnCharCallbackFuncs[i].first == CallbackID)
		{
			UserOnCharCallbackFuncs.erase(UserOnCharCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnKeyCallbackFuncs.size(); i++)
	{
		if (UserOnKeyCallbackFuncs[i].first == CallbackID)
		{
			UserOnKeyCallbackFuncs.erase(UserOnKeyCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnDropCallbackFuncs.size(); i++)
	{
		if (UserOnDropCallbackFuncs[i].first == CallbackID)
		{
			UserOnDropCallbackFuncs.erase(UserOnDropCallbackFuncs.begin() + i);
			return;
		}
	}

	for (int i = 0; i < UserOnScrollCallbackFuncs.size(); i++)
	{
		if (UserOnScrollCallbackFuncs[i].first == CallbackID)
		{
			UserOnScrollCallbackFuncs.erase(UserOnScrollCallbackFuncs.begin() + i);
			return;
		}
	}
}