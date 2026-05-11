#include "FEPlatformWindowGLFW.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"

namespace FocalEngine
{
	FEPlatformWindowGLFW::~FEPlatformWindowGLFW()
	{
		if (GLFWWindow != nullptr)
			glfwDestroyWindow(GLFWWindow);
	}

	void* FEPlatformWindowGLFW::GetNativeHandle()
	{
		return GLFWWindow;
	}

	void FEPlatformWindowGLFW::MakeContextCurrent()
	{
		if (API == GraphicsAPI::OpenGL)
			glfwMakeContextCurrent(GLFWWindow);
	}

	void FEPlatformWindowGLFW::SwapBuffers()
	{
		if (API == GraphicsAPI::OpenGL)
			glfwSwapBuffers(GLFWWindow);
	}

	void FEPlatformWindowGLFW::ImGuiPlatformInit(ImGuiContext* Context)
	{
		ImGui::SetCurrentContext(Context);
		ImGui_ImplGlfw_InitForOther(GLFWWindow, false);
	}

	void FEPlatformWindowGLFW::ImGuiPlatformShutdown()
	{
		ImGui_ImplGlfw_Shutdown();
	}

	void FEPlatformWindowGLFW::ImGuiPlatformNewFrame()
	{
		ImGui_ImplGlfw_NewFrame();
	}

	void FEPlatformWindowGLFW::SetTitle(const std::string& Title)
	{
		glfwSetWindowTitle(GLFWWindow, Title.c_str());
	}

	bool FEPlatformWindowGLFW::IsInFocus() const
	{
		return glfwGetWindowAttrib(GLFWWindow, GLFW_FOCUSED);
	}

	void FEPlatformWindowGLFW::GetPosition(int* Xpos, int* Ypos) const
	{
		glfwGetWindowPos(GLFWWindow, Xpos, Ypos);
	}

	void FEPlatformWindowGLFW::GetSize(int* Width, int* Height) const
	{
		glfwGetWindowSize(GLFWWindow, Width, Height);
	}

	void FEPlatformWindowGLFW::SetSize(int Width, int Height)
	{
		glfwSetWindowSize(GLFWWindow, Width, Height);
	}

	void FEPlatformWindowGLFW::Minimize() const
	{
		glfwIconifyWindow(GLFWWindow);
	}

	void FEPlatformWindowGLFW::Restore() const
	{
		glfwRestoreWindow(GLFWWindow);
	}

	void FEPlatformWindowGLFW::PushContext()
	{
		if (API != GraphicsAPI::OpenGL)
			return;

		SavedContext = glfwGetCurrentContext();
		if (SavedContext != GLFWWindow)
			glfwMakeContextCurrent(GLFWWindow);
	}

	void FEPlatformWindowGLFW::PopContext()
	{
		if (API != GraphicsAPI::OpenGL)
			return;

		if (SavedContext != GLFWWindow)
			glfwMakeContextCurrent(SavedContext);
	}

	void FEPlatformWindowGLFW::ImGuiForwardMonitor(void* NativeMonitor, int Event)
	{
		ImGui_ImplGlfw_MonitorCallback(static_cast<GLFWmonitor*>(NativeMonitor), Event);
	}

	void FEPlatformWindowGLFW::ImGuiForwardFocus(int Focused)
	{
		ImGui_ImplGlfw_WindowFocusCallback(GLFWWindow, Focused);
	}

	void FEPlatformWindowGLFW::ImGuiForwardMouseEnter(int Entered)
	{
		ImGui_ImplGlfw_CursorEnterCallback(GLFWWindow, Entered);
	}

	void FEPlatformWindowGLFW::ImGuiForwardMouseButton(int Button, int Action, int Mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(GLFWWindow, Button, Action, Mods);
	}

	void FEPlatformWindowGLFW::ImGuiForwardMouseMove(double Xpos, double Ypos)
	{
		ImGui_ImplGlfw_CursorPosCallback(GLFWWindow, Xpos, Ypos);
	}

	void FEPlatformWindowGLFW::ImGuiForwardChar(unsigned int Codepoint)
	{
		ImGui_ImplGlfw_CharCallback(GLFWWindow, Codepoint);
	}

	void FEPlatformWindowGLFW::ImGuiForwardKey(int Key, int Scancode, int Action, int Mods)
	{
		ImGui_ImplGlfw_KeyCallback(GLFWWindow, Key, Scancode, Action, Mods);
	}

	void FEPlatformWindowGLFW::ImGuiForwardScroll(double Xoffset, double Yoffset)
	{
		ImGui_ImplGlfw_ScrollCallback(GLFWWindow, Xoffset, Yoffset);
	}
}