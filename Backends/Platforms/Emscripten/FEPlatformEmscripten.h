#pragma once

#include "../FEPlatformInterface.h"
#include "FEPlatformWindowEmscripten.h"

namespace FocalEngine
{
	class FEPlatformEmscripten : public FEPlatformInterface
	{
	public:
		~FEPlatformEmscripten() override;

		bool Initialize() override;
		void Shutdown() override;
		void PollEvents() override;
		void RunMainLoop(std::function<void()> Tick) override;

		double GetTime() override;

		FEPlatformWindowInterface* OpenWindow(int Width, int Height, std::string Title, GraphicsAPI API) override;
		FEPlatformWindowInterface* OpenFullscreenWindow(MonitorInfo* Monitor, GraphicsAPI API) override;

		std::vector<MonitorInfo> GetMonitors() override;
		MonitorInfo GetMonitorContainingWindow(FEPlatformWindowInterface* Window) override;

		void SetMonitorCallback(std::function<void(void* NativeMonitor, int Event)> Callback) override;

		bool SetClipboardText(std::string Text) override;
		std::string GetClipboardText() override;
	};
}
