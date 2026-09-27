#include "FEWindowSystemWindowGLFW.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"

namespace FocalEngine
{
	FEWindowSystemWindowGLFW::~FEWindowSystemWindowGLFW()
	{
		if (GLFWWindow != nullptr)
			glfwDestroyWindow(GLFWWindow);
	}

	void* FEWindowSystemWindowGLFW::GetNativeHandle()
	{
		return GLFWWindow;
	}

	void FEWindowSystemWindowGLFW::MakeContextCurrent()
	{
		if (API == GraphicsAPI::OpenGL)
			glfwMakeContextCurrent(GLFWWindow);
	}

	void FEWindowSystemWindowGLFW::SwapBuffers()
	{
		if (API == GraphicsAPI::OpenGL)
			glfwSwapBuffers(GLFWWindow);
	}

	void FEWindowSystemWindowGLFW::ImGuiPlatformInit(ImGuiContext* Context)
	{
		ImGui::SetCurrentContext(Context);
		ImGui_ImplGlfw_InitForOther(GLFWWindow, false);
	}

	void FEWindowSystemWindowGLFW::ImGuiPlatformShutdown()
	{
		ImGui_ImplGlfw_Shutdown();
	}

	void FEWindowSystemWindowGLFW::ImGuiPlatformNewFrame()
	{
		ImGui_ImplGlfw_NewFrame();
	}

	void FEWindowSystemWindowGLFW::SetTitle(const std::string& Title)
	{
		glfwSetWindowTitle(GLFWWindow, Title.c_str());
	}

	bool FEWindowSystemWindowGLFW::IsInFocus() const
	{
		return glfwGetWindowAttrib(GLFWWindow, GLFW_FOCUSED);
	}

	void FEWindowSystemWindowGLFW::GetPosition(int* Xpos, int* Ypos) const
	{
		glfwGetWindowPos(GLFWWindow, Xpos, Ypos);
	}

	void FEWindowSystemWindowGLFW::GetSize(int* Width, int* Height) const
	{
		glfwGetWindowSize(GLFWWindow, Width, Height);
	}

	void FEWindowSystemWindowGLFW::SetSize(int Width, int Height)
	{
		glfwSetWindowSize(GLFWWindow, Width, Height);
	}

	void FEWindowSystemWindowGLFW::Minimize() const
	{
		glfwIconifyWindow(GLFWWindow);
	}

	void FEWindowSystemWindowGLFW::Restore() const
	{
		glfwRestoreWindow(GLFWWindow);
	}

	void FEWindowSystemWindowGLFW::PushContext()
	{
		if (API != GraphicsAPI::OpenGL)
			return;

		SavedContext = glfwGetCurrentContext();
		if (SavedContext != GLFWWindow)
			glfwMakeContextCurrent(GLFWWindow);
	}

	void FEWindowSystemWindowGLFW::PopContext()
	{
		if (API != GraphicsAPI::OpenGL)
			return;

		if (SavedContext != GLFWWindow)
			glfwMakeContextCurrent(SavedContext);
	}

	void FEWindowSystemWindowGLFW::ImGuiForwardMonitor(void* NativeMonitor, int Event)
	{
		ImGui_ImplGlfw_MonitorCallback(static_cast<GLFWmonitor*>(NativeMonitor), Event);
	}

	void FEWindowSystemWindowGLFW::ImGuiForwardFocus(int Focused)
	{
		ImGui_ImplGlfw_WindowFocusCallback(GLFWWindow, Focused);
	}

	void FEWindowSystemWindowGLFW::ImGuiForwardMouseEnter(int Entered)
	{
		ImGui_ImplGlfw_CursorEnterCallback(GLFWWindow, Entered);
	}

	void FEWindowSystemWindowGLFW::ImGuiForwardMouseButton(int Button, int Action, int Mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(GLFWWindow, Button, Action, Mods);
	}

	void FEWindowSystemWindowGLFW::ImGuiForwardMouseMove(double Xpos, double Ypos)
	{
		ImGui_ImplGlfw_CursorPosCallback(GLFWWindow, Xpos, Ypos);
	}

	void FEWindowSystemWindowGLFW::ImGuiForwardChar(unsigned int Codepoint)
	{
		ImGui_ImplGlfw_CharCallback(GLFWWindow, Codepoint);
	}

	void FEWindowSystemWindowGLFW::ImGuiForwardKey(int Key, int Scancode, int Action, int Mods)
	{
		ImGui_ImplGlfw_KeyCallback(GLFWWindow, Key, Scancode, Action, Mods);
	}

	void FEWindowSystemWindowGLFW::ImGuiForwardScroll(double Xoffset, double Yoffset)
	{
		ImGui_ImplGlfw_ScrollCallback(GLFWWindow, Xoffset, Yoffset);
	}
}