#pragma once

#include "../FEDeviceInterface.h"
#include "GL/glew.h"
#include "GL/wglew.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GL/GL.h>

namespace FocalEngine
{
	class FEDeviceOpenGL : public FEDeviceInterface
	{
	public:
		~FEDeviceOpenGL();

		GraphicsAPI GetGraphicsAPI() const override;

		bool Initialize(FEPlatformWindowInterface* PrimaryWindow);
		void Shutdown();

		FEDeviceSurfaceInterface* CreateSurface(FEPlatformWindowInterface* Window);
		void* GetNativeDevice();
	};
}