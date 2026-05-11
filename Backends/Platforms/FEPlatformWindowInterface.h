#pragma once

#include "../FEThreadPool.h"

struct ImGuiContext;

namespace FocalEngine
{
	class FEPlatformWindowInterface
	{
	public:
		virtual ~FEPlatformWindowInterface() = default;

		virtual void* GetNativeHandle() = 0;

		virtual void MakeContextCurrent() = 0;
		virtual void SwapBuffers() = 0;

		virtual void ImGuiPlatformInit(ImGuiContext* Context) = 0;
		virtual void ImGuiPlatformShutdown() = 0;
		virtual void ImGuiPlatformNewFrame() = 0;

		virtual void SetTitle(const std::string& Title) = 0;

		virtual bool IsInFocus() const = 0;

		virtual void GetPosition(int* Xpos, int* Ypos) const = 0;
		virtual void GetSize(int* Width, int* Height) const = 0;
		virtual void SetSize(int Width, int Height) = 0;
		virtual void Minimize() const = 0;
		virtual void Restore() const = 0;

		virtual void PushContext() = 0;
		virtual void PopContext() = 0;

		// Forward GLFW-style events to the platform's ImGui backend.
		// FE_FIX_ME: NativeMonitor is currently GLFWmonitor*, define an opaque monitor handle later.
		virtual void ImGuiForwardMonitor(void* NativeMonitor, int Event) = 0;
		virtual void ImGuiForwardFocus(int Focused) = 0;
		virtual void ImGuiForwardMouseEnter(int Entered) = 0;
		virtual void ImGuiForwardMouseButton(int Button, int Action, int Mods) = 0;
		virtual void ImGuiForwardMouseMove(double Xpos, double Ypos) = 0;
		virtual void ImGuiForwardChar(unsigned int Codepoint) = 0;
		virtual void ImGuiForwardKey(int Key, int Scancode, int Action, int Mods) = 0;
		virtual void ImGuiForwardScroll(double Xoffset, double Yoffset) = 0;
	};
}
