#pragma once

#include "../FEDeviceSurfaceInterface.h"
#include "imgui/imgui_impl_wgpu.h"
#include <webgpu/webgpu_cpp.h>

namespace FocalEngine
{
	class FEDeviceWebGPU;

	class FEDeviceSurfaceWebGPU : public FEDeviceSurfaceInterface
	{
		friend class FEDeviceWebGPU;
		FEDeviceWebGPU* OwnerDevice = nullptr;

		wgpu::Surface           Surface;
		wgpu::SurfaceTexture    CurrentTexture;
		wgpu::TextureView       CurrentView;
		wgpu::CommandEncoder    CurrentEncoder;
		wgpu::RenderPassEncoder CurrentPass;

		float ClearR = 0.0f;
		float ClearG = 0.0f;
		float ClearB = 0.0f;
		float ClearA = 1.0f;
	public:
		~FEDeviceSurfaceWebGPU() override;

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
