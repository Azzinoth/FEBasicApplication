#pragma once

#include "../../../FEPlatform.h"
#include "../FEDeviceInterface.h"
#include "GL/glew.h"
#if FE_PLATFORM(WINDOWS)
	#include "GL/wglew.h"
#endif
#include "imgui/imgui_impl_opengl3.h"
#if FE_PLATFORM(WINDOWS)
	#include <GL/GL.h>
#endif

namespace FocalEngine
{
	class FEDeviceOpenGL : public FEDeviceInterface
	{
	public:
		~FEDeviceOpenGL();

		GraphicsAPI GetGraphicsAPI() const override;

		bool Initialize(FEWindowSystemWindowInterface* PrimaryWindow);
		void Shutdown();

		FEDeviceSurfaceInterface* CreateSurface(FEWindowSystemWindowInterface* Window);
		void* GetNativeDevice();
	};
}