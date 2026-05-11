#include "FEDeviceOpenGL.h"
#include "FEDeviceSurfaceOpenGL.h"

namespace FocalEngine
{
	FEDeviceInterface* CreateDevice()
	{
		return new FEDeviceOpenGL();
	}

	GraphicsAPI FEDeviceOpenGL::GetGraphicsAPI() const
	{
		return GraphicsAPI::OpenGL;
	}

	FEDeviceOpenGL::~FEDeviceOpenGL()
	{

	}

	bool FEDeviceOpenGL::Initialize(FEPlatformWindowInterface* PrimaryWindow)
	{
		PrimaryWindow->MakeContextCurrent();
		return glewInit() == GLEW_OK;
	}

	void FEDeviceOpenGL::Shutdown()
	{

	}

	FEDeviceSurfaceInterface* FEDeviceOpenGL::CreateSurface(FEPlatformWindowInterface* Window)
	{
		FEDeviceSurfaceOpenGL* NewSurface = new FEDeviceSurfaceOpenGL();
		NewSurface->OwnerDevice = this;
		return NewSurface;
	}

	void* FEDeviceOpenGL::GetNativeDevice()
	{
		return nullptr;
	}
}