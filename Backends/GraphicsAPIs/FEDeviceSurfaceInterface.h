#pragma once

#include "../Platforms/FEPlatformWindowInterface.h"

namespace FocalEngine
{
	class FEDeviceSurfaceInterface
	{
	public:
		virtual ~FEDeviceSurfaceInterface() = default;

		virtual void Resize(int Width, int Height) = 0;

		virtual void SetClearColor(float R, float G, float B, float A) = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void ImGuiInit() = 0;
		virtual void ImGuiShutdown() = 0;
		virtual void ImGuiNewFrame() = 0;
		virtual void ImGuiRenderDrawData() = 0;
	};
}
