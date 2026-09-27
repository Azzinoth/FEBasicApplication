#include "FEWindowSystemGLFW.h"
#include "../../../FEBasicApplication.h"
#include <algorithm>

namespace FocalEngine
{
	// Static bridge so GLFW's C-style monitor callback can forward into our std::function.
	// glfwSetMonitorCallback takes a plain function pointer, so we cannot pass a capturing
	// lambda directly. Store the registered callback and route through a non-capturing stub.
	static std::function<void(void* NativeMonitor, int Event)> RegisteredMonitorCallback;
	static void MonitorCallbackBridge(GLFWmonitor* Monitor, int Event)
	{
		if (RegisteredMonitorCallback)
			RegisteredMonitorCallback(static_cast<void*>(Monitor), Event);
	}

	FEWindowSystemInterface* CreateWindowSystem()
	{
		return new FEWindowSystemGLFW();
	}

	FEWindowSystemGLFW::~FEWindowSystemGLFW() {}

	bool FEWindowSystemGLFW::Initialize()
	{
		glfwInit();
		return true;
	}

	void FEWindowSystemGLFW::Shutdown()
	{
		glfwTerminate();
	}

	void FEWindowSystemGLFW::PollEvents()
	{
		glfwPollEvents();
	}

	void FEWindowSystemGLFW::RunMainLoop(std::function<void()> Tick)
	{
		while (APPLICATION.IsNotTerminated())
			Tick();
	}

	double FEWindowSystemGLFW::GetTime()
	{
		return glfwGetTime();
	}

	FEWindowSystemWindowInterface* FEWindowSystemGLFW::OpenWindow(int Width, int Height, std::string Title, GraphicsAPI API)
	{
		if (API == GraphicsAPI::WebGPU)
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		FEWindowSystemWindowGLFW* WindowSystemWindow = new FEWindowSystemWindowGLFW();
		WindowSystemWindow->GLFWWindow = glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr);
		WindowSystemWindow->API = API;

		glfwDefaultWindowHints();
		return WindowSystemWindow;
	}

	FEWindowSystemWindowInterface* FEWindowSystemGLFW::OpenFullscreenWindow(MonitorInfo* Monitor, GraphicsAPI API)
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

		FEWindowSystemWindowGLFW* WindowSystemWindow = new FEWindowSystemWindowGLFW();
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

	bool FEWindowSystemGLFW::SetClipboardText(std::string Text)
	{
		glfwSetClipboardString(nullptr, Text.c_str());
		return true;
	}

	std::string FEWindowSystemGLFW::GetClipboardText()
	{
		const char* Clipboard = glfwGetClipboardString(nullptr);
		return Clipboard ? std::string(Clipboard) : std::string();
	}

	std::vector<MonitorInfo> FEWindowSystemGLFW::GetMonitors()
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

	MonitorInfo FEWindowSystemGLFW::GetMonitorContainingWindow(FEWindowSystemWindowInterface* Window)
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

			int MinX = (std::max)(WindowX, MonitorX);
			int MinY = (std::max)(WindowY, MonitorY);
			int MaxX = (std::min)(WindowX + WindowWidth, MonitorX + MonitorWidth);
			int MaxY = (std::min)(WindowY + WindowHeight, MonitorY + MonitorHeight);

			int Area = (std::max)(0, MaxX - MinX) * (std::max)(0, MaxY - MinY);

			if (Area > BestArea)
			{
				BestArea = Area;
				BestMonitor.Monitor = Monitors[i];
				BestMonitor.VideoMode = glfwGetVideoMode(Monitors[i]);
			}
		}

		return BestMonitor;
	}

	void FEWindowSystemGLFW::SetMonitorCallback(std::function<void(void* NativeMonitor, int Event)> Callback)
	{
		RegisteredMonitorCallback = std::move(Callback);
		glfwSetMonitorCallback(RegisteredMonitorCallback ? MonitorCallbackBridge : nullptr);
	}
}