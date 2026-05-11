#include "FEPlatformWindowEmscripten.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"

namespace FocalEngine
{
	FEPlatformWindowEmscripten::~FEPlatformWindowEmscripten()
	{
		if (GLFWWindow != nullptr)
			glfwDestroyWindow(GLFWWindow);
	}

	void* FEPlatformWindowEmscripten::GetNativeHandle()
	{
		return GLFWWindow;
	}

	void FEPlatformWindowEmscripten::MakeContextCurrent()
	{
		if (API == GraphicsAPI::OpenGL)
			glfwMakeContextCurrent(GLFWWindow);
	}

	void FEPlatformWindowEmscripten::SwapBuffers()
	{
		if (API == GraphicsAPI::OpenGL)
			glfwSwapBuffers(GLFWWindow);
	}

	void FEPlatformWindowEmscripten::ImGuiPlatformInit(ImGuiContext* Context)
	{
		ImGui::SetCurrentContext(Context);
		ImGui_ImplGlfw_InitForOther(GLFWWindow, false);
	}

	void FEPlatformWindowEmscripten::ImGuiPlatformShutdown()
	{
		ImGui_ImplGlfw_Shutdown();
	}

	void FEPlatformWindowEmscripten::ImGuiPlatformNewFrame()
	{
		ImGui_ImplGlfw_NewFrame();
	}

	void FEPlatformWindowEmscripten::SetTitle(const std::string& Title)
	{
		glfwSetWindowTitle(GLFWWindow, Title.c_str());
	}

	bool FEPlatformWindowEmscripten::IsInFocus() const
	{
		return glfwGetWindowAttrib(GLFWWindow, GLFW_FOCUSED);
	}

	void FEPlatformWindowEmscripten::GetPosition(int* Xpos, int* Ypos) const
	{
		glfwGetWindowPos(GLFWWindow, Xpos, Ypos);
	}

	void FEPlatformWindowEmscripten::GetSize(int* Width, int* Height) const
	{
		glfwGetWindowSize(GLFWWindow, Width, Height);
	}

	void FEPlatformWindowEmscripten::SetSize(int Width, int Height)
	{
		glfwSetWindowSize(GLFWWindow, Width, Height);
	}

	void FEPlatformWindowEmscripten::Minimize() const
	{
		glfwIconifyWindow(GLFWWindow);
	}

	void FEPlatformWindowEmscripten::Restore() const
	{
		glfwRestoreWindow(GLFWWindow);
	}

	void FEPlatformWindowEmscripten::PushContext()
	{
		if (API != GraphicsAPI::OpenGL)
			return;

		SavedContext = glfwGetCurrentContext();
		if (SavedContext != GLFWWindow)
			glfwMakeContextCurrent(GLFWWindow);
	}

	void FEPlatformWindowEmscripten::PopContext()
	{
		if (API != GraphicsAPI::OpenGL)
			return;

		if (SavedContext != GLFWWindow)
			glfwMakeContextCurrent(SavedContext);
	}

	void FEPlatformWindowEmscripten::ImGuiForwardMonitor(void* NativeMonitor, int Event)
	{
		ImGui_ImplGlfw_MonitorCallback(static_cast<GLFWmonitor*>(NativeMonitor), Event);
	}

	void FEPlatformWindowEmscripten::ImGuiForwardFocus(int Focused)
	{
		ImGui_ImplGlfw_WindowFocusCallback(GLFWWindow, Focused);
	}

	void FEPlatformWindowEmscripten::ImGuiForwardMouseEnter(int Entered)
	{
		ImGui_ImplGlfw_CursorEnterCallback(GLFWWindow, Entered);
	}

	void FEPlatformWindowEmscripten::ImGuiForwardMouseButton(int Button, int Action, int Mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(GLFWWindow, Button, Action, Mods);
	}

	void FEPlatformWindowEmscripten::ImGuiForwardMouseMove(double Xpos, double Ypos)
	{
		ImGui_ImplGlfw_CursorPosCallback(GLFWWindow, Xpos, Ypos);
	}

	void FEPlatformWindowEmscripten::ImGuiForwardChar(unsigned int Codepoint)
	{
		ImGui_ImplGlfw_CharCallback(GLFWWindow, Codepoint);
	}

	void FEPlatformWindowEmscripten::ImGuiForwardKey(int Key, int Scancode, int Action, int Mods)
	{
		ImGui_ImplGlfw_KeyCallback(GLFWWindow, Key, Scancode, Action, Mods);
	}

	void FEPlatformWindowEmscripten::ImGuiForwardScroll(double Xoffset, double Yoffset)
	{
		ImGui_ImplGlfw_ScrollCallback(GLFWWindow, Xoffset, Yoffset);
	}
}