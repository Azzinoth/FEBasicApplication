#pragma once

#include "../FEDeviceSurfaceInterface.h"

namespace FocalEngine
{
	class FEDeviceOpenGL;

	class FEDeviceSurfaceOpenGL : public FEDeviceSurfaceInterface
	{
		friend class FEDeviceOpenGL;
		FEDeviceOpenGL* OwnerDevice = nullptr;

		float ClearR = 0.0f;
		float ClearG = 0.0f;
		float ClearB = 0.0f;
		float ClearA = 1.0f;
	public:
		~FEDeviceSurfaceOpenGL() override;

		void Resize(int Width, int Height) override;

		void SetClearColor(float R, float G, float B, float A) override;

		void BeginFrame() override;
		void EndFrame() override;

		void ImGuiInit() override;
		void ImGuiShutdown() override;
		void ImGuiNewFrame() override;
		void ImGuiRenderDrawData() override;
	};
}
