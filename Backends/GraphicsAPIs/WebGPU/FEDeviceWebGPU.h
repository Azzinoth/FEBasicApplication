#pragma once

#include "../../FEDeviceInterface.h"
#include <webgpu/webgpu_cpp.h>

namespace FocalEngine
{
	class FEDeviceWebGPU : public FEDeviceInterface
	{
		wgpu::Instance Instance;
		wgpu::Adapter  Adapter;
		wgpu::Device   Device;
	public:
		~FEDeviceWebGPU() override;

		GraphicsAPI GetGraphicsAPI() const override;

		bool Initialize(FEPlatformWindowInterface* PrimaryWindow) override;
		void Shutdown() override;

		FEDeviceSurfaceInterface* CreateSurface(FEPlatformWindowInterface* Window) override;
		void* GetNativeDevice() override;

		const wgpu::Device&  GetWGPUDevice()  const { return Device; }
		const wgpu::Adapter& GetWGPUAdapter() const { return Adapter; }
	};
}