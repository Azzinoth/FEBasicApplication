#pragma once

// Usage: #if FE_PLATFORM(WINDOWS)
// A misspelled name or a missing #include of this header is a compile error.
#define FE_PLATFORM(Name) (FE_PLATFORM_INTERNAL_##Name())

// Android also defines __linux__, so it is tested before Linux.
#if defined(__EMSCRIPTEN__)
	#define FE_PLATFORM_INTERNAL_EMSCRIPTEN() 1
#elif defined(_WIN32)
	#define FE_PLATFORM_INTERNAL_WINDOWS() 1
#elif defined(__APPLE__)
	#include <TargetConditionals.h>
	#if TARGET_OS_OSX
		#define FE_PLATFORM_INTERNAL_MACOS() 1
	#else
		#error "FocalEngine: Only macOS is supported among Apple platforms."
	#endif
#elif defined(__ANDROID__)
	#error "FocalEngine: Android is not a supported platform."
#elif defined(__linux__)
	#define FE_PLATFORM_INTERNAL_LINUX() 1
#else
	#error "FocalEngine: Unsupported platform."
#endif

#ifndef FE_PLATFORM_INTERNAL_WINDOWS
	#define FE_PLATFORM_INTERNAL_WINDOWS() 0
#endif
#ifndef FE_PLATFORM_INTERNAL_MACOS
	#define FE_PLATFORM_INTERNAL_MACOS() 0
#endif
#ifndef FE_PLATFORM_INTERNAL_LINUX
	#define FE_PLATFORM_INTERNAL_LINUX() 0
#endif
#ifndef FE_PLATFORM_INTERNAL_EMSCRIPTEN
	#define FE_PLATFORM_INTERNAL_EMSCRIPTEN() 0
#endif