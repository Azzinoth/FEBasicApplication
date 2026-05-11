#include "FEDeviceSurfaceWebGPU.h"
#include "FEDeviceWebGPU.h"

namespace FocalEngine
{
	FEDeviceSurfaceWebGPU::~FEDeviceSurfaceWebGPU() {}

	void FEDeviceSurfaceWebGPU::ImGuiInit()
	{
		ImGui_ImplWGPU_InitInfo InitInfo{};
		InitInfo.Device = OwnerDevice->GetWGPUDevice().Get();
		InitInfo.RenderTargetFormat = WGPUTextureFormat_BGRA8Unorm;
		InitInfo.DepthStencilFormat = WGPUTextureFormat_Undefined;
		InitInfo.NumFramesInFlight = 3;
		ImGui_ImplWGPU_Init(&InitInfo);
	}

	void FEDeviceSurfaceWebGPU::Resize(int Width, int Height)
	{
		if (Width == 0 || Height == 0 || OwnerDevice == nullptr)
			return;

		wgpu::SurfaceCapabilities Capabilities;
		Surface.GetCapabilities(OwnerDevice->GetWGPUAdapter(), &Capabilities);

		wgpu::SurfaceConfiguration SurfaceConfig{};
		SurfaceConfig.device = OwnerDevice->GetWGPUDevice();
		SurfaceConfig.format = Capabilities.formats[0];
		SurfaceConfig.width = static_cast<uint32_t>(Width);
		SurfaceConfig.height = static_cast<uint32_t>(Height);
		SurfaceConfig.presentMode = wgpu::PresentMode::Fifo;
		Surface.Configure(&SurfaceConfig);
	}

	void FEDeviceSurfaceWebGPU::SetClearColor(float R, float G, float B, float A)
	{
		ClearR = R;
		ClearG = G;
		ClearB = B;
		ClearA = A;
	}

	void FEDeviceSurfaceWebGPU::BeginFrame()
	{
		Surface.GetCurrentTexture(&CurrentTexture);

		wgpu::TextureViewDescriptor ViewDesc{};
		CurrentView = CurrentTexture.texture.CreateView(&ViewDesc);

		wgpu::RenderPassColorAttachment ColorAttachment{};
		ColorAttachment.view = CurrentView;
		ColorAttachment.loadOp = wgpu::LoadOp::Clear;
		ColorAttachment.storeOp = wgpu::StoreOp::Store;
		ColorAttachment.clearValue = { ClearR, ClearG, ClearB, ClearA };

		wgpu::RenderPassDescriptor RenderPassDesc{};
		RenderPassDesc.colorAttachmentCount = 1;
		RenderPassDesc.colorAttachments = &ColorAttachment;

		CurrentEncoder = OwnerDevice->GetWGPUDevice().CreateCommandEncoder();
		CurrentPass = CurrentEncoder.BeginRenderPass(&RenderPassDesc);
	}

	void FEDeviceSurfaceWebGPU::EndFrame()
	{
		CurrentPass.End();
		wgpu::CommandBuffer Commands = CurrentEncoder.Finish();
		OwnerDevice->GetWGPUDevice().GetQueue().Submit(1, &Commands);

#ifndef __EMSCRIPTEN__
		// Browser auto-presents the canvas after each requestAnimationFrame tick;
		// wgpuSurfacePresent is intentionally not implemented in emdawnwebgpu.
		Surface.Present();
#endif
	}

	void FEDeviceSurfaceWebGPU::ImGuiShutdown()
	{
		ImGui_ImplWGPU_Shutdown();
	}

	void FEDeviceSurfaceWebGPU::ImGuiNewFrame()
	{
		ImGui_ImplWGPU_NewFrame();
	}

	void FEDeviceSurfaceWebGPU::ImGuiRenderDrawData()
	{
		ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), CurrentPass.Get());
	}
}