#pragma once

#include "../FEWindowSystemInterface.h"
#include "FEWindowSystemWindowGLFW.h"

namespace FocalEngine
{
	class FEWindowSystemGLFW : public FEWindowSystemInterface
	{
	public:
		~FEWindowSystemGLFW() override;

		bool Initialize() override;
		void Shutdown() override;
		void PollEvents() override;
		void RunMainLoop(std::function<void()> Tick) override;

		double GetTime() override;

		FEWindowSystemWindowInterface* OpenWindow(int Width, int Height, std::string Title, GraphicsAPI API) override;
		FEWindowSystemWindowInterface* OpenFullscreenWindow(MonitorInfo* Monitor, GraphicsAPI API) override;

		std::vector<MonitorInfo> GetMonitors() override;
		MonitorInfo GetMonitorContainingWindow(FEWindowSystemWindowInterface* Window) override;

		void SetMonitorCallback(std::function<void(void* NativeMonitor, int Event)> Callback) override;

		bool SetClipboardText(std::string Text) override;
		std::string GetClipboardText() override;
	};
}
