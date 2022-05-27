#include "FEBasicApplication.h"
using namespace FocalEngine;

FEBasicApplication* FEBasicApplication::_instance = nullptr;
void(*FEBasicApplication::clientWindowCloseCallbackImpl)() = nullptr;
void(*FEBasicApplication::clientWindowResizeCallbackImpl)(int, int) = nullptr;
void(*FEBasicApplication::clientMouseButtonCallbackImpl)(int, int, int) = nullptr;
void(*FEBasicApplication::clientMouseMoveCallbackImpl)(double, double) = nullptr;
void(*FEBasicApplication::clientKeyButtonCallbackImpl)(int, int, int, int) = nullptr;
void(*FEBasicApplication::clientDropCallbackImpl)(int, const char**) = nullptr;

FEBasicApplication::FEBasicApplication()
{

}

FEBasicApplication::~FEBasicApplication()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void FEBasicApplication::createWindow(int width, int height, std::string WindowTitle)
{
	windowW = width;
	windowH = height;
	windowTitle = WindowTitle;

	glfwInit();

	window = glfwCreateWindow(windowW, windowH, windowTitle.c_str(), NULL, NULL);
	if (!window)
		glfwTerminate();
	
	glfwMakeContextCurrent(window);
	glewInit();

	glfwSetWindowCloseCallback(window, windowCloseCallback);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, mouseMoveCallback);
	glfwSetKeyCallback(window, keyButtonCallback);
	glfwSetDropCallback(window, dropCallback);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");
}

void FEBasicApplication::setWindowCaption(std::string newCaption)
{
	glfwSetWindowTitle(window, newCaption.c_str());
}

GLFWwindow* FEBasicApplication::getGLFWWindow()
{
	return window;
}

void FEBasicApplication::beginFrame()
{
	ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void FEBasicApplication::endFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(window);
	glfwPollEvents();
}

bool FEBasicApplication::isWindowOpened()
{
	return !glfwWindowShouldClose(window);
}

bool FEBasicApplication::isWindowInFocus()
{
	return glfwGetWindowAttrib(window, GLFW_FOCUSED);
}

void FEBasicApplication::terminate()
{
	glfwSetWindowShouldClose(window, true);
}

void FEBasicApplication::cancelTerination()
{
	glfwSetWindowShouldClose(window, false);
}

void FEBasicApplication::setWindowCloseCallback(void(*func)())
{
	clientWindowCloseCallbackImpl = func;
}

void FEBasicApplication::windowCloseCallback(GLFWwindow* window)
{
	APPLICATION.cancelTerination();

	if (clientWindowCloseCallbackImpl != nullptr)
	{
		clientWindowCloseCallbackImpl();
	}
	else
	{
		APPLICATION.terminate();
	}
}

void FEBasicApplication::setWindowResizeCallback(void(*func)(int, int))
{
	clientWindowResizeCallbackImpl = func;
}

void FEBasicApplication::windowResizeCallback(GLFWwindow* window, int width, int height)
{
	int updatedWidth, updatedHeight;
	glfwGetWindowSize(APPLICATION.getGLFWWindow(), &updatedWidth, &updatedHeight);

	if (updatedWidth == 0 || updatedHeight == 0)
		return;

	APPLICATION.windowW = updatedWidth;
	APPLICATION.windowH = updatedHeight;

	ImGui::GetIO().DisplaySize = ImVec2(float(updatedWidth), float(updatedHeight));

	if (clientWindowResizeCallbackImpl != nullptr)
		clientWindowResizeCallbackImpl(updatedWidth, updatedHeight);
}

void FEBasicApplication::setMouseButtonCallback(void(*func)(int, int, int))
{
	clientMouseButtonCallbackImpl = func;
}

void FEBasicApplication::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (clientMouseButtonCallbackImpl != nullptr)
		clientMouseButtonCallbackImpl(button, action, mods);
}

void FEBasicApplication::setMouseMoveCallback(void(*func)(double, double))
{
	clientMouseMoveCallbackImpl = func;
}

void FEBasicApplication::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (clientMouseMoveCallbackImpl != nullptr)
		clientMouseMoveCallbackImpl(xpos, ypos);
}

void FEBasicApplication::setKeyCallback(void(*func)(int, int, int, int))
{
	clientKeyButtonCallbackImpl = func;
}

void FEBasicApplication::keyButtonCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (clientKeyButtonCallbackImpl != nullptr)
		clientKeyButtonCallbackImpl(key, scancode, action, mods);
}

void FEBasicApplication::setDropCallback(void(*func)(int, const char**))
{
	clientDropCallbackImpl = func;
}

void FEBasicApplication::dropCallback(GLFWwindow* window, int count, const char** paths)
{
	if (clientDropCallbackImpl != nullptr)
		clientDropCallbackImpl(count, paths);
}

void FEBasicApplication::getWindowPosition(int* xpos, int* ypos)
{
	glfwGetWindowPos(window, xpos, ypos);
}

void FEBasicApplication::getWindowSize(int* width, int* height)
{
	glfwGetWindowSize(window, width, height);
}

void FEBasicApplication::minimizeWindow()
{
	glfwIconifyWindow(window);
}

void FEBasicApplication::restoreWindow()
{
	glfwRestoreWindow(window);
}