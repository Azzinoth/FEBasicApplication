#pragma once

#include "../FEGraphicsAPI.h"
#include "../WindowSystems/FEWindowSystemWindowInterface.h"
#include "FEDeviceSurfaceInterface.h"

namespace FocalEngine
{
	class FEDeviceInterface
	{
	public:
		virtual ~FEDeviceInterface() = default;

		virtual GraphicsAPI GetGraphicsAPI() const { return GraphicsAPI::Unknown; }

		virtual bool Initialize(FEWindowSystemWindowInterface* PrimaryWindow) = 0;
		virtual void Shutdown() = 0;

		virtual FEDeviceSurfaceInterface* CreateSurface(FEWindowSystemWindowInterface* Window) = 0;

		virtual void* GetNativeDevice() = 0;
	};

	FEDeviceInterface* CreateDevice();
}