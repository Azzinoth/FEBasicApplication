#pragma once

#include "../FEUniqueID.h"
#include "../FEOS.h"
#include <cstdint>
#include <vector>

namespace FocalEngine
{
	struct FEOSVersion
	{
		OS OperatingSystem = OS::Unknown;
		// Windows 11 reports 10.0.
		int Major = 0;
		int Minor = 0;
		int Patch = 0;
		// Windows build with update revision (for example 26200.9711), macOS build ID or Linux kernel release.
		std::string Build;
		// For example "Windows 11 Home" or Linux distribution "Ubuntu 24.04.1 LTS". Empty on macOS and web.
		std::string Name;
	};

	struct FECPUInfo
	{
		// Empty on web, browsers do not expose it.
		std::string Name;
		// 0 on web, browsers do not expose it.
		int PhysicalCoreCount = 0;
		int LogicalCoreCount = 0;
	};

	struct FERAMInfo
	{
		// Memory usable by the OS, a bit less than installed. 0 on web.
		uint64_t TotalBytes = 0;
		// 0 on web.
		uint64_t AvailableBytes = 0;
	};

	struct FEGPUInfo
	{
		std::string Name;
		// PCI IDs, for example vendor 0x10DE is NVIDIA, 0x1002 is AMD, 0x8086 is Intel. 0 when unknown.
		uint32_t VendorID = 0;
		uint32_t DeviceID = 0;
		// 0 when unknown or when the GPU uses system memory (Apple silicon), integrated GPUs on Windows report only their small reserved part.
		uint64_t DedicatedMemoryBytes = 0;
		// Linux kernel driver, for example amdgpu or nvidia.
		std::string DriverName;
		// Version as the vendor names it, for example NVIDIA 610.88 or AMD Software 25.10.1. Empty on macOS and web, macOS GPU drivers are part of the OS.
		std::string DriverVersion;
		// Windows only, the format Device Manager shows, for example 32.0.16.1088.
		std::string WindowsDriverVersion;
	};

	class FEBASICAPPLICATION_API FESystemInfo
	{
	public:
		SINGLETON_PUBLIC_PART(FESystemInfo)

		FEOSVersion GetOSVersion() const;
		// For example "Windows 11 Home build 26200.9711".
		std::string GetOSVersionString() const;

		FECPUInfo GetCPUInfo() const;
		// For example "AMD Ryzen 9 7950X 16-Core Processor (32 logical cores)".
		std::string GetCPUInfoString() const;

		// Queried on every call, because available memory changes.
		FERAMInfo GetRAMInfo() const;
		// For example "63.2 GB (41.7 GB available)".
		std::string GetRAMInfoString() const;

		// All GPUs in the system, web gives only the one the browser uses. Queried on every call.
		std::vector<FEGPUInfo> GetGPUInfo() const;
		// One line per GPU, for example "NVIDIA GeForce RTX 4090 (23.6 GB, driver 610.88)".
		std::string GetGPUInfoString() const;
	private:
		SINGLETON_PRIVATE_PART(FESystemInfo)

		FEOSVersion OSVersion;
		FECPUInfo CPUInfo;
	};

#ifdef FEBASICAPPLICATION_SHARED
	extern "C" __declspec(dllexport) void* GetSystemInformation();
	#define SYSTEM_INFORMATION (*static_cast<FESystemInfo*>(GetSystemInformation()))
#else
	#define SYSTEM_INFORMATION FESystemInfo::GetInstance()
#endif
}