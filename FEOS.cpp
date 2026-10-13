#include "FEOS.h"
#include "FEPlatform.h"

#if FE_PLATFORM(EMSCRIPTEN)
#include <emscripten.h>

// Returns OS values. Mobile is tested first: Android user agents contain "Linux",
// iPhone ones contain "Mac OS X", and iPadOS Safari presents itself as a touch-screen Mac.
EM_JS(int, FEQueryBrowserOS, (), {
	if (typeof navigator === "undefined") return 0;
	const Info = ((navigator.userAgentData && navigator.userAgentData.platform) || "") + " " + navigator.userAgent;
	if (/Android/.test(Info)) return 4;
	if (/iPhone|iPad|iPod/.test(Info)) return 5;
	if (/Windows/.test(Info)) return 1;
	if (/Mac/.test(Info)) return navigator.maxTouchPoints > 1 ? 5 : 2;
	if (/Linux|X11|CrOS/.test(Info)) return 3;
	return 0;
});
#endif

using namespace FocalEngine;

OS FocalEngine::GetOS()
{
#if FE_PLATFORM(WINDOWS)
	return OS::Windows;
#elif FE_PLATFORM(MACOS)
	return OS::MacOS;
#elif FE_PLATFORM(LINUX)
	return OS::Linux;
#elif FE_PLATFORM(EMSCRIPTEN)
	static const OS BrowserOS = static_cast<OS>(FEQueryBrowserOS());
	return BrowserOS;
#else
	#error "GetOS: Unhandled platform."
#endif
}

std::string FocalEngine::OSToString(const OS Value)
{
	switch (Value)
	{
		case OS::Windows: return "Windows";
		case OS::MacOS:   return "MacOS";
		case OS::Linux:   return "Linux";
		case OS::Android: return "Android";
		case OS::iOS:     return "iOS";
		default:          return "Unknown";
	}
}

OS FocalEngine::OSFromString(const std::string& Name)
{
	if (Name == "Windows") return OS::Windows;
	if (Name == "MacOS")   return OS::MacOS;
	if (Name == "Linux")   return OS::Linux;
	if (Name == "Android") return OS::Android;
	if (Name == "iOS")     return OS::iOS;
	return OS::Unknown;
}