#include "FEDeviceWebGPU.h"
#include "FEDeviceSurfaceWebGPU.h"

// FE_FIX_ME: Windows-only HWND extraction. When porting to Mac/Linux,
// branch on platform here or use the Dawn-shipped wgpu::glfw helper once available.
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Windows.h>

namespace FocalEngine
{
	FEDeviceInterface* CreateDevice()
	{
		return new FEDeviceWebGPU();
	}

	GraphicsAPI FEDeviceWebGPU::GetGraphicsAPI() const
	{
		return GraphicsAPI::WebGPU;
	}

	FEDeviceWebGPU::~FEDeviceWebGPU()
	{

	}

	bool FEDeviceWebGPU::Initialize(FEPlatformWindowInterface* PrimaryWindow)
	{
		// 1. Create instance
		wgpu::InstanceDescriptor InstanceDesc{};
		Instance = wgpu::CreateInstance(&InstanceDesc);
		if (!Instance)
			return false;

		// 2. Request adapter
		wgpu::RequestAdapterOptions AdapterOptions{};
		AdapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;

		Instance.RequestAdapter(
			&AdapterOptions,
			wgpu::CallbackMode::AllowSpontaneous,
			[this](wgpu::RequestAdapterStatus Status, wgpu::Adapter Result, const char*) {
				if (Status == wgpu::RequestAdapterStatus::Success)
					Adapter = std::move(Result);
		});

		while (!Adapter)
			Instance.ProcessEvents();

		if (!Adapter)
			return false;

		// 3. Request device
		wgpu::DeviceDescriptor DeviceDesc{};
		Adapter.RequestDevice(
			&DeviceDesc,
			wgpu::CallbackMode::AllowSpontaneous,
			[this](wgpu::RequestDeviceStatus Status, wgpu::Device Result, const char*) {
				if (Status == wgpu::RequestDeviceStatus::Success)
					Device = std::move(Result);
		});

		while (!Device)
			Instance.ProcessEvents();

		return Device != nullptr;
	}

	void FEDeviceWebGPU::Shutdown()
	{

	}

	FEDeviceSurfaceInterface* FEDeviceWebGPU::CreateSurface(FEPlatformWindowInterface* Window)
	{
		if (!Instance || !Adapter || !Device || Window == nullptr)
			return nullptr;

		GLFWwindow* GlfwWindow = static_cast<GLFWwindow*>(Window->GetNativeHandle());
		if (GlfwWindow == nullptr)
			return nullptr;

		// Build surface descriptor from the GLFW window's HWND (Windows-only path)
		HWND Hwnd = glfwGetWin32Window(GlfwWindow);
		HINSTANCE Hinstance = GetModuleHandle(nullptr);

		wgpu::SurfaceSourceWindowsHWND WindowDesc{};
		WindowDesc.hwnd = Hwnd;
		WindowDesc.hinstance = Hinstance;

		wgpu::SurfaceDescriptor SurfaceDesc{};
		SurfaceDesc.nextInChain = &WindowDesc;

		FEDeviceSurfaceWebGPU* NewSurface = new FEDeviceSurfaceWebGPU();
		NewSurface->OwnerDevice = this;
		NewSurface->Surface = Instance.CreateSurface(&SurfaceDesc);

		// Configure surface
		wgpu::SurfaceCapabilities Capabilities;
		NewSurface->Surface.GetCapabilities(Adapter, &Capabilities);

		int Width = 0;
		int Height = 0;
		Window->GetSize(&Width, &Height);

		wgpu::SurfaceConfiguration SurfaceConfig{};
		SurfaceConfig.device = Device;
		SurfaceConfig.format = Capabilities.formats[0];
		SurfaceConfig.width = static_cast<uint32_t>(Width);
		SurfaceConfig.height = static_cast<uint32_t>(Height);
		SurfaceConfig.presentMode = wgpu::PresentMode::Fifo;
		NewSurface->Surface.Configure(&SurfaceConfig);

		return NewSurface;
	}

	void* FEDeviceWebGPU::GetNativeDevice()
	{
		return Device.Get();
	}
}