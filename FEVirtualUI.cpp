#include "FEVirtualUI.h"
using namespace FocalEngine;

FEVirtualUI::FEVirtualUI(int Width, int Height, std::string Name)
{
    this->Width = Width;
    this->Height = Height;
    this->Name = Name;
}

void FEVirtualUI::Initialize(GLuint FrameBuffer, int Width, int Height)
{
    TempImguiContext = ImGui::GetCurrentContext();
    
	this->Framebuffer = FrameBuffer;
	this->Width = Width;
	this->Height = Height;

	ImguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(ImguiContext);

	ImGuiIO& IO = ImGui::GetIO();
	IO.DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));
	IO.DeltaTime = 1.0f / 60.0f;

	ImGui_ImplOpenGL3_Init("#version 410");

    if (TempImguiContext != nullptr)
        ImGui::SetCurrentContext(TempImguiContext);
}

ImGuiContext* FEVirtualUI::GetImGuiContext() const
{
	return ImguiContext;
}

void FEVirtualUI::TerminateImGui()
{
	ImGui::SetCurrentContext(ImguiContext);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext(ImguiContext);
	ImGui::SetCurrentContext(nullptr);
}

FEVirtualUI::~FEVirtualUI()
{
	TerminateImGui();
}

std::string FEVirtualUI::GetName() const
{
	return Name;
}

void FEVirtualUI::SetName(const std::string NewValue)
{
	Name = NewValue;
}

void FEVirtualUI::GetClearColor(float* R, float* G, float* B, float* A) const
{
	*R = ClearColor[0];
	*G = ClearColor[1];
	*B = ClearColor[2];
	*A = ClearColor[3];
}

void FEVirtualUI::SetClearColor(float R, float G, float B, float A)
{
	ClearColor[0] = R;
	ClearColor[1] = G;
	ClearColor[2] = B;
	ClearColor[3] = A;
}

std::function<void()> FEVirtualUI::GetOnRenderFunction()
{
	return UserRenderFunctionImpl;
}

void FEVirtualUI::SetOnRenderFunction(std::function<void()> UserRenderFunction)
{
	UserRenderFunctionImpl = UserRenderFunction;
}

void FEVirtualUI::ClearOnRenderFunction()
{
	UserRenderFunctionImpl = nullptr;
}

void FEVirtualUI::BeginFrame()
{
	ImGui::SetCurrentContext(ImguiContext);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();
}

void FEVirtualUI::Render()
{
	if (UserRenderFunctionImpl != nullptr)
		UserRenderFunctionImpl();
}

void FEVirtualUI::EndFrame()
{
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
	glViewport(0, 0, Width, Height);

	glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!FunctionsToToAddFont.empty())
    {
        for (int i = 0; i < FunctionsToToAddFont.size(); i++)
        {
            FunctionsToToAddFont[i]();
            CallbacksOnFontReady[i]();
            FunctionsToToAddFont.erase(FunctionsToToAddFont.begin() + i);
            CallbacksOnFontReady.erase(CallbacksOnFontReady.begin() + i);
            i--;
        }

        ImGui::GetIO().Fonts->Build();
        ImGui_ImplOpenGL3_CreateFontsTexture();
    }
}

void FEVirtualUI::EnsureCorrectContextBegin()
{
	TempImguiContext = ImGui::GetCurrentContext();
	if (TempImguiContext != ImguiContext)
		ImGui::SetCurrentContext(ImguiContext);
}

void FEVirtualUI::EnsureCorrectContextEnd()
{
	if (TempImguiContext != ImguiContext)
		ImGui::SetCurrentContext(TempImguiContext);
}

void FEVirtualUI::AddOnResizeCallback(std::function<void(int, int)> UserOnResizeCallback)
{
	UserOnResizeCallbackFuncs.push_back(UserOnResizeCallback);
}

void FEVirtualUI::InvokeResize(GLuint FrameBuffer, int Width, int Height)
{
	EnsureCorrectContextBegin();

	this->Width = Width;
	this->Height = Height;
	this->Framebuffer = FrameBuffer;

	ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));

	for (int i = 0; i < UserOnResizeCallbackFuncs.size(); i++)
		UserOnResizeCallbackFuncs[i](Width, Height);

	EnsureCorrectContextEnd();
}

void FEVirtualUI::AddOnMouseEnterCallback(std::function<void(int)> UserOnMouseEnterCallback)
{
	UserOnMouseEnterCallbackFuncs.push_back(UserOnMouseEnterCallback);
}

void FEVirtualUI::InvokeMouseEnterCallback(int Entered)
{
	EnsureCorrectContextBegin();

    if (Entered == 1)
    {
        ImGui::GetIO().AddMousePosEvent(LastMouseX, LastMouseY);
    }
    else if (Entered == 0)
    {
        ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

	for (int i = 0; i < UserOnMouseEnterCallbackFuncs.size(); i++)
		UserOnMouseEnterCallbackFuncs[i](Entered);

	EnsureCorrectContextEnd();
}

void FEVirtualUI::AddOnMouseButtonCallback(std::function<void(int, int, int)> UserOnMouseButtonCallback)
{
	UserOnMouseButtonCallbackFuncs.push_back(UserOnMouseButtonCallback);
}

void FEVirtualUI::InvokeMouseButton(const int Button, const int Action, const int Mods)
{
	EnsureCorrectContextBegin();

	ImGui::GetIO().AddMouseButtonEvent(Button, Action == 1);

	for (int i = 0; i < UserOnMouseButtonCallbackFuncs.size(); i++)
		UserOnMouseButtonCallbackFuncs[i](Button, Action, Mods);

	EnsureCorrectContextEnd();
}

void FEVirtualUI::AddOnMouseMoveCallback(std::function<void(double, double)> UserOnMouseMoveCallback)
{
	UserOnMouseMoveCallbackFuncs.push_back(UserOnMouseMoveCallback);
}

void FEVirtualUI::InvokeMouseMove(const double Xpos, const double Ypos)
{
	EnsureCorrectContextBegin();

	ImGui::GetIO().MousePos = ImVec2(static_cast<float>(Xpos), static_cast<float>(Ypos));
    LastMouseX = static_cast<float>(Xpos);
    LastMouseY = static_cast<float>(Ypos);

	for (int i = 0; i < UserOnMouseMoveCallbackFuncs.size(); i++)
		UserOnMouseMoveCallbackFuncs[i](Xpos, Ypos);

	EnsureCorrectContextEnd();
}

void FEVirtualUI::AddOnCharCallback(std::function<void(unsigned int)> UserOnCharCallback)
{
	UserOnCharCallbackFuncs.push_back(UserOnCharCallback);
}

void FEVirtualUI::InvokeCharInput(unsigned int codepoint)
{
	EnsureCorrectContextBegin();

	ImGui::GetIO().AddInputCharacter(codepoint);

	for (int i = 0; i < UserOnCharCallbackFuncs.size(); i++)
		UserOnCharCallbackFuncs[i](codepoint);

	EnsureCorrectContextEnd();
}

void FEVirtualUI::AddOnKeyCallback(std::function<void(int, int, int, int)> UserOnKeyCallback)
{
	UserOnKeyCallbackFuncs.push_back(UserOnKeyCallback);
}

ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int key)
{
    switch (key)
    {
    case GLFW_KEY_TAB: return ImGuiKey_Tab;
    case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
    case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
    case GLFW_KEY_UP: return ImGuiKey_UpArrow;
    case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
    case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
    case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
    case GLFW_KEY_HOME: return ImGuiKey_Home;
    case GLFW_KEY_END: return ImGuiKey_End;
    case GLFW_KEY_INSERT: return ImGuiKey_Insert;
    case GLFW_KEY_DELETE: return ImGuiKey_Delete;
    case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
    case GLFW_KEY_SPACE: return ImGuiKey_Space;
    case GLFW_KEY_ENTER: return ImGuiKey_Enter;
    case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
    case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
    case GLFW_KEY_COMMA: return ImGuiKey_Comma;
    case GLFW_KEY_MINUS: return ImGuiKey_Minus;
    case GLFW_KEY_PERIOD: return ImGuiKey_Period;
    case GLFW_KEY_SLASH: return ImGuiKey_Slash;
    case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
    case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
    case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
    case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
    case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
    case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
    case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
    case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
    case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
    case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
    case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
    case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
    case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
    case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
    case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
    case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
    case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
    case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
    case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
    case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
    case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
    case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
    case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
    case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
    case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
    case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
    case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
    case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
    case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
    case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
    case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
    case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
    case GLFW_KEY_MENU: return ImGuiKey_Menu;
    case GLFW_KEY_0: return ImGuiKey_0;
    case GLFW_KEY_1: return ImGuiKey_1;
    case GLFW_KEY_2: return ImGuiKey_2;
    case GLFW_KEY_3: return ImGuiKey_3;
    case GLFW_KEY_4: return ImGuiKey_4;
    case GLFW_KEY_5: return ImGuiKey_5;
    case GLFW_KEY_6: return ImGuiKey_6;
    case GLFW_KEY_7: return ImGuiKey_7;
    case GLFW_KEY_8: return ImGuiKey_8;
    case GLFW_KEY_9: return ImGuiKey_9;
    case GLFW_KEY_A: return ImGuiKey_A;
    case GLFW_KEY_B: return ImGuiKey_B;
    case GLFW_KEY_C: return ImGuiKey_C;
    case GLFW_KEY_D: return ImGuiKey_D;
    case GLFW_KEY_E: return ImGuiKey_E;
    case GLFW_KEY_F: return ImGuiKey_F;
    case GLFW_KEY_G: return ImGuiKey_G;
    case GLFW_KEY_H: return ImGuiKey_H;
    case GLFW_KEY_I: return ImGuiKey_I;
    case GLFW_KEY_J: return ImGuiKey_J;
    case GLFW_KEY_K: return ImGuiKey_K;
    case GLFW_KEY_L: return ImGuiKey_L;
    case GLFW_KEY_M: return ImGuiKey_M;
    case GLFW_KEY_N: return ImGuiKey_N;
    case GLFW_KEY_O: return ImGuiKey_O;
    case GLFW_KEY_P: return ImGuiKey_P;
    case GLFW_KEY_Q: return ImGuiKey_Q;
    case GLFW_KEY_R: return ImGuiKey_R;
    case GLFW_KEY_S: return ImGuiKey_S;
    case GLFW_KEY_T: return ImGuiKey_T;
    case GLFW_KEY_U: return ImGuiKey_U;
    case GLFW_KEY_V: return ImGuiKey_V;
    case GLFW_KEY_W: return ImGuiKey_W;
    case GLFW_KEY_X: return ImGuiKey_X;
    case GLFW_KEY_Y: return ImGuiKey_Y;
    case GLFW_KEY_Z: return ImGuiKey_Z;
    case GLFW_KEY_F1: return ImGuiKey_F1;
    case GLFW_KEY_F2: return ImGuiKey_F2;
    case GLFW_KEY_F3: return ImGuiKey_F3;
    case GLFW_KEY_F4: return ImGuiKey_F4;
    case GLFW_KEY_F5: return ImGuiKey_F5;
    case GLFW_KEY_F6: return ImGuiKey_F6;
    case GLFW_KEY_F7: return ImGuiKey_F7;
    case GLFW_KEY_F8: return ImGuiKey_F8;
    case GLFW_KEY_F9: return ImGuiKey_F9;
    case GLFW_KEY_F10: return ImGuiKey_F10;
    case GLFW_KEY_F11: return ImGuiKey_F11;
    case GLFW_KEY_F12: return ImGuiKey_F12;
    case GLFW_KEY_F13: return ImGuiKey_F13;
    case GLFW_KEY_F14: return ImGuiKey_F14;
    case GLFW_KEY_F15: return ImGuiKey_F15;
    case GLFW_KEY_F16: return ImGuiKey_F16;
    case GLFW_KEY_F17: return ImGuiKey_F17;
    case GLFW_KEY_F18: return ImGuiKey_F18;
    case GLFW_KEY_F19: return ImGuiKey_F19;
    case GLFW_KEY_F20: return ImGuiKey_F20;
    case GLFW_KEY_F21: return ImGuiKey_F21;
    case GLFW_KEY_F22: return ImGuiKey_F22;
    case GLFW_KEY_F23: return ImGuiKey_F23;
    case GLFW_KEY_F24: return ImGuiKey_F24;
    default: return ImGuiKey_None;
    }
}

void FEVirtualUI::InvokeKeyInput(const int Key, const int Scancode, const int Action, const int Mods)
{
	EnsureCorrectContextBegin();

	ImGuiIO& IO = ImGui::GetIO();
	ImGuiKey imgui_key = ImGui_ImplGlfw_KeyToImGuiKey(Key);
    IO.AddKeyEvent(imgui_key, (Action == GLFW_PRESS));
    IO.SetKeyEventNativeData(imgui_key, Key, Scancode);

	for (int i = 0; i < UserOnKeyCallbackFuncs.size(); i++)
		UserOnKeyCallbackFuncs[i](Key, Scancode, Action, Mods);

	EnsureCorrectContextEnd();
}

void FEVirtualUI::AddOnDropCallback(std::function<void(int, const char**)> UserOnDropCallback)
{
	UserOnDropCallbackFuncs.push_back(UserOnDropCallback);
}

void FEVirtualUI::InvokeDropInput(const int Count, const char** Paths)
{
	EnsureCorrectContextBegin();

	for (int i = 0; i < UserOnDropCallbackFuncs.size(); i++)
		UserOnDropCallbackFuncs[i](Count, Paths);

	EnsureCorrectContextEnd();
}

void FEVirtualUI::AddOnScrollCallback(std::function<void(double, double)> UserOnScrollCallback)
{
	UserOnScrollCallbackFuncs.push_back(UserOnScrollCallback);
}

void FEVirtualUI::InvokeScrollInput(const double Xoffset, const double Yoffset)
{
	EnsureCorrectContextBegin();

    ImGui::GetIO().AddMouseWheelEvent((float)Xoffset, (float)Yoffset);

	for (int i = 0; i < UserOnScrollCallbackFuncs.size(); i++)
		UserOnScrollCallbackFuncs[i](Xoffset, Yoffset);

	EnsureCorrectContextEnd();
}

void FEVirtualUI::GetSize(int* Width, int* Height) const
{
	*Width = this->Width;
    *Height = this->Height;
}

void FEVirtualUI::SetSize(int NewWidth, int NewHeight)
{
	Width = NewWidth;
    Height = NewHeight;
}

int FEVirtualUI::GetWidth() const
{
	return Width;
}

int FEVirtualUI::GetHeight() const
{
	return Height;
}

std::string FEVirtualUI::GetID() const
{
	return ID;
}

void FEVirtualUI::ExecuteFunctionToAddFont(std::function<void()> Func, std::function<void()> CallbackOnFontReady)
{
    FunctionsToToAddFont.push_back(Func);
    CallbacksOnFontReady.push_back(CallbackOnFontReady);
}