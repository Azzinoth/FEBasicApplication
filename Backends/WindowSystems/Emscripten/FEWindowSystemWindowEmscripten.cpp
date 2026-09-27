#include "FEWindowSystemWindowEmscripten.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"

namespace FocalEngine
{
	FEWindowSystemWindowEmscripten::~FEWindowSystemWindowEmscripten()
	{
		if (GLFWWindow != nullptr)
			glfwDestroyWindow(GLFWWindow);
	}

	void* FEWindowSystemWindowEmscripten::GetNativeHandle()
	{
		return GLFWWindow;
	}

	void FEWindowSystemWindowEmscripten::MakeContextCurrent()
	{
		if (API == GraphicsAPI::OpenGL)
			glfwMakeContextCurrent(GLFWWindow);
	}

	void FEWindowSystemWindowEmscripten::SwapBuffers()
	{
		if (API == GraphicsAPI::OpenGL)
			glfwSwapBuffers(GLFWWindow);
	}

	void FEWindowSystemWindowEmscripten::ImGuiPlatformInit(ImGuiContext* Context)
	{
		ImGui::SetCurrentContext(Context);
		ImGui_ImplGlfw_InitForOther(GLFWWindow, false);
	}

	void FEWindowSystemWindowEmscripten::ImGuiPlatformShutdown()
	{
		ImGui_ImplGlfw_Shutdown();
	}

	void FEWindowSystemWindowEmscripten::ImGuiPlatformNewFrame()
	{
		ImGui_ImplGlfw_NewFrame();
	}

	void FEWindowSystemWindowEmscripten::SetTitle(const std::string& Title)
	{
		glfwSetWindowTitle(GLFWWindow, Title.c_str());
	}

	bool FEWindowSystemWindowEmscripten::IsInFocus() const
	{
		return glfwGetWindowAttrib(GLFWWindow, GLFW_FOCUSED);
	}

	void FEWindowSystemWindowEmscripten::GetPosition(int* Xpos, int* Ypos) const
	{
		glfwGetWindowPos(GLFWWindow, Xpos, Ypos);
	}

	void FEWindowSystemWindowEmscripten::GetSize(int* Width, int* Height) const
	{
		glfwGetWindowSize(GLFWWindow, Width, Height);
	}

	void FEWindowSystemWindowEmscripten::SetSize(int Width, int Height)
	{
		glfwSetWindowSize(GLFWWindow, Width, Height);
	}

	void FEWindowSystemWindowEmscripten::Minimize() const
	{
		glfwIconifyWindow(GLFWWindow);
	}

	void FEWindowSystemWindowEmscripten::Restore() const
	{
		glfwRestoreWindow(GLFWWindow);
	}

	void FEWindowSystemWindowEmscripten::PushContext()
	{
		if (API != GraphicsAPI::OpenGL)
			return;

		SavedContext = glfwGetCurrentContext();
		if (SavedContext != GLFWWindow)
			glfwMakeContextCurrent(GLFWWindow);
	}

	void FEWindowSystemWindowEmscripten::PopContext()
	{
		if (API != GraphicsAPI::OpenGL)
			return;

		if (SavedContext != GLFWWindow)
			glfwMakeContextCurrent(SavedContext);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardMonitor(void* NativeMonitor, int Event)
	{
		ImGui_ImplGlfw_MonitorCallback(static_cast<GLFWmonitor*>(NativeMonitor), Event);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardFocus(int Focused)
	{
		ImGui_ImplGlfw_WindowFocusCallback(GLFWWindow, Focused);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardMouseEnter(int Entered)
	{
		ImGui_ImplGlfw_CursorEnterCallback(GLFWWindow, Entered);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardMouseButton(int Button, int Action, int Mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(GLFWWindow, Button, Action, Mods);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardMouseMove(double Xpos, double Ypos)
	{
		ImGui_ImplGlfw_CursorPosCallback(GLFWWindow, Xpos, Ypos);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardChar(unsigned int Codepoint)
	{
		ImGui_ImplGlfw_CharCallback(GLFWWindow, Codepoint);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardKey(int Key, int Scancode, int Action, int Mods)
	{
		ImGui_ImplGlfw_KeyCallback(GLFWWindow, Key, Scancode, Action, Mods);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardScroll(double Xoffset, double Yoffset)
	{
		ImGui_ImplGlfw_ScrollCallback(GLFWWindow, Xoffset, Yoffset);
	}
}