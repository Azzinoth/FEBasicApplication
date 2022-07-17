#include "FEBasicApplication.h"
using namespace FocalEngine;

FEBasicApplication* FEBasicApplication::Instance = nullptr;
void(*FEBasicApplication::ClientWindowCloseCallbackImpl)() = nullptr;
void(*FEBasicApplication::ClientWindowResizeCallbackImpl)(int, int) = nullptr;
void(*FEBasicApplication::ClientMouseButtonCallbackImpl)(int, int, int) = nullptr;
void(*FEBasicApplication::ClientMouseMoveCallbackImpl)(double, double) = nullptr;
void(*FEBasicApplication::ClientKeyButtonCallbackImpl)(int, int, int, int) = nullptr;
void(*FEBasicApplication::ClientDropCallbackImpl)(int, const char**) = nullptr;

FEBasicApplication::FEBasicApplication()
{

}

FEBasicApplication::~FEBasicApplication()
{
	glfwDestroyWindow(Window);
	glfwTerminate();
}

void FEBasicApplication::InitWindow(const int Width, const int Height, std::string WindowTitle)
{
	WindowW = Width;
	WindowH = Height;
	this->WindowTitle = WindowTitle;

	glfwInit();

	Window = glfwCreateWindow(WindowW, WindowH, WindowTitle.c_str(), nullptr, nullptr);
	if (!Window)
		glfwTerminate();
	
	glfwMakeContextCurrent(Window);
	glewInit();

	glfwSetWindowCloseCallback(Window, WindowCloseCallback);
	glfwSetWindowSizeCallback(Window, WindowResizeCallback);
	glfwSetMouseButtonCallback(Window, MouseButtonCallback);
	glfwSetCursorPosCallback(Window, MouseMoveCallback);
	glfwSetKeyCallback(Window, KeyButtonCallback);
	glfwSetDropCallback(Window, DropCallback);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForOpenGL(Window, true);
	ImGui_ImplOpenGL3_Init("#version 410");
}

void FEBasicApplication::SetWindowCaption(const std::string NewCaption) const
{
	glfwSetWindowTitle(Window, NewCaption.c_str());
}

GLFWwindow* FEBasicApplication::GetGlfwWindow() const
{
	return Window;
}

void FEBasicApplication::BeginFrame()
{
	THREAD_POOL.Update();

	ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void FEBasicApplication::EndFrame() const
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(Window);
	glfwPollEvents();
}

bool FEBasicApplication::IsWindowOpened() const
{
	return !glfwWindowShouldClose(Window);
}

bool FEBasicApplication::IsWindowInFocus() const
{
	return glfwGetWindowAttrib(Window, GLFW_FOCUSED);
}

void FEBasicApplication::Terminate() const
{
	glfwSetWindowShouldClose(Window, true);
}

void FEBasicApplication::CancelTerination() const
{
	glfwSetWindowShouldClose(Window, false);
}

void FEBasicApplication::SetWindowCloseCallback(void(*Func)())
{
	ClientWindowCloseCallbackImpl = Func;
}

void FEBasicApplication::WindowCloseCallback(GLFWwindow* Window)
{
	APPLICATION.CancelTerination();

	if (ClientWindowCloseCallbackImpl != nullptr)
	{
		ClientWindowCloseCallbackImpl();
	}
	else
	{
		APPLICATION.Terminate();
	}
}

void FEBasicApplication::SetWindowResizeCallback(void(*Func)(int, int))
{
	ClientWindowResizeCallbackImpl = Func;
}

void FEBasicApplication::WindowResizeCallback(GLFWwindow* Window, int Width, int Height)
{
	int UpdatedWidth, UpdatedHeight;
	glfwGetWindowSize(APPLICATION.GetGlfwWindow(), &UpdatedWidth, &UpdatedHeight);

	if (UpdatedWidth == 0 || UpdatedHeight == 0)
		return;

	APPLICATION.WindowW = UpdatedWidth;
	APPLICATION.WindowH = UpdatedHeight;

	ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(UpdatedWidth), static_cast<float>(UpdatedHeight));

	if (ClientWindowResizeCallbackImpl != nullptr)
		ClientWindowResizeCallbackImpl(UpdatedWidth, UpdatedHeight);
}

void FEBasicApplication::SetMouseButtonCallback(void(*Func)(int, int, int))
{
	ClientMouseButtonCallbackImpl = Func;
}

void FEBasicApplication::MouseButtonCallback(GLFWwindow* Window, const int Button, const int Action, const int Mods)
{
	if (ClientMouseButtonCallbackImpl != nullptr)
		ClientMouseButtonCallbackImpl(Button, Action, Mods);
}

void FEBasicApplication::SetMouseMoveCallback(void(*Func)(double, double))
{
	ClientMouseMoveCallbackImpl = Func;
}

void FEBasicApplication::MouseMoveCallback(GLFWwindow* Window, const double Xpos, const double Ypos)
{
	if (ClientMouseMoveCallbackImpl != nullptr)
		ClientMouseMoveCallbackImpl(Xpos, Ypos);
}

void FEBasicApplication::SetKeyCallback(void(*Func)(int, int, int, int))
{
	ClientKeyButtonCallbackImpl = Func;
}

void FEBasicApplication::KeyButtonCallback(GLFWwindow* Window, const int Key, const int Scancode, const int Action, const int Mods)
{
	if (ClientKeyButtonCallbackImpl != nullptr)
		ClientKeyButtonCallbackImpl(Key, Scancode, Action, Mods);
}

void FEBasicApplication::SetDropCallback(void(*Func)(int, const char**))
{
	ClientDropCallbackImpl = Func;
}

void FEBasicApplication::DropCallback(GLFWwindow* Window, const int Count, const char** Paths)
{
	if (ClientDropCallbackImpl != nullptr)
		ClientDropCallbackImpl(Count, Paths);
}

void FEBasicApplication::GetWindowPosition(int* Xpos, int* Ypos) const
{
	glfwGetWindowPos(Window, Xpos, Ypos);
}

void FEBasicApplication::GetWindowSize(int* Width, int* Height) const
{
	glfwGetWindowSize(Window, Width, Height);
}

void FEBasicApplication::MinimizeWindow() const
{
	glfwIconifyWindow(Window);
}

void FEBasicApplication::RestoreWindow() const
{
	glfwRestoreWindow(Window);
}

std::string FEBasicApplication::GetUniqueId()
{
	static std::random_device RandomDevice;
	static std::mt19937 mt(RandomDevice());
	static std::uniform_int_distribution<int> distribution(0, 128);

	static bool FirstInitialization = true;
	if (FirstInitialization)
	{
		srand(static_cast<unsigned>(time(nullptr)));
		FirstInitialization = false;
	}

	std::string ID;
	ID += static_cast<char>(distribution(mt));
	for (size_t j = 0; j < 11; j++)
	{
		ID.insert(rand() % ID.size(), 1, static_cast<char>(distribution(mt)));
	}

	return ID;
}

std::string FEBasicApplication::GetUniqueHexID()
{
	const std::string ID = GetUniqueId();
	std::string IDinHex;

	for (size_t i = 0; i < ID.size(); i++)
	{
		IDinHex.push_back("0123456789ABCDEF"[(ID[i] >> 4) & 15]);
		IDinHex.push_back("0123456789ABCDEF"[ID[i] & 15]);
	}

	const std::string AdditionalRandomness = GetUniqueId();
	std::string AdditionalString;
	for (size_t i = 0; i < ID.size(); i++)
	{
		AdditionalString.push_back("0123456789ABCDEF"[(AdditionalRandomness[i] >> 4) & 15]);
		AdditionalString.push_back("0123456789ABCDEF"[AdditionalRandomness[i] & 15]);
	}
	std::string FinalID;

	for (size_t i = 0; i < ID.size() * 2; i++)
	{
		if (rand() % 2 - 1)
		{
			FinalID += IDinHex[i];
		}
		else
		{
			FinalID += AdditionalString[i];
		}
	}

	return FinalID;
}

bool FEBasicApplication::SetClipboardText(const std::string Text)
{
	if (OpenClipboard(nullptr))
	{
		EmptyClipboard();

		const HGLOBAL HMem = GlobalAlloc(GMEM_MOVEABLE, Text.size() + 1);
		memcpy(GlobalLock(HMem), Text.c_str(), Text.size() + 1);
		GlobalUnlock(HMem);

		SetClipboardData(CF_TEXT, HMem);

		CloseClipboard();
		return true;
	}

	return false;
}

std::string FEBasicApplication::GetClipboardText()
{
	std::string text;

	if (OpenClipboard(nullptr))
	{
		HANDLE data = nullptr;
		data = GetClipboardData(CF_TEXT);
		if (data != nullptr)
		{
			const char* PszText = static_cast<char*>(GlobalLock(data));
			if (PszText != nullptr)
				text = PszText;
		}

		CloseClipboard();
	}

	return text;
}