#include "FEDeviceWebGPU.h"
#include "FEDeviceSurfaceWebGPU.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Windows.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
		{
#ifdef __EMSCRIPTEN__
			// Yield to the browser event loop so the JS promise can resolve.
			emscripten_sleep(0);
#else
			Instance.ProcessEvents();
#endif
		}

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
		{
#ifdef __EMSCRIPTEN__
			emscripten_sleep(0);
#else
			Instance.ProcessEvents();
#endif
		}

		return Device != nullptr;
	}

	void FEDeviceWebGPU::Shutdown()
	{

	}

	FEDeviceSurfaceInterface* FEDeviceWebGPU::CreateSurface(FEPlatformWindowInterface* Window)
	{
		if (!Instance || !Adapter || !Device || Window == nullptr)
			return nullptr;

#ifdef _WIN32
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
#else
		// Web: WebGPU surface is created from an HTML canvas element by CSS selector.
		wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector CanvasDesc{};
		CanvasDesc.selector = "#canvas";

		wgpu::SurfaceDescriptor SurfaceDesc{};
		SurfaceDesc.nextInChain = &CanvasDesc;
#endif

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