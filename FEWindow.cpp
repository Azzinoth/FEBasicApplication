#include "FEWindow.h"
using namespace FocalEngine;

FEWindow::FEWindow(int Width, int Height, std::string WindowTitle)
{
	GLFWWindow = glfwCreateWindow(Width, Height, WindowTitle.c_str(), nullptr, nullptr);
}

FEWindow::FEWindow(MonitorInfo* MonitorInfoToUse)
{
	glfwWindowHint(GLFW_RED_BITS, MonitorInfoToUse->VideoMode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, MonitorInfoToUse->VideoMode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, MonitorInfoToUse->VideoMode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, MonitorInfoToUse->VideoMode->refreshRate);
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

	GLFWWindow = glfwCreateWindow(MonitorInfoToUse->VideoMode->width, MonitorInfoToUse->VideoMode->height, "", MonitorInfoToUse->Monitor, nullptr);
	glfwSetWindowMonitor(GLFWWindow, MonitorInfoToUse->Monitor, 0, 0, MonitorInfoToUse->VideoMode->width, MonitorInfoToUse->VideoMode->height, MonitorInfoToUse->VideoMode->refreshRate);

	glfwDefaultWindowHints();
}

void FEWindow::InitializeImGui()
{
	ImguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(ImguiContext);

	// We are asking ImGui not to installing callbacks for us
	// We will do it manually because with multiple contexts we need to manage it manually
	ImGui_ImplGlfw_InitForOpenGL(GLFWWindow, false);
	ImGui_ImplOpenGL3_Init("#version 410");
}

ImGuiContext* FEWindow::GetImGuiContext() const
{
	return ImguiContext;
}

void FEWindow::TerminateImGui()
{
	ImGui::SetCurrentContext(ImguiContext);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(ImguiContext);
	ImGui::SetCurrentContext(nullptr);
}

FEWindow::~FEWindow()
{
	glfwMakeContextCurrent(GLFWWindow);
	glfwDestroyWindow(GLFWWindow);
	TerminateImGui();
}

std::string FEWindow::GetTitle() const
{
	return Title;
}

void FEWindow::SetTitle(const std::string NewValue)
{
	Title = NewValue;
	glfwSetWindowTitle(GLFWWindow, Title.c_str());
}

GLFWwindow* FEWindow::GetGlfwWindow() const
{
	return GLFWWindow;
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
	glfwMakeContextCurrent(GLFWWindow);
	ImGui::SetCurrentContext(ImguiContext);

	ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void FEWindow::Render()
{
	if (UserRenderFunctionImpl != nullptr)
		UserRenderFunctionImpl();
}

void FEWindow::EndFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(GLFWWindow);
}

bool FEWindow::IsInFocus() const
{
	return glfwGetWindowAttrib(GLFWWindow, GLFW_FOCUSED);
}

void FEWindow::EnsureCorrectContextBegin()
{
	TempImguiContext = ImGui::GetCurrentContext();
	if (TempImguiContext != ImguiContext)
		ImGui::SetCurrentContext(ImguiContext);

	TempGLFWContext = glfwGetCurrentContext();
	if (TempGLFWContext != GLFWWindow)
		glfwMakeContextCurrent(GLFWWindow);
}

void FEWindow::EnsureCorrectContextEnd()
{
	if (TempImguiContext != ImguiContext)
		ImGui::SetCurrentContext(TempImguiContext);

	if (TempGLFWContext != GLFWWindow)
		glfwMakeContextCurrent(TempGLFWContext);
}

void FEWindow::AddOnMonitorCallback(std::function<void(GLFWmonitor*, int)> UserOnMonitorCallback)
{
	UserOnMonitorCallbackFuncs.push_back(UserOnMonitorCallback);
}

void FEWindow::RemoveOnMonitorCallback(std::function<void(GLFWmonitor*, int)> UserOnMonitorCallback)
{
	for (int i = 0; i < UserOnMonitorCallbackFuncs.size(); i++)
	{
		if (UserOnMonitorCallbackFuncs[i].target_type() == UserOnMonitorCallback.target_type())
		{
			UserOnMonitorCallbackFuncs.erase(UserOnMonitorCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeMonitorCallback(GLFWmonitor* Monitor, int Event)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_MonitorCallback(Monitor, Event);

	for (int i = 0; i < UserOnMonitorCallbackFuncs.size(); i++)
		UserOnMonitorCallbackFuncs[i](Monitor, Event);

	EnsureCorrectContextEnd();
}

void FEWindow::InvokeCloseCallback()
{
	bShouldClose = true;

	for (int i = 0; i < UserOnCloseCallbackFuncs.size(); i++)
		UserOnCloseCallbackFuncs[i]();
}

void FEWindow::InvokeOnFocusCallback(int Focused)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_WindowFocusCallback(GLFWWindow, Focused);

	for (int i = 0; i < UserOnFocusCallbackFuncs.size(); i++)
		UserOnFocusCallbackFuncs[i](Focused);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnFocusCallback(std::function<void(int)> UserOnFocusCallback)
{
	UserOnFocusCallbackFuncs.push_back(UserOnFocusCallback);
}

void FEWindow::RemoveOnFocusCallback(std::function<void(int)> UserOnFocusCallback)
{
	for (int i = 0; i < UserOnFocusCallbackFuncs.size(); i++)
	{
		if (UserOnFocusCallbackFuncs[i].target_type() == UserOnFocusCallback.target_type())
		{
			UserOnFocusCallbackFuncs.erase(UserOnFocusCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeTerminateCallback()
{
	for (int i = 0; i < UserOnTerminateCallbackFuncs.size(); i++)
		UserOnTerminateCallbackFuncs[i]();
}

void FEWindow::AddOnResizeCallback(std::function<void(int, int)> UserOnResizeCallback)
{
	UserOnResizeCallbackFuncs.push_back(UserOnResizeCallback);
}

void FEWindow::RemoveOnResizeCallback(std::function<void(int, int)> UserOnResizeCallback)
{
	for (int i = 0; i < UserOnResizeCallbackFuncs.size(); i++)
	{
		if (UserOnResizeCallbackFuncs[i].target_type() == UserOnResizeCallback.target_type())
		{
			UserOnResizeCallbackFuncs.erase(UserOnResizeCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeResizeCallback(int Width, int Height)
{
	EnsureCorrectContextBegin();

	if (Width == 0 || Height == 0)
	{
		EnsureCorrectContextEnd();
		return;
	}
	
	glViewport(0, 0, Width, Height);

	ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));

	for (int i = 0; i < UserOnResizeCallbackFuncs.size(); i++)
		UserOnResizeCallbackFuncs[i](Width, Height);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnMouseEnterCallback(std::function<void(int)> UserOnMouseEnterCallback)
{
	UserOnMouseEnterCallbackFuncs.push_back(UserOnMouseEnterCallback);
}

void FEWindow::RemoveOnMouseEnterCallback(std::function<void(int)> UserOnMouseEnterCallback)
{
	for (int i = 0; i < UserOnMouseEnterCallbackFuncs.size(); i++)
	{
		if (UserOnMouseEnterCallbackFuncs[i].target_type() == UserOnMouseEnterCallback.target_type())
		{
			UserOnMouseEnterCallbackFuncs.erase(UserOnMouseEnterCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeMouseEnterCallback(int Entered)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_CursorEnterCallback(GLFWWindow, Entered);

	for (int i = 0; i < UserOnMouseEnterCallbackFuncs.size(); i++)
		UserOnMouseEnterCallbackFuncs[i](Entered);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnMouseButtonCallback(std::function<void(int, int, int)> UserOnMouseButtonCallback)
{
	UserOnMouseButtonCallbackFuncs.push_back(UserOnMouseButtonCallback);
}

void FEWindow::RemoveOnMouseButtonCallback(std::function<void(int, int, int)> UserOnMouseButtonCallback)
{
	for (int i = 0; i < UserOnMouseButtonCallbackFuncs.size(); i++)
	{
		if (UserOnMouseButtonCallbackFuncs[i].target_type() == UserOnMouseButtonCallback.target_type())
		{
			UserOnMouseButtonCallbackFuncs.erase(UserOnMouseButtonCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeMouseButtonCallback(const int Button, const int Action, const int Mods)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_MouseButtonCallback(GLFWWindow, Button, Action, Mods);

	for (int i = 0; i < UserOnMouseButtonCallbackFuncs.size(); i++)
		UserOnMouseButtonCallbackFuncs[i](Button, Action, Mods);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnMouseMoveCallback(std::function<void(double, double)> UserOnMouseMoveCallback)
{
	UserOnMouseMoveCallbackFuncs.push_back(UserOnMouseMoveCallback);
}

void FEWindow::RemoveOnMouseMoveCallback(std::function<void(double, double)> UserOnMouseMoveCallback)
{
	for (int i = 0; i < UserOnMouseMoveCallbackFuncs.size(); i++)
	{
		if (UserOnMouseMoveCallbackFuncs[i].target_type() == UserOnMouseMoveCallback.target_type())
		{
			UserOnMouseMoveCallbackFuncs.erase(UserOnMouseMoveCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeMouseMoveCallback(const double Xpos, const double Ypos)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_CursorPosCallback(GLFWWindow, Xpos, Ypos);

	for (int i = 0; i < UserOnMouseMoveCallbackFuncs.size(); i++)
		UserOnMouseMoveCallbackFuncs[i](Xpos, Ypos);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnCharCallback(std::function<void(unsigned int)> UserOnCharCallback)
{
	UserOnCharCallbackFuncs.push_back(UserOnCharCallback);
}

void FEWindow::RemoveOnCharCallback(std::function<void(unsigned int)> UserOnCharCallback)
{
	for (int i = 0; i < UserOnCharCallbackFuncs.size(); i++)
	{
		if (UserOnCharCallbackFuncs[i].target_type() == UserOnCharCallback.target_type())
		{
			UserOnCharCallbackFuncs.erase(UserOnCharCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeCharCallback(unsigned int Codepoint)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_CharCallback(GLFWWindow, Codepoint);

	for (int i = 0; i < UserOnCharCallbackFuncs.size(); i++)
		UserOnCharCallbackFuncs[i](Codepoint);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnKeyCallback(std::function<void(int, int, int, int)> UserOnKeyCallback)
{
	UserOnKeyCallbackFuncs.push_back(UserOnKeyCallback);
}

void FEWindow::RemoveOnKeyCallback(std::function<void(int, int, int, int)> UserOnKeyCallback)
{
	for (int i = 0; i < UserOnKeyCallbackFuncs.size(); i++)
	{
		if (UserOnKeyCallbackFuncs[i].target_type() == UserOnKeyCallback.target_type())
		{
			UserOnKeyCallbackFuncs.erase(UserOnKeyCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeKeyCallback(const int Key, const int Scancode, const int Action, const int Mods)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_KeyCallback(GLFWWindow, Key, Scancode, Action, Mods);

	for (int i = 0; i < UserOnKeyCallbackFuncs.size(); i++)
		UserOnKeyCallbackFuncs[i](Key, Scancode, Action, Mods);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnDropCallback(std::function<void(int, const char**)> UserOnDropCallback)
{
	UserOnDropCallbackFuncs.push_back(UserOnDropCallback);
}

void FEWindow::RemoveOnDropCallback(std::function<void(int, const char**)> UserOnDropCallback)
{
	for (int i = 0; i < UserOnDropCallbackFuncs.size(); i++)
	{
		if (UserOnDropCallbackFuncs[i].target_type() == UserOnDropCallback.target_type())
		{
			UserOnDropCallbackFuncs.erase(UserOnDropCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeDropCallback(const int Count, const char** Paths)
{
	EnsureCorrectContextBegin();

	for (int i = 0; i < UserOnDropCallbackFuncs.size(); i++)
		UserOnDropCallbackFuncs[i](Count, Paths);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnScrollCallback(std::function<void(double, double)> UserOnScrollCallback)
{
	UserOnScrollCallbackFuncs.push_back(UserOnScrollCallback);
}

void FEWindow::RemoveOnScrollCallback(std::function<void(double, double)> UserOnScrollCallback)
{
	for (int i = 0; i < UserOnScrollCallbackFuncs.size(); i++)
	{
		if (UserOnScrollCallbackFuncs[i].target_type() == UserOnScrollCallback.target_type())
		{
			UserOnScrollCallbackFuncs.erase(UserOnScrollCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::InvokeScrollCallback(const double Xoffset, const double Yoffset)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_ScrollCallback(GLFWWindow, Xoffset, Yoffset);

	for (int i = 0; i < UserOnScrollCallbackFuncs.size(); i++)
		UserOnScrollCallbackFuncs[i](Xoffset, Yoffset);

	EnsureCorrectContextEnd();
}

void FEWindow::GetPosition(int* Xpos, int* Ypos) const
{
	glfwGetWindowPos(GLFWWindow, Xpos, Ypos);
}

int FEWindow::GetXPosition() const
{
	int X, Y;
	glfwGetWindowPos(GLFWWindow, &X, &Y);
	return X;
}

int FEWindow::GetYPosition() const
{
	int X, Y;
	glfwGetWindowPos(GLFWWindow, &X, &Y);
	return Y;
}

void FEWindow::GetSize(int* Width, int* Height) const
{
	glfwGetWindowSize(GLFWWindow, Width, Height);
}

void FEWindow::SetSize(int NewWidth, int NewHeight)
{
	glfwSetWindowSize(GLFWWindow, NewWidth, NewHeight);
}

int FEWindow::GetWidth() const
{
	int Width, Height;
	glfwGetWindowSize(GLFWWindow, &Width, &Height);
	return Width;
}

int FEWindow::GetHeight() const
{
	int Width, Height;
	glfwGetWindowSize(GLFWWindow, &Width, &Height);
	return Height;
}

void FEWindow::Minimize() const
{
	glfwIconifyWindow(GLFWWindow);
}

void FEWindow::Restore() const
{
	glfwRestoreWindow(GLFWWindow);
}

std::string FEWindow::GetID() const
{
	return ID;
}

void FEWindow::AddOnTerminateCallback(std::function<void()> UserOnTerminateCallback)
{
	UserOnTerminateCallbackFuncs.push_back(UserOnTerminateCallback);
}

void FEWindow::RemoveOnTerminateCallback(std::function<void()> UserOnTerminateCallback)
{
	for (int i = 0; i < UserOnTerminateCallbackFuncs.size(); i++)
	{
		if (UserOnTerminateCallbackFuncs[i].target_type() == UserOnTerminateCallback.target_type())
		{
			UserOnTerminateCallbackFuncs.erase(UserOnTerminateCallbackFuncs.begin() + i);
			break;
		}
	}
}

void FEWindow::AddOnCloseCallback(std::function<void()> UserOnCloseCallback)
{
	UserOnCloseCallbackFuncs.push_back(UserOnCloseCallback);
}

void FEWindow::RemoveOnCloseCallback(std::function<void()> UserOnCloseCallback)
{
	for (int i = 0; i < UserOnCloseCallbackFuncs.size(); i++)
	{
		if (UserOnCloseCallbackFuncs[i].target_type() == UserOnCloseCallback.target_type())
		{
			UserOnCloseCallbackFuncs.erase(UserOnCloseCallbackFuncs.begin() + i);
			break;
		}
	}
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
	int X, Y, Width, Height;
	glfwGetWindowPos(GLFWWindow, &X, &Y);
	glfwGetWindowSize(GLFWWindow, &Width, &Height);

	int MonitorsCount;
	GLFWmonitor** Monitors = glfwGetMonitors(&MonitorsCount);

	MonitorInfo BestMonitor;
	int BestArea = 0;

	for (int i = 0; i < MonitorsCount; i++)
	{
		int MonitorX, MonitorY, MonitorWidth, MonitorHeight;
		glfwGetMonitorWorkarea(Monitors[i], &MonitorX, &MonitorY, &MonitorWidth, &MonitorHeight);

		int MinX = max(X, MonitorX);
		int MinY = max(Y, MonitorY);
		int MaxX = min(X + Width, MonitorX + MonitorWidth);
		int MaxY = min(Y + Height, MonitorY + MonitorHeight);

		int Area = max(0, MaxX - MinX) * max(0, MaxY - MinY);

		if (Area > BestArea)
		{
			BestArea = Area;
			BestMonitor.Monitor = Monitors[i];
			BestMonitor.VideoMode = glfwGetVideoMode(Monitors[i]);
		}
	}

	return BestMonitor;
}