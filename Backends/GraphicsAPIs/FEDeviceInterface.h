#pragma once

#include "../FEGraphicsAPI.h"
#include "../Platforms/FEPlatformWindowInterface.h"
#include "FEDeviceSurfaceInterface.h"

namespace FocalEngine
{
	class FEDeviceInterface
	{
	public:
		virtual ~FEDeviceInterface() = default;

		virtual GraphicsAPI GetGraphicsAPI() const { return GraphicsAPI::Unknown; }

		virtual bool Initialize(FEPlatformWindowInterface* PrimaryWindow) = 0;
		virtual void Shutdown() = 0;

		virtual FEDeviceSurfaceInterface* CreateSurface(FEPlatformWindowInterface* Window) = 0;

		virtual void* GetNativeDevice() = 0;
	};

	FEDeviceInterface* CreateDevice();
}