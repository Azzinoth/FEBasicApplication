#pragma once

#include "../FEGraphicsAPI.h"
#include "FEPlatformWindowInterface.h"
#include "../FEMonitorInfo.h"
#include <functional>
#include <string>
#include <vector>

namespace FocalEngine
{
	class FEPlatformInterface
	{
	public:
		virtual ~FEPlatformInterface() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void PollEvents() = 0;
		virtual void RunMainLoop(std::function<void()> Tick) = 0;

		virtual double GetTime() = 0;

		virtual FEPlatformWindowInterface* OpenWindow(int Width, int Height, std::string Title, GraphicsAPI API) = 0;
		virtual FEPlatformWindowInterface* OpenFullscreenWindow(MonitorInfo* Monitor, GraphicsAPI API) = 0;

		virtual std::vector<MonitorInfo> GetMonitors() = 0;
		virtual MonitorInfo GetMonitorContainingWindow(FEPlatformWindowInterface* Window) = 0;

		// Register a process-global callback for monitor connect/disconnect events.
		// NativeMonitor is the opaque native handle (GLFWmonitor* for the GLFW backend).
		// Event matches GLFW_CONNECTED / GLFW_DISCONNECTED values.
		virtual void SetMonitorCallback(std::function<void(void* NativeMonitor, int Event)> Callback) = 0;

		virtual bool SetClipboardText(std::string Text) = 0;
		virtual std::string GetClipboardText() = 0;
	};

	FEPlatformInterface* CreatePlatform();
}
