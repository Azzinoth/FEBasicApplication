#include "FEPlatformGLFW.h"

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

	FEPlatformInterface* CreatePlatform()
	{
		return new FEPlatformGLFW();
	}

	FEPlatformGLFW::~FEPlatformGLFW() {}

	bool FEPlatformGLFW::Initialize()
	{
		glfwInit();
		return true;
	}

	void FEPlatformGLFW::Shutdown()
	{
		glfwTerminate();
	}

	void FEPlatformGLFW::PollEvents()
	{
		glfwPollEvents();
	}

	void FEPlatformGLFW::RunMainLoop(std::function<void()> Tick) {}

	double FEPlatformGLFW::GetTime()
	{
		return glfwGetTime();
	}

	FEPlatformWindowInterface* FEPlatformGLFW::OpenWindow(int Width, int Height, std::string Title, GraphicsAPI API)
	{
		if (API == GraphicsAPI::WebGPU)
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		FEPlatformWindowGLFW* PlatformWindow = new FEPlatformWindowGLFW();
		PlatformWindow->GLFWWindow = glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr);
		PlatformWindow->API = API;

		glfwDefaultWindowHints();
		return PlatformWindow;
	}

	FEPlatformWindowInterface* FEPlatformGLFW::OpenFullscreenWindow(MonitorInfo* Monitor, GraphicsAPI API)
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

		FEPlatformWindowGLFW* PlatformWindow = new FEPlatformWindowGLFW();
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

	bool FEPlatformGLFW::SetClipboardText(std::string Text)
	{
		glfwSetClipboardString(nullptr, Text.c_str());
		return true;
	}

	std::string FEPlatformGLFW::GetClipboardText()
	{
		const char* Clipboard = glfwGetClipboardString(nullptr);
		return Clipboard ? std::string(Clipboard) : std::string();
	}

	std::vector<MonitorInfo> FEPlatformGLFW::GetMonitors()
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

	MonitorInfo FEPlatformGLFW::GetMonitorContainingWindow(FEPlatformWindowInterface* Window)
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

			int MinX = max(WindowX, MonitorX);
			int MinY = max(WindowY, MonitorY);
			int MaxX = min(WindowX + WindowWidth, MonitorX + MonitorWidth);
			int MaxY = min(WindowY + WindowHeight, MonitorY + MonitorHeight);

			int Area = max(0, MaxX - MinX) * max(0, MaxY - MinY);

			if (Area > BestArea)
			{
				BestArea = Area;
				BestMonitor.Monitor = Monitors[i];
				BestMonitor.VideoMode = glfwGetVideoMode(Monitors[i]);
			}
		}

		return BestMonitor;
	}

	void FEPlatformGLFW::SetMonitorCallback(std::function<void(void* NativeMonitor, int Event)> Callback)
	{
		RegisteredMonitorCallback = std::move(Callback);
		glfwSetMonitorCallback(RegisteredMonitorCallback ? MonitorCallbackBridge : nullptr);
	}
}