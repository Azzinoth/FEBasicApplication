#pragma once

#include <string>

// FE_FIX_ME: GLFW types leak through the Monitor field and VideoMode pointer.
// Phase 2 will replace these with backend-agnostic fields. For now,
// MonitorInfo's consumers (Focal Engine, callbacks) still expect them.
struct GLFWmonitor;
struct GLFWvidmode;

namespace FocalEngine
{
	struct MonitorInfo
	{
		GLFWmonitor* Monitor = nullptr;
		std::string Name;
		int VirtualX = 0;
		int VirtualY = 0;
		const GLFWvidmode* VideoMode = nullptr;
	};
}
