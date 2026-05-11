#pragma once

#include "../FEPlatformWindowInterface.h"
#include "../../FEGraphicsAPI.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace FocalEngine
{
	class FEPlatformWindowEmscripten : public FEPlatformWindowInterface
	{
		friend class FEPlatformEmscripten;
		GLFWwindow* GLFWWindow = nullptr;
		GLFWwindow* SavedContext = nullptr;
		GraphicsAPI API = GraphicsAPI::Unknown;
	public:
		~FEPlatformWindowEmscripten() override;

		void* GetNativeHandle() override;

		void MakeContextCurrent() override;
		void SwapBuffers() override;

		void ImGuiPlatformInit(ImGuiContext* Context) override;
		void ImGuiPlatformShutdown() override;
		void ImGuiPlatformNewFrame() override;

		void SetTitle(const std::string& Title) override;

		bool IsInFocus() const override;

		void GetPosition(int* Xpos, int* Ypos) const override;
		void GetSize(int* Width, int* Height) const override;
		void SetSize(int Width, int Height) override;
		void Minimize() const override;
		void Restore() const override;

		void PushContext() override;
		void PopContext() override;

		void ImGuiForwardMonitor(void* NativeMonitor, int Event) override;
		void ImGuiForwardFocus(int Focused) override;
		void ImGuiForwardMouseEnter(int Entered) override;
		void ImGuiForwardMouseButton(int Button, int Action, int Mods) override;
		void ImGuiForwardMouseMove(double Xpos, double Ypos) override;
		void ImGuiForwardChar(unsigned int Codepoint) override;
		void ImGuiForwardKey(int Key, int Scancode, int Action, int Mods) override;
		void ImGuiForwardScroll(double Xoffset, double Yoffset) override;
	};
}