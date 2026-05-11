#include "FEPlatformEmscripten.h"
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

	FEPlatformInterface* CreatePlatform()
	{
		return new FEPlatformEmscripten();
	}

	FEPlatformEmscripten::~FEPlatformEmscripten() {}

	bool FEPlatformEmscripten::Initialize()
	{
		return glfwInit() == GLFW_TRUE;
	}

	void FEPlatformEmscripten::Shutdown()
	{
		glfwTerminate();
	}

	void FEPlatformEmscripten::PollEvents()
	{
		glfwPollEvents();
	}

	// emscripten_set_main_loop takes a non-capturing C function pointer, so we route
	// the std::function through a file-scope storage + thin dispatch stub.
	static std::function<void()> StoredMainLoopTick;
	static void DispatchMainLoopTick() { if (StoredMainLoopTick) StoredMainLoopTick(); }

	void FEPlatformEmscripten::RunMainLoop(std::function<void()> Tick)
	{
		StoredMainLoopTick = std::move(Tick);
		// fps=0 -> use requestAnimationFrame, simulate_infinite_loop=1 -> never returns to caller.
		emscripten_set_main_loop(DispatchMainLoopTick, 0, 1);
	}

	double FEPlatformEmscripten::GetTime()
	{
		return glfwGetTime();
	}

	FEPlatformWindowInterface* FEPlatformEmscripten::OpenWindow(int Width, int Height, std::string Title, GraphicsAPI API)
	{
		if (API == GraphicsAPI::WebGPU)
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		FEPlatformWindowEmscripten* PlatformWindow = new FEPlatformWindowEmscripten();
		PlatformWindow->GLFWWindow = glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr);
		PlatformWindow->API = API;

		glfwDefaultWindowHints();
		return PlatformWindow;
	}

	FEPlatformWindowInterface* FEPlatformEmscripten::OpenFullscreenWindow(MonitorInfo* Monitor, GraphicsAPI API)
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

		FEPlatformWindowEmscripten* PlatformWindow = new FEPlatformWindowEmscripten();
		PlatformWindow->GLFWWindow = glfwCreateWindow(
			Monitor->VideoMode->width,
			Monitor->VideoMode->height,
			"",
			Monitor->Monitor,
			nullptr);
		PlatformWindow->API = API;

		glfwSetWindowMonitor(
			PlatformWindow->GLFWWindow,
			Monitor->Monitor,
			0, 0,
			Monitor->VideoMode->width,
			Monitor->VideoMode->height,
			Monitor->VideoMode->refreshRate);

		glfwDefaultWindowHints();
		return PlatformWindow;
	}

	bool FEPlatformEmscripten::SetClipboardText(std::string Text)
	{
		glfwSetClipboardString(nullptr, Text.c_str());
		return true;
	}

	std::string FEPlatformEmscripten::GetClipboardText()
	{
		const char* Clipboard = glfwGetClipboardString(nullptr);
		return Clipboard ? std::string(Clipboard) : std::string();
	}

	std::vector<MonitorInfo> FEPlatformEmscripten::GetMonitors()
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

	MonitorInfo FEPlatformEmscripten::GetMonitorContainingWindow(FEPlatformWindowInterface* Window)
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

	void FEPlatformEmscripten::SetMonitorCallback(std::function<void(void* NativeMonitor, int Event)> Callback)
	{
		RegisteredMonitorCallback = std::move(Callback);
		glfwSetMonitorCallback(RegisteredMonitorCallback ? MonitorCallbackBridge : nullptr);
	}
}