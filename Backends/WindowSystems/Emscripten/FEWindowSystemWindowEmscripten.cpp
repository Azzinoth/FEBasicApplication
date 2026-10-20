#include "FEWindowSystemWindowEmscripten.h"
#include "FEWindowSystemEmscripten.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include <emscripten.h>

// Emscripten's GLFW reports touches as mouse input, so the browser's pointer type is used to tell ImGui which one it is.
EM_JS(void, FEBrowserInstallPointerTypeListener, (), {
	if (Module.FEBrowserPointerTypeListenerInstalled)
		return;

	Module.FEBrowserPointerTypeListenerInstalled = true;
	const Update = (Event) => { Module.FEBrowserPointerIsTouch = Event.pointerType === "touch"; };
	document.addEventListener("pointerdown", Update, true);
	document.addEventListener("pointermove", Update, true);
});

EM_JS(int, FEBrowserIsPointerTouch, (), {
	return Module.FEBrowserPointerIsTouch ? 1 : 0;
});

// Emscripten's GLFW only follows one finger, so a two-finger pinch is collected here and turned into mouse wheel steps in ImGuiPlatformNewFrame.
EM_JS(void, FEBrowserInstallPinchListener, (), {
	if (Module.FEBrowserPinch)
		return;

	const Pinch = Module.FEBrowserPinch = { Active: false, Started: false, Distance: 0, Scale: 0, X: 0, Y: 0 };
	const OnTouch = (Event) => {
		const Canvas = Module.canvas;
		if (Event.target !== Canvas)
			return;

		if (Event.touches.length >= 2)
		{
			const A = Event.touches[0];
			const B = Event.touches[1];
			const Distance = Math.hypot(A.clientX - B.clientX, A.clientY - B.clientY);
			if (!Pinch.Active)
			{
				Pinch.Active = true;
				Pinch.Started = true;
				Pinch.Scale = 0;
			}
			else if (Pinch.Distance > 0 && Distance > 0)
			{
				Pinch.Scale += Math.log(Distance / Pinch.Distance);
			}

			const Rect = Canvas.getBoundingClientRect();
			Pinch.Distance = Distance;
			Pinch.X = ((A.clientX + B.clientX) / 2 - Rect.left) * Canvas.width / Rect.width;
			Pinch.Y = ((A.clientY + B.clientY) / 2 - Rect.top) * Canvas.height / Rect.height;
		}
		else if (Event.touches.length === 0)
		{
			Pinch.Active = false;
		}

		// GLFW does not see new fingers or finger movement until all fingers are lifted, so a pinch does not also drag.
		if (Pinch.Active && (Event.type === "touchstart" || Event.type === "touchmove"))
		{
			Event.preventDefault();
			Event.stopPropagation();
		}
	};

	for (const Type of ["touchstart", "touchmove", "touchend", "touchcancel"])
		window.addEventListener(Type, OnTouch, { capture: true, passive: false });
});

EM_JS(int, FEBrowserTakePinchStart, (), {
	const Pinch = Module.FEBrowserPinch;
	if (!Pinch || !Pinch.Started)
		return 0;

	Pinch.Started = false;
	return 1;
});

// One wheel step for each 15% change of the distance between the fingers, at most one step per frame.
EM_JS(int, FEBrowserTakePinchWheelStep, (), {
	const Pinch = Module.FEBrowserPinch;
	const StepSize = Math.log(1.15);
	if (!Pinch || Math.abs(Pinch.Scale) < StepSize)
		return 0;

	const Step = Math.sign(Pinch.Scale);
	Pinch.Scale -= Step * StepSize;
	return Step;
});

EM_JS(double, FEBrowserGetPinchX, (), {
	return Module.FEBrowserPinch.X;
});

EM_JS(double, FEBrowserGetPinchY, (), {
	return Module.FEBrowserPinch.Y;
});

namespace FocalEngine
{
	static void ForwardMouseSource()
	{
		ImGui::GetIO().AddMouseSourceEvent(FEBrowserIsPointerTouch() ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
	}

	FEWindowSystemWindowEmscripten::~FEWindowSystemWindowEmscripten()
	{
		if (GLFWWindow != nullptr)
			glfwDestroyWindow(GLFWWindow);
	}

	void* FEWindowSystemWindowEmscripten::GetNativeHandle()
	{
		return GLFWWindow;
	}

	void* FEWindowSystemWindowEmscripten::GetPlatformWindowHandle()
	{
		// Browser canvas has no operating system window.
		return nullptr;
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
		ImGui_ImplGlfw_InstallEmscriptenCallbacks(GLFWWindow, "#canvas");
		FEBrowserInstallPointerTypeListener();
		FEBrowserInstallPinchListener();

		// Emscripten's GLFW does not implement cursor shapes, and its glfwSetInputMode calls document.exitPointerLock, which iPhone Safari does not have.
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		// The ImGui GLFW backend uses glfwSetClipboardString/glfwGetClipboardString, which do nothing in Emscripten's built-in GLFW.
		ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
		PlatformIO.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* Text) { SetBrowserClipboardText(Text); };
		PlatformIO.Platform_GetClipboardTextFn = [](ImGuiContext*) { return GetBrowserClipboardText(); };
	}

	void FEWindowSystemWindowEmscripten::ImGuiPlatformShutdown()
	{
		ImGui_ImplGlfw_Shutdown();
	}

	void FEWindowSystemWindowEmscripten::ImGuiPlatformNewFrame()
	{
		ImGui_ImplGlfw_NewFrame();

		// The first finger already pressed the left button, a pinch releases it so it does not drag.
		ImGuiIO& IO = ImGui::GetIO();
		if (FEBrowserTakePinchStart())
			IO.AddMouseButtonEvent(ImGuiMouseButton_Left, false);

		const int PinchWheelStep = FEBrowserTakePinchWheelStep();
		if (PinchWheelStep != 0)
		{
			IO.AddMousePosEvent(static_cast<float>(FEBrowserGetPinchX()), static_cast<float>(FEBrowserGetPinchY()));
			IO.AddMouseWheelEvent(0.0f, static_cast<float>(PinchWheelStep));
		}
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
		ForwardMouseSource();

		// Emscripten's GLFW reports a touch press without a cursor move, so ImGui would see it at the previous touch position.
		double Xpos = 0.0;
		double Ypos = 0.0;
		glfwGetCursorPos(GLFWWindow, &Xpos, &Ypos);
		ImGui_ImplGlfw_CursorPosCallback(GLFWWindow, Xpos, Ypos);

		ImGui_ImplGlfw_MouseButtonCallback(GLFWWindow, Button, Action, Mods);
	}

	void FEWindowSystemWindowEmscripten::ImGuiForwardMouseMove(double Xpos, double Ypos)
	{
		ForwardMouseSource();
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