#include "FEDeviceSurfaceOpenGL.h"
#include "FEDeviceOpenGL.h"
using namespace FocalEngine;

FEDeviceSurfaceOpenGL::~FEDeviceSurfaceOpenGL() {}

void FEDeviceSurfaceOpenGL::Resize(int Width, int Height)
{
	glViewport(0, 0, Width, Height);
}

void FEDeviceSurfaceOpenGL::SetClearColor(float R, float G, float B, float A)
{
	ClearR = R;
	ClearG = G;
	ClearB = B;
	ClearA = A;
}

void FEDeviceSurfaceOpenGL::BeginFrame()
{
	glClearColor(ClearR, ClearG, ClearB, ClearA);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FEDeviceSurfaceOpenGL::EndFrame()
{
	// Do nothing.
}

void FEDeviceSurfaceOpenGL::ImGuiInit()
{
	ImGui_ImplOpenGL3_Init("#version 410");
}

void FEDeviceSurfaceOpenGL::ImGuiShutdown()
{
	ImGui_ImplOpenGL3_Shutdown();
}

void FEDeviceSurfaceOpenGL::ImGuiNewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
}

void FEDeviceSurfaceOpenGL::ImGuiRenderDrawData()
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}