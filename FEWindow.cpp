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

void FEWindow::InvokeTerminateCallback()
{
	for (int i = 0; i < UserOnTerminateCallbackFuncs.size(); i++)
		UserOnTerminateCallbackFuncs[i]();
}

void FEWindow::AddOnResizeCallback(std::function<void(int, int)> UserOnResizeCallback)
{
	UserOnResizeCallbackFuncs.push_back(UserOnResizeCallback);
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

void FEWindow::InvokeCharCallback(unsigned int codepoint)
{
	EnsureCorrectContextBegin();

	ImGui_ImplGlfw_CharCallback(GLFWWindow, codepoint);

	for (int i = 0; i < UserOnCharCallbackFuncs.size(); i++)
		UserOnCharCallbackFuncs[i](codepoint);

	EnsureCorrectContextEnd();
}

void FEWindow::AddOnKeyCallback(std::function<void(int, int, int, int)> UserOnKeyCallback)
{
	UserOnKeyCallbackFuncs.push_back(UserOnKeyCallback);
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

void FEWindow::AddOnCloseCallback(std::function<void()> UserOnCloseCallback)
{
	UserOnCloseCallbackFuncs.push_back(UserOnCloseCallback);
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