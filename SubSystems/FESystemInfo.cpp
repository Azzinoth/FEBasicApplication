#include "FESystemInfo.h"
#include "../FEPlatform.h"
#include <iomanip>
#include <sstream>
#include <thread>

#if FE_PLATFORM(WINDOWS)
#include <windows.h>
#include <winternl.h>
#include <d3dkmthk.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "gdi32.lib")
#elif FE_PLATFORM(MACOS)
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstdio>
#include <cstring>
#elif FE_PLATFORM(LINUX)
#include "../FEFileSystem.h"
#include <sys/utsname.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#elif FE_PLATFORM(EMSCRIPTEN)
#include <emscripten.h>

// Browsers can lower this value for privacy.
EM_JS(int, FEQueryBrowserLogicalCoreCount, (), {
	if (typeof navigator === "undefined" || !navigator.hardwareConcurrency) return 0;
	return navigator.hardwareConcurrency;
});

EM_JS_DEPS(FEQueryBrowserGPURenderer, "$stringToUTF8");

// WebGL is used because WebGPU adapter information is asynchronous and usually has no device name.
EM_JS(void, FEQueryBrowserGPURenderer, (char* Buffer, int BufferSize), {
	if (typeof document === "undefined") return;
	const Context = document.createElement("canvas").getContext("webgl");
	if (!Context) return;
	const DebugInfo = Context.getExtension("WEBGL_debug_renderer_info");
	const Renderer = Context.getParameter(DebugInfo ? DebugInfo.UNMASKED_RENDERER_WEBGL : Context.RENDERER);
	const LoseContext = Context.getExtension("WEBGL_lose_context");
	if (LoseContext) LoseContext.loseContext();
	if (Renderer) stringToUTF8(Renderer, Buffer, BufferSize);
});
#endif

using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetSystemInformation()
{
	return FESystemInfo::GetInstancePointer();
}
#endif

namespace
{
	std::string TrimWhitespace(const std::string& Text)
	{
		const size_t First = Text.find_first_not_of(" \t\r\n");
		if (First == std::string::npos)
			return "";

		const size_t Last = Text.find_last_not_of(" \t\r\n");
		return Text.substr(First, Last - First + 1);
	}

	std::string BytesToGigabytesString(const uint64_t Bytes)
	{
		std::ostringstream Stream;
		Stream << std::fixed << std::setprecision(1) << static_cast<double>(Bytes) / (1024.0 * 1024.0 * 1024.0) << " GB";
		return Stream.str();
	}

	std::string GPUInfoToString(const FEGPUInfo& GPU)
	{
		std::string Result = GPU.Name.empty() ? "Unknown GPU" : GPU.Name;

		std::string Details;
		if (GPU.DedicatedMemoryBytes > 0)
			Details = BytesToGigabytesString(GPU.DedicatedMemoryBytes);

		std::string Driver = GPU.DriverName;
		if (!GPU.DriverVersion.empty())
			Driver += (Driver.empty() ? "" : " ") + GPU.DriverVersion;

		if (!Driver.empty())
			Details += (Details.empty() ? "driver " : ", driver ") + Driver;

		if (!Details.empty())
			Result += " (" + Details + ")";

		return Result;
	}

#if FE_PLATFORM(WINDOWS)
	// GetVersionEx reports 6.2 to applications without a compatibility manifest, RtlGetVersion always reports the real version.
	void QueryWindowsVersion(FEOSVersion& Version)
	{
		typedef LONG(WINAPI* RtlGetVersionFunction)(PRTL_OSVERSIONINFOW);

		HMODULE NtDll = GetModuleHandleW(L"ntdll.dll");
		if (NtDll == nullptr)
			return;

		RtlGetVersionFunction RtlGetVersion = reinterpret_cast<RtlGetVersionFunction>(GetProcAddress(NtDll, "RtlGetVersion"));
		if (RtlGetVersion == nullptr)
			return;

		RTL_OSVERSIONINFOW Info = {};
		Info.dwOSVersionInfoSize = sizeof(Info);
		if (RtlGetVersion(&Info) != 0)
			return;

		Version.Major = static_cast<int>(Info.dwMajorVersion);
		Version.Minor = static_cast<int>(Info.dwMinorVersion);
		Version.Build = std::to_string(Info.dwBuildNumber);

		const char* CurrentVersionKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
		DWORD UpdateRevision = 0;
		DWORD UpdateRevisionSize = sizeof(UpdateRevision);
		if (RegGetValueA(HKEY_LOCAL_MACHINE, CurrentVersionKey, "UBR", RRF_RT_REG_DWORD, nullptr, &UpdateRevision, &UpdateRevisionSize) == ERROR_SUCCESS)
			Version.Build += "." + std::to_string(UpdateRevision);

		char ProductName[256] = {};
		DWORD ProductNameSize = sizeof(ProductName);
		if (RegGetValueA(HKEY_LOCAL_MACHINE, CurrentVersionKey, "ProductName", RRF_RT_REG_SZ, nullptr, ProductName, &ProductNameSize) != ERROR_SUCCESS)
			return;

		Version.Name = ProductName;
		// ProductName still says Windows 10 on Windows 11, whose first build is 22000.
		const std::string OldName = "Windows 10";
		if (Info.dwBuildNumber >= 22000 && Version.Name.compare(0, OldName.size(), OldName) == 0)
			Version.Name.replace(0, OldName.size(), "Windows 11");
	}

	void QueryWindowsCPU(FECPUInfo& CPU)
	{
		char Name[256] = {};
		DWORD NameSize = sizeof(Name);
		if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", RRF_RT_REG_SZ, nullptr, Name, &NameSize) == ERROR_SUCCESS)
			CPU.Name = Name;

		DWORD BufferSize = 0;
		GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &BufferSize);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<char> Buffer(BufferSize);
			if (GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(Buffer.data()), &BufferSize))
			{
				DWORD Offset = 0;
				while (Offset < BufferSize)
				{
					const PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(Buffer.data() + Offset);
					CPU.PhysicalCoreCount++;
					Offset += Entry->Size;
				}
			}
		}

		CPU.LogicalCoreCount = static_cast<int>(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
	}

	void QueryWindowsRAM(FERAMInfo& RAM)
	{
		MEMORYSTATUSEX Status = {};
		Status.dwLength = sizeof(Status);
		if (!GlobalMemoryStatusEx(&Status))
			return;

		RAM.TotalBytes = Status.ullTotalPhys;
		RAM.AvailableBytes = Status.ullAvailPhys;
	}

	std::string WideToUTF8(const wchar_t* Text)
	{
		const int Size = WideCharToMultiByte(CP_UTF8, 0, Text, -1, nullptr, 0, nullptr, nullptr);
		if (Size <= 1)
			return "";

		std::string Result(static_cast<size_t>(Size - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, Text, -1, Result.data(), Size, nullptr, nullptr);
		return Result;
	}

	// NVIDIA version is the last five digits of the last two parts, 32.0.16.1088 is 610.88.
	std::string WindowsToNVIDIADriverVersion(const uint64_t Version)
	{
		const uint64_t Digits = (((Version >> 16) & 0xFFFF) % 10) * 10000 + (Version & 0xFFFF);
		const uint64_t Minor = Digits % 100;
		return std::to_string(Digits / 100) + (Minor < 10 ? ".0" : ".") + std::to_string(Minor);
	}

	// Reads a string value from the adapter's driver registry key.
	std::string ReadAdapterRegistryString(const D3DKMT_HANDLE Adapter, const wchar_t* ValueName)
	{
		std::vector<BYTE> Buffer(sizeof(D3DDDI_QUERYREGISTRY_INFO) + 1024);
		D3DDDI_QUERYREGISTRY_INFO* Info = reinterpret_cast<D3DDDI_QUERYREGISTRY_INFO*>(Buffer.data());
		Info->QueryType = D3DDDI_QUERYREGISTRY_ADAPTERKEY;
		Info->ValueType = REG_SZ;
		wcsncpy_s(Info->ValueName, ValueName, _TRUNCATE);

		D3DKMT_QUERYADAPTERINFO Query = {};
		Query.hAdapter = Adapter;
		Query.Type = KMTQAITYPE_QUERYREGISTRY;
		Query.pPrivateDriverData = Info;
		Query.PrivateDriverDataSize = static_cast<UINT>(Buffer.size());
		if (D3DKMTQueryAdapterInfo(&Query) != 0 || Info->Status != D3DDDI_QUERYREGISTRY_STATUS_SUCCESS)
			return "";

		const size_t MaxLength = (Buffer.size() - static_cast<size_t>(reinterpret_cast<BYTE*>(Info->OutputString) - Buffer.data())) / sizeof(WCHAR);
		return WideToUTF8(std::wstring(Info->OutputString, wcsnlen(Info->OutputString, MaxLength)).c_str());
	}

	// Returns false for display-only adapters, such as virtual monitors, which DXGI lists with the name of the GPU that renders for them.
	bool QueryKernelAdapterInfo(const LUID& AdapterLUID, FEGPUInfo& GPU)
	{
		D3DKMT_OPENADAPTERFROMLUID OpenAdapter = {};
		OpenAdapter.AdapterLuid = AdapterLUID;
		if (D3DKMTOpenAdapterFromLuid(&OpenAdapter) != 0)
			return true;

		D3DKMT_ADAPTERTYPE AdapterType = {};
		D3DKMT_QUERYADAPTERINFO Query = {};
		Query.hAdapter = OpenAdapter.hAdapter;
		Query.Type = KMTQAITYPE_ADAPTERTYPE;
		Query.pPrivateDriverData = &AdapterType;
		Query.PrivateDriverDataSize = sizeof(AdapterType);
		const bool bCanRender = D3DKMTQueryAdapterInfo(&Query) != 0 || AdapterType.RenderSupported;

		// Version that AMD Software shows, for example 25.10.1.
		if (bCanRender && GPU.VendorID == 0x1002)
			GPU.DriverVersion = ReadAdapterRegistryString(OpenAdapter.hAdapter, L"RadeonSoftwareVersion");

		D3DKMT_CLOSEADAPTER CloseAdapter = {};
		CloseAdapter.hAdapter = OpenAdapter.hAdapter;
		D3DKMTCloseAdapter(&CloseAdapter);

		return bCanRender;
	}

	void QueryWindowsGPUs(std::vector<FEGPUInfo>& GPUs)
	{
		IDXGIFactory1* Factory = nullptr;
		if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&Factory))))
			return;

		IDXGIAdapter1* Adapter = nullptr;
		for (UINT Index = 0; Factory->EnumAdapters1(Index, &Adapter) != DXGI_ERROR_NOT_FOUND; Index++)
		{
			DXGI_ADAPTER_DESC1 Description = {};
			if (SUCCEEDED(Adapter->GetDesc1(&Description)) && !(Description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
			{
				FEGPUInfo GPU;
				GPU.Name = WideToUTF8(Description.Description);
				GPU.VendorID = Description.VendorId;
				GPU.DeviceID = Description.DeviceId;
				GPU.DedicatedMemoryBytes = Description.DedicatedVideoMemory;

				LARGE_INTEGER DriverVersion = {};
				if (SUCCEEDED(Adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &DriverVersion)))
				{
					const uint64_t Version = static_cast<uint64_t>(DriverVersion.QuadPart);
					GPU.WindowsDriverVersion = std::to_string((Version >> 48) & 0xFFFF) + "." + std::to_string((Version >> 32) & 0xFFFF) + "." +
											   std::to_string((Version >> 16) & 0xFFFF) + "." + std::to_string(Version & 0xFFFF);
					if (GPU.VendorID == 0x10DE)
						GPU.DriverVersion = WindowsToNVIDIADriverVersion(Version);
				}

				if (QueryKernelAdapterInfo(Description.AdapterLuid, GPU))
				{
					if (GPU.DriverVersion.empty())
						GPU.DriverVersion = GPU.WindowsDriverVersion;

					GPUs.push_back(GPU);
				}
			}

			Adapter->Release();
		}

		Factory->Release();
	}
#elif FE_PLATFORM(MACOS)
	std::string ReadSysctlString(const char* Name)
	{
		size_t Size = 0;
		if (sysctlbyname(Name, nullptr, &Size, nullptr, 0) != 0 || Size == 0)
			return "";

		std::string Result(Size, '\0');
		if (sysctlbyname(Name, Result.data(), &Size, nullptr, 0) != 0)
			return "";

		Result.resize(std::strlen(Result.c_str()));
		return Result;
	}

	void QueryMacOSVersion(FEOSVersion& Version)
	{
		const std::string ProductVersion = ReadSysctlString("kern.osproductversion");
		std::sscanf(ProductVersion.c_str(), "%d.%d.%d", &Version.Major, &Version.Minor, &Version.Patch);
		Version.Build = ReadSysctlString("kern.osversion");
	}

	int ReadSysctlInt(const char* Name)
	{
		int Value = 0;
		size_t Size = sizeof(Value);
		if (sysctlbyname(Name, &Value, &Size, nullptr, 0) != 0)
			return 0;

		return Value;
	}

	void QueryMacOSCPU(FECPUInfo& CPU)
	{
		CPU.Name = ReadSysctlString("machdep.cpu.brand_string");
		CPU.PhysicalCoreCount = ReadSysctlInt("hw.physicalcpu");
		CPU.LogicalCoreCount = ReadSysctlInt("hw.logicalcpu");
	}

	// Available memory is free plus inactive pages, the same as psutil.
	void QueryMacOSRAM(FERAMInfo& RAM)
	{
		uint64_t MemorySize = 0;
		size_t Size = sizeof(MemorySize);
		if (sysctlbyname("hw.memsize", &MemorySize, &Size, nullptr, 0) == 0)
			RAM.TotalBytes = MemorySize;

		const mach_port_t Host = mach_host_self();
		vm_size_t PageSize = 0;
		vm_statistics64_data_t Statistics = {};
		mach_msg_type_number_t Count = HOST_VM_INFO64_COUNT;
		if (host_page_size(Host, &PageSize) == KERN_SUCCESS &&
			host_statistics64(Host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&Statistics), &Count) == KERN_SUCCESS)
		{
			RAM.AvailableBytes = (static_cast<uint64_t>(Statistics.free_count) + Statistics.inactive_count) * PageSize;
		}

		mach_port_deallocate(mach_task_self(), Host);
	}

	// Searches the entry and then its parents, on Intel Macs most GPU properties are on the parent PCI device.
	CFTypeRef CopyGPUProperty(const io_registry_entry_t Entry, const CFStringRef Key)
	{
		return IORegistryEntrySearchCFProperty(Entry, kIOServicePlane, Key, kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);
	}

	// "model" is a string on Apple silicon and data on Intel Macs.
	std::string GPUPropertyToString(const CFTypeRef Value)
	{
		if (Value == nullptr)
			return "";

		std::string Result;
		if (CFGetTypeID(Value) == CFStringGetTypeID())
		{
			char Buffer[256] = {};
			if (CFStringGetCString(static_cast<CFStringRef>(Value), Buffer, sizeof(Buffer), kCFStringEncodingUTF8))
				Result = Buffer;
		}
		else if (CFGetTypeID(Value) == CFDataGetTypeID())
		{
			const CFDataRef Data = static_cast<CFDataRef>(Value);
			Result.assign(reinterpret_cast<const char*>(CFDataGetBytePtr(Data)), static_cast<size_t>(CFDataGetLength(Data)));
			Result.resize(std::strlen(Result.c_str()));
		}

		CFRelease(Value);
		return Result;
	}

	// PCI IDs are 4 bytes of data, other values are numbers.
	uint32_t GPUPropertyToUInt32(const CFTypeRef Value)
	{
		if (Value == nullptr)
			return 0;

		uint32_t Result = 0;
		if (CFGetTypeID(Value) == CFDataGetTypeID())
		{
			const CFDataRef Data = static_cast<CFDataRef>(Value);
			if (CFDataGetLength(Data) >= static_cast<CFIndex>(sizeof(Result)))
				std::memcpy(&Result, CFDataGetBytePtr(Data), sizeof(Result));
		}
		else if (CFGetTypeID(Value) == CFNumberGetTypeID())
		{
			CFNumberGetValue(static_cast<CFNumberRef>(Value), kCFNumberSInt32Type, &Result);
		}

		CFRelease(Value);
		return Result;
	}

	void QueryMacOSGPUs(std::vector<FEGPUInfo>& GPUs)
	{
		io_iterator_t Iterator = 0;
		if (IOServiceGetMatchingServices(MACH_PORT_NULL, IOServiceMatching("IOAccelerator"), &Iterator) != KERN_SUCCESS)
			return;

		io_registry_entry_t Entry = 0;
		while ((Entry = IOIteratorNext(Iterator)) != 0)
		{
			FEGPUInfo GPU;
			GPU.Name = GPUPropertyToString(CopyGPUProperty(Entry, CFSTR("model")));
			GPU.VendorID = GPUPropertyToUInt32(CopyGPUProperty(Entry, CFSTR("vendor-id")));
			GPU.DeviceID = GPUPropertyToUInt32(CopyGPUProperty(Entry, CFSTR("device-id")));
			GPU.DedicatedMemoryBytes = static_cast<uint64_t>(GPUPropertyToUInt32(CopyGPUProperty(Entry, CFSTR("VRAM,totalMB")))) * 1024 * 1024;
			GPUs.push_back(GPU);

			IOObjectRelease(Entry);
		}

		IOObjectRelease(Iterator);
	}
#elif FE_PLATFORM(LINUX)
	std::string ReadLinuxDistributionName()
	{
		const std::string Key = "PRETTY_NAME=";
		for (const char* Path : { "/etc/os-release", "/usr/lib/os-release" })
		{
			std::ifstream File(Path);
			std::string Line;
			while (std::getline(File, Line))
			{
				if (Line.compare(0, Key.size(), Key) != 0)
					continue;

				std::string Value = Line.substr(Key.size());
				if (Value.size() >= 2 && (Value.front() == '"' || Value.front() == '\'') && Value.back() == Value.front())
					Value = Value.substr(1, Value.size() - 2);

				return Value;
			}
		}

		return "";
	}

	void QueryLinuxVersion(FEOSVersion& Version)
	{
		utsname Info;
		if (uname(&Info) == 0)
		{
			Version.Build = Info.release;
			std::sscanf(Info.release, "%d.%d.%d", &Version.Major, &Version.Minor, &Version.Patch);
		}

		Version.Name = ReadLinuxDistributionName();
	}

	// Physical cores are the unique "physical id" and "core id" pairs.
	void QueryLinuxCPU(FECPUInfo& CPU)
	{
		std::ifstream File("/proc/cpuinfo");
		std::set<std::string> Cores;
		std::string PhysicalID;
		std::string Line;
		while (std::getline(File, Line))
		{
			const size_t Colon = Line.find(':');
			if (Colon == std::string::npos)
				continue;

			const std::string Key = TrimWhitespace(Line.substr(0, Colon));
			const std::string Value = TrimWhitespace(Line.substr(Colon + 1));
			if (Key == "processor")
				CPU.LogicalCoreCount++;
			else if (Key == "model name" && CPU.Name.empty())
				CPU.Name = Value;
			else if (Key == "physical id")
				PhysicalID = Value;
			else if (Key == "core id")
				Cores.insert(PhysicalID + ":" + Value);
		}

		CPU.PhysicalCoreCount = static_cast<int>(Cores.size());
	}

	void QueryLinuxRAM(FERAMInfo& RAM)
	{
		std::ifstream File("/proc/meminfo");
		std::string Line;
		while (std::getline(File, Line))
		{
			std::istringstream Stream(Line);
			std::string Key;
			uint64_t Kilobytes = 0;
			if (!(Stream >> Key >> Kilobytes))
				continue;

			if (Key == "MemTotal:")
				RAM.TotalBytes = Kilobytes * 1024;
			else if (Key == "MemAvailable:")
				RAM.AvailableBytes = Kilobytes * 1024;
		}
	}

	std::string ReadFirstLine(const std::string& Path)
	{
		std::ifstream File(Path);
		std::string Line;
		std::getline(File, Line);
		return TrimWhitespace(Line);
	}

	// Same names as lspci shows, for example "NVIDIA Corporation AD102 [GeForce RTX 4090]".
	std::string FindPCIDeviceName(const uint32_t VendorID, const uint32_t DeviceID)
	{
		char VendorKey[16] = {};
		char DeviceKey[16] = {};
		std::snprintf(VendorKey, sizeof(VendorKey), "%04x", VendorID);
		std::snprintf(DeviceKey, sizeof(DeviceKey), "%04x", DeviceID);

		for (const char* Path : { "/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids", "/usr/share/pci.ids" })
		{
			std::ifstream File(Path);
			if (!File.is_open())
				continue;

			// Vendor lines are "10de  NVIDIA Corporation", their device lines are "\t2684  AD102 [GeForce RTX 4090]".
			std::string VendorName;
			std::string Line;
			while (std::getline(File, Line))
			{
				if (Line.empty() || Line[0] == '#')
					continue;

				if (Line[0] != '\t')
				{
					if (!VendorName.empty())
						break;

					if (Line.size() > 6 && Line.compare(0, 4, VendorKey) == 0)
						VendorName = Line.substr(6);

					continue;
				}

				if (!VendorName.empty() && Line.size() > 7 && Line[1] != '\t' && Line.compare(1, 4, DeviceKey) == 0)
					return VendorName + " " + Line.substr(7);
			}

			return VendorName;
		}

		return "";
	}

	void QueryLinuxGPUs(std::vector<FEGPUInfo>& GPUs)
	{
		const std::string DRMPath = "/sys/class/drm/";
		std::vector<std::string> Cards;
		// Connectors are listed as "card0-HDMI-A-1" and render nodes as "renderD128".
		for (const std::string& Name : FILE_SYSTEM.GetDirectoryList(DRMPath))
		{
			if (Name.size() > 4 && Name.compare(0, 4, "card") == 0 && Name.find('-') == std::string::npos)
				Cards.push_back(Name);
		}

		std::sort(Cards.begin(), Cards.end(), [](const std::string& First, const std::string& Second) {
			return std::atoi(First.c_str() + 4) < std::atoi(Second.c_str() + 4);
		});

		for (const std::string& Card : Cards)
		{
			const std::string DevicePath = DRMPath + Card + "/device/";

			FEGPUInfo GPU;
			GPU.VendorID = static_cast<uint32_t>(std::strtoul(ReadFirstLine(DevicePath + "vendor").c_str(), nullptr, 16));
			GPU.DeviceID = static_cast<uint32_t>(std::strtoul(ReadFirstLine(DevicePath + "device").c_str(), nullptr, 16));
			// Virtual devices such as vgem have no PCI IDs.
			if (GPU.VendorID == 0)
				continue;

			GPU.Name = FindPCIDeviceName(GPU.VendorID, GPU.DeviceID);
			// Only amdgpu reports memory here.
			GPU.DedicatedMemoryBytes = std::strtoull(ReadFirstLine(DevicePath + "mem_info_vram_total").c_str(), nullptr, 10);

			char DriverLink[4096] = {};
			const ssize_t Length = readlink((DevicePath + "driver").c_str(), DriverLink, sizeof(DriverLink) - 1);
			if (Length > 0)
			{
				const std::string DriverPath(DriverLink, static_cast<size_t>(Length));
				GPU.DriverName = DriverPath.substr(DriverPath.find_last_of('/') + 1);
				// Modules built into the kernel, such as amdgpu and i915 from most distributions, have no version file.
				GPU.DriverVersion = ReadFirstLine("/sys/module/" + GPU.DriverName + "/version");
			}

			GPUs.push_back(GPU);
		}
	}
#endif
}

FESystemInfo::FESystemInfo()
{
	OSVersion.OperatingSystem = GetOS();

#if FE_PLATFORM(WINDOWS)
	QueryWindowsVersion(OSVersion);
	QueryWindowsCPU(CPUInfo);
#elif FE_PLATFORM(MACOS)
	QueryMacOSVersion(OSVersion);
	QueryMacOSCPU(CPUInfo);
#elif FE_PLATFORM(LINUX)
	QueryLinuxVersion(OSVersion);
	QueryLinuxCPU(CPUInfo);
#elif FE_PLATFORM(EMSCRIPTEN)
	// Browsers freeze the version in the user agent (Windows 11 reports 10.0, macOS reports 10.15.7), so only the OS is known.
	CPUInfo.LogicalCoreCount = FEQueryBrowserLogicalCoreCount();
#else
	#error "FESystemInfo: Unhandled platform."
#endif

	CPUInfo.Name = TrimWhitespace(CPUInfo.Name);
	if (CPUInfo.LogicalCoreCount == 0)
		CPUInfo.LogicalCoreCount = static_cast<int>(std::thread::hardware_concurrency());
}

FESystemInfo::~FESystemInfo() {}

FEOSVersion FESystemInfo::GetOSVersion() const
{
	return OSVersion;
}

std::string FESystemInfo::GetOSVersionString() const
{
	std::string Result = OSToString(OSVersion.OperatingSystem);

#if FE_PLATFORM(EMSCRIPTEN)
	Result += " (web browser)";
#elif FE_PLATFORM(LINUX)
	// Kernel release already starts with the version numbers.
	if (!OSVersion.Build.empty())
		Result += " " + OSVersion.Build;

	if (!OSVersion.Name.empty())
		Result += " (" + OSVersion.Name + ")";
#else
	// Version numbers are left out when there is a name, Windows 11 would show as 10.0.
	if (!OSVersion.Name.empty())
	{
		Result = OSVersion.Name;
	}
	else if (OSVersion.Major != 0 || OSVersion.Minor != 0)
	{
		Result += " " + std::to_string(OSVersion.Major) + "." + std::to_string(OSVersion.Minor);
		if (OSVersion.Patch != 0)
			Result += "." + std::to_string(OSVersion.Patch);
	}

	if (!OSVersion.Build.empty())
		Result += " build " + OSVersion.Build;
#endif

	return Result;
}

FECPUInfo FESystemInfo::GetCPUInfo() const
{
	return CPUInfo;
}

std::string FESystemInfo::GetCPUInfoString() const
{
	std::string Result = CPUInfo.Name.empty() ? "Unknown CPU" : CPUInfo.Name;
	if (CPUInfo.LogicalCoreCount > 0)
		Result += " (" + std::to_string(CPUInfo.LogicalCoreCount) + (CPUInfo.LogicalCoreCount == 1 ? " logical core)" : " logical cores)");

	return Result;
}

FERAMInfo FESystemInfo::GetRAMInfo() const
{
	FERAMInfo RAM;
#if FE_PLATFORM(WINDOWS)
	QueryWindowsRAM(RAM);
#elif FE_PLATFORM(MACOS)
	QueryMacOSRAM(RAM);
#elif FE_PLATFORM(LINUX)
	QueryLinuxRAM(RAM);
#elif FE_PLATFORM(EMSCRIPTEN)
	// navigator.deviceMemory is rounded and capped at 8 GB, so RAM is not reported.
#endif
	return RAM;
}

std::string FESystemInfo::GetRAMInfoString() const
{
	const FERAMInfo RAM = GetRAMInfo();
	if (RAM.TotalBytes == 0)
		return "Unknown RAM";

	std::string Result = BytesToGigabytesString(RAM.TotalBytes);
	if (RAM.AvailableBytes > 0)
		Result += " (" + BytesToGigabytesString(RAM.AvailableBytes) + " available)";

	return Result;
}

std::vector<FEGPUInfo> FESystemInfo::GetGPUInfo() const
{
	std::vector<FEGPUInfo> GPUs;
#if FE_PLATFORM(WINDOWS)
	QueryWindowsGPUs(GPUs);
#elif FE_PLATFORM(MACOS)
	QueryMacOSGPUs(GPUs);
#elif FE_PLATFORM(LINUX)
	QueryLinuxGPUs(GPUs);
#elif FE_PLATFORM(EMSCRIPTEN)
	char Renderer[512] = {};
	FEQueryBrowserGPURenderer(Renderer, sizeof(Renderer));
	if (Renderer[0] != '\0')
	{
		FEGPUInfo GPU;
		GPU.Name = Renderer;
		GPUs.push_back(GPU);
	}
#endif
	return GPUs;
}

std::string FESystemInfo::GetGPUInfoString() const
{
	const std::vector<FEGPUInfo> GPUs = GetGPUInfo();
	if (GPUs.empty())
		return "Unknown GPU";

	std::string Result;
	for (size_t i = 0; i < GPUs.size(); i++)
	{
		if (i > 0)
			Result += "\n";

		Result += GPUInfoToString(GPUs[i]);
	}

	return Result;
}