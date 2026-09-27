#include "FEWindowSystemEmscripten.h"
#include <algorithm>
#include <emscripten.h>

namespace FocalEngine
{
	static std::function<void(void* NativeMonitor, int Event)> RegisteredMonitorCallback;
	static void MonitorCallbackBridge(GLFWmonitor* Monitor, int Event)
	{
		if (RegisteredMonitorCallback)
			RegisteredMonitorCallback(static_cast<void*>(Monitor), Event);
	}

	FEWindowSystemInterface* CreateWindowSystem()
	{
		return new FEWindowSystemEmscripten();
	}

	FEWindowSystemEmscripten::~FEWindowSystemEmscripten() {}

	bool FEWindowSystemEmscripten::Initialize()
	{
		return glfwInit() == GLFW_TRUE;
	}

	void FEWindowSystemEmscripten::Shutdown()
	{
		glfwTerminate();
	}

	void FEWindowSystemEmscripten::PollEvents()
	{
		glfwPollEvents();
	}

	// emscripten_set_main_loop takes a non-capturing C function pointer, so we route
	// the std::function through a file-scope storage + thin dispatch stub.
	static std::function<void()> StoredMainLoopTick;
	static void DispatchMainLoopTick() { if (StoredMainLoopTick) StoredMainLoopTick(); }

	void FEWindowSystemEmscripten::RunMainLoop(std::function<void()> Tick)
	{
		StoredMainLoopTick = std::move(Tick);
		// fps=0 -> use requestAnimationFrame, simulate_infinite_loop=1 -> never returns to caller.
		emscripten_set_main_loop(DispatchMainLoopTick, 0, 1);
	}

	double FEWindowSystemEmscripten::GetTime()
	{
		return glfwGetTime();
	}

	FEWindowSystemWindowInterface* FEWindowSystemEmscripten::OpenWindow(int Width, int Height, std::string Title, GraphicsAPI API)
	{
		if (API == GraphicsAPI::WebGPU)
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		FEWindowSystemWindowEmscripten* WindowSystemWindow = new FEWindowSystemWindowEmscripten();
		WindowSystemWindow->GLFWWindow = glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr);
		WindowSystemWindow->API = API;

		glfwDefaultWindowHints();
		return WindowSystemWindow;
	}

	FEWindowSystemWindowInterface* FEWindowSystemEmscripten::OpenFullscreenWindow(MonitorInfo* Monitor, GraphicsAPI API)
	{
		if (Monitor == nullptr || Monitor->Monitor == nullptr || Monitor->VideoMode == nullptr)
			return nullptr;

		if (API == GraphicsAPI::WebGPU)
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		glfwWindowHint(GLFW_RED_BITS,      Monitor->VideoMode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS,    Monitor->VideoMode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS,     Monitor->VideoMode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE,  Monitor->VideoMode->refreshRate);
		glfwWindowHint(GLFW_DECORATED,     GLFW_FALSE);

		FEWindowSystemWindowEmscripten* WindowSystemWindow = new FEWindowSystemWindowEmscripten();
		WindowSystemWindow->GLFWWindow = glfwCreateWindow(
			Monitor->VideoMode->width,
			Monitor->VideoMode->height,
			"",
			Monitor->Monitor,
			nullptr);
		WindowSystemWindow->API = API;

		glfwSetWindowMonitor(
			WindowSystemWindow->GLFWWindow,
			Monitor->Monitor,
			0, 0,
			Monitor->VideoMode->width,
			Monitor->VideoMode->height,
			Monitor->VideoMode->refreshRate);

		glfwDefaultWindowHints();
		return WindowSystemWindow;
	}

	bool FEWindowSystemEmscripten::SetClipboardText(std::string Text)
	{
		glfwSetClipboardString(nullptr, Text.c_str());
		return true;
	}

	std::string FEWindowSystemEmscripten::GetClipboardText()
	{
		const char* Clipboard = glfwGetClipboardString(nullptr);
		return Clipboard ? std::string(Clipboard) : std::string();
	}

	std::vector<MonitorInfo> FEWindowSystemEmscripten::GetMonitors()
	{
		std::vector<MonitorInfo> Result;

		int MonitorCount = 0;
		GLFWmonitor** Monitors = glfwGetMonitors(&MonitorCount);

		if (Monitors == nullptr || MonitorCount == 0)
			return Result;

		for (int i = 0; i < MonitorCount; i++)
		{
			if (Monitors[i] == nullptr)
				continue;

			MonitorInfo Info;
			Info.Monitor = Monitors[i];
			Info.VideoMode = glfwGetVideoMode(Monitors[i]);
			Info.Name = glfwGetMonitorName(Monitors[i]);
			glfwGetMonitorPos(Monitors[i], &Info.VirtualX, &Info.VirtualY);

			Result.push_back(Info);
		}

		return Result;
	}

	MonitorInfo FEWindowSystemEmscripten::GetMonitorContainingWindow(FEWindowSystemWindowInterface* Window)
	{
		MonitorInfo BestMonitor;
		if (Window == nullptr)
			return BestMonitor;

		int WindowX = 0, WindowY = 0;
		int WindowWidth = 0, WindowHeight = 0;
		Window->GetPosition(&WindowX, &WindowY);
		Window->GetSize(&WindowWidth, &WindowHeight);

		int MonitorCount = 0;
		GLFWmonitor** Monitors = glfwGetMonitors(&MonitorCount);
		if (Monitors == nullptr || MonitorCount == 0)
			return BestMonitor;

		int BestArea = 0;
		for (int i = 0; i < MonitorCount; i++)
		{
			int MonitorX = 0, MonitorY = 0;
			int MonitorWidth = 0, MonitorHeight = 0;
			glfwGetMonitorWorkarea(Monitors[i], &MonitorX, &MonitorY, &MonitorWidth, &MonitorHeight);

			int MinX = std::max(WindowX, MonitorX);
			int MinY = std::max(WindowY, MonitorY);
			int MaxX = std::min(WindowX + WindowWidth, MonitorX + MonitorWidth);
			int MaxY = std::min(WindowY + WindowHeight, MonitorY + MonitorHeight);

			int Area = std::max(0, MaxX - MinX) * std::max(0, MaxY - MinY);

			if (Area > BestArea)
			{
				BestArea = Area;
				BestMonitor.Monitor = Monitors[i];
				BestMonitor.VideoMode = glfwGetVideoMode(Monitors[i]);
			}
		}

		return BestMonitor;
	}

	void FEWindowSystemEmscripten::SetMonitorCallback(std::function<void(void* NativeMonitor, int Event)> Callback)
	{
		RegisteredMonitorCallback = std::move(Callback);
		glfwSetMonitorCallback(RegisteredMonitorCallback ? MonitorCallbackBridge : nullptr);
	}
}