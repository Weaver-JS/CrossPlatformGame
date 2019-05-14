#pragma once 

#include <windowsx.h>
#include <iostream>

#define GLM_FORCE_RADIANS
#include "../Gameplay/glm/gtc/matrix_transform.hpp"
#include "Framework.h"
#include "../Gameplay/GameInterface.hpp"
#include "../Gameplay/profiler.hpp"
#include "../Gameplay/TaskManager.hpp"
#include "../Gameplay/TaskManagerHelpers.hpp"

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

static HGLRC s_OpenGLRenderingContext = nullptr;
static HDC s_WindowHandleToDeviceContext;

static bool windowActive = true;
float screenWidth = 1920, screenHeight = 1080;
//float screenWidth = 1920.0f, screenHeight = 1080.0f;

// Input
bool keyboard[256] = {};

// forward declarations
// File.cpp
namespace File {
	FullFile ReadFullFile(const wchar_t* path);
}
// Shader.cpp
namespace Shader {
	bool CompileShader(GLint &shaderId_, const char* shaderCode, GLenum shaderType);
	bool LinkShaders(GLint &programId_, GLint &vs, GLint &ps);
	bool initShaders(GLint &programID_, const char* vsData, const char* fsData);
}

namespace Texture {
	GLuint LoadTexture(const char* path);
	GLuint GenerateFontImgui();
}

namespace Timer {
	void initTimer();
	void update();
	double deltaTime();
};

#ifndef DUMMY_SCHEDULER

class WinJobScheduler : public Utilities::TaskManager::JobScheduler
{
public:


	void SwitchToFiber(void* fiber) override
	{
		::SwitchToFiber(fiber);
	}

	void* CreateFiber(size_t stackSize, void(__stdcall*call) (void*), void* parameter) override
	{
		return ::CreateFiber(stackSize, call, parameter);
	}

	void* GetFiberData() const override
	{
		return ::GetFiberData();
	}
};

WinJobScheduler s_JobScheduler;
 // !DUMMY_SCHEDULER;
#else
Utilities::TaskManager::JobScheduler s_JobScheduler;
#endif
Utilities::Profiler profiler;

inline void SetThreadName(unsigned long dwThreadID, const char* threadName)
{
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)  
	struct
	{
		DWORD dwType; // Must be 0x1000.  
		LPCSTR szName; // Pointer to name (in user addr space).  
		DWORD dwThreadID; // Thread ID (-1=caller thread).  
		DWORD dwFlags; // Reserved for future use, must be zero.  
	} info;
#pragma pack(pop)  

	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
#pragma warning(pop)  
}

void __stdcall WorkerThread(int idx)
{
	{
		char buffer[128];
		sprintf_s(buffer, "Worker Thread %d", idx);
		SetThreadName(GetCurrentThreadId(), buffer);

		//auto hr = SetThreadAffinityMask(GetCurrentThread(), 1LL << idx);
		//assert(hr != 0);
	}

	s_JobScheduler.SetRootFiber(ConvertThreadToFiber(nullptr), idx);


	s_JobScheduler.RunScheduler(idx, profiler);
}

namespace Renderer {
	RendererData* InitRendererData();
	void FinalizeRendererData(RendererData* rendererData);
	void GenerateBuffer(RendererData* rendererData);
	void BindBuffer(RendererData* rendererData, InstanceData instanceData);
	VAO CreateVertexArrayObject(const VertexTN* vertexs, int numVertex, const uint16_t* indexs, int numIndexs);
	void DrawElements(RendererData* rendererData);
};

void RenderDearImgui(const RendererData &renderer);

inline void GLAPIENTRY openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	OutputDebugStringA("openGL Debug Callback : [");
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		OutputDebugStringA("ERROR");
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		OutputDebugStringA("DEPRECATED_BEHAVIOR");
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		OutputDebugStringA("UNDEFINED_BEHAVIOR");
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		OutputDebugStringA("PORTABILITY");
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		OutputDebugStringA("PERFORMANCE");
		break;
	case GL_DEBUG_TYPE_OTHER:
		OutputDebugStringA("OTHER");
		break;
	default:
		OutputDebugStringA("????");
		break;
	}
	OutputDebugStringA(":");
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		OutputDebugStringA("LOW");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		OutputDebugStringA("MEDIUM");
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		OutputDebugStringA("HIGH");
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		OutputDebugStringA("NOTIFICATION");
		break;
	default:
		OutputDebugStringA("????");
		break;
	}
	OutputDebugStringA(":");
	char buffer[512];
	//sprintf_s(buffer, "%d", id);
	OutputDebugStringA(buffer);
	OutputDebugStringA("] ");
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		s_WindowHandleToDeviceContext = GetDC(hWnd);
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
			32,                        //Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                        //Number of bits for the depthbuffer
			8,                        //Number of bits for the stencilbuffer
			0,                        //Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		int letWindowsChooseThisPixelFormat;
		letWindowsChooseThisPixelFormat = ChoosePixelFormat(s_WindowHandleToDeviceContext, &pfd);
		SetPixelFormat(s_WindowHandleToDeviceContext, letWindowsChooseThisPixelFormat, &pfd);

		HGLRC tmpContext = wglCreateContext(s_WindowHandleToDeviceContext);
		wglMakeCurrent(s_WindowHandleToDeviceContext, tmpContext);

		// init glew
		GLenum err = glewInit();
		if (err != GLEW_OK)
		{
			assert(false); // TODO
		}

		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 1,
			WGL_CONTEXT_FLAGS_ARB,
			0 //WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if _DEBUG
			| WGL_CONTEXT_DEBUG_BIT_ARB
#endif
			, 0
		};

		s_OpenGLRenderingContext = wglCreateContextAttribsARB(s_WindowHandleToDeviceContext, 0, attribs);

		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(tmpContext);

		wglMakeCurrent(s_WindowHandleToDeviceContext, s_OpenGLRenderingContext);

		if (GLEW_ARB_debug_output)
		{
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallbackARB(openGLDebugCallback, nullptr);
			GLuint unusedIds = 0;
			glDebugMessageControl(GL_DONT_CARE,
				GL_DONT_CARE,
				GL_DONT_CARE,
				0,
				&unusedIds,
				true);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

#ifndef _DEBUG
		//ToggleFullscreen(hWnd);
#endif
	
	}
	break;
	case WM_DESTROY:		
		wglDeleteContext(s_OpenGLRenderingContext);
		s_OpenGLRenderingContext = nullptr;
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			return 0;
		}
		break;
	}
	case WM_SIZE:
	{
		screenWidth = LOWORD(lParam);  // Macro to get the low-order word.
		screenHeight = HIWORD(lParam); // Macro to get the high-order word.
		break;
	}
	case WM_ACTIVATE:
	{
		windowActive = wParam != WA_INACTIVE;
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

inline bool HandleMouse(const MSG& msg, Game::InputData &data_)
{
	if (msg.message == WM_LBUTTONDOWN)
	{
		data_.clicking = true;
		data_.clickDown = true;
		return true;
	}
	else if (msg.message == WM_LBUTTONUP)
	{
		data_.clicking = false;
		return true;
	}
	else if (msg.message == WM_RBUTTONDOWN)
	{
		data_.clickingRight = true;
		data_.clickDownRight = true;
		return true;
	}
	else if (msg.message == WM_RBUTTONUP)
	{
		data_.clickingRight = false;
		return true;
	}
	else if (msg.message == WM_MOUSEMOVE)
	{
		data_.mousePosition.x = (float)GET_X_LPARAM(msg.lParam);
		data_.mousePosition.y = (float)GET_Y_LPARAM(msg.lParam);
		return true;
	}
	else
	{
		return false;
	}
}

int __stdcall WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd)
{	// load window stuff
	MSG msg = { 0 };
	WNDCLASSW wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName = L"Aello World";
	wc.style = CS_OWNDC;
	if (!RegisterClassW(&wc))
		return 1;
	HWND hWnd = CreateWindowW(wc.lpszClassName, L"Hello World", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 480, 270, screenWidth, screenHeight, 0, 0, hInstance, 0);

	// init worker threads
	unsigned int numHardwareCores = std::thread::hardware_concurrency();
	numHardwareCores = numHardwareCores > 1 ? numHardwareCores - 1 : 1;
	int numThreads = THREADS;// (numHardwareCores < Utilities::MaxNumThreads - 1) ? numHardwareCores : Utilities::MaxNumThreads - 1;
	std::thread *workerThread = (std::thread*)alloca(sizeof(std::thread) * numThreads);

	Utilities::TaskManager::JobContext mainThreadContext; // {nullptr, &blockAllocator, numThreads, -1};
	
	mainThreadContext.scheduler = &s_JobScheduler;
	mainThreadContext.profiler = &profiler;
	//mainThreadContext = nullptr; 
	mainThreadContext.threadIndex = numThreads;
	mainThreadContext.fiberIndex = -1;
	//profiler.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN, 0, "Threads");
	s_JobScheduler.Init(numThreads, &profiler/*, &blockAllocator*/);

	for (int i = 0; i < numThreads; ++i)
	{
		new(&workerThread[i]) std::thread(WorkerThread, i);
	}

	std::mutex frameLockMutex;
	std::condition_variable frameLockConditionVariable;
	//init worker threads

	// Create Quad	
	VertexTN vtxs[4] = {
		{ { -0.5f, -0.5f, 0.0f },{ 1, 0, 0, 1 },{ 0, 0 } },
		{ { +0.5f, -0.5f, 0.0f },{ 0, 1, 0, 1 },{ 1, 0 } },
		{ { -0.5f, +0.5f, 0.0f },{ 0, 0, 1, 1 },{ 0, 1 } },
		{ { +0.5f, +0.5f, 0.0f },{ 1, 1, 0, 1 },{ 1, 1 } }
	};

	uint16_t idx[6] = {
		0, 1, 3,
		0, 3, 2
	};

	ImGui::CreateContext();
	
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(screenWidth, screenHeight);
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';
	
	RendererData* rendererData = Renderer::InitRendererData();
	RendererData* imguiData = Renderer::InitRendererData();

	rendererData->quadVAO = Renderer::CreateVertexArrayObject(vtxs, 4, idx, 6); 
	Renderer::GenerateBuffer(rendererData);

	// vao vbo imgui
	glGenBuffers(1, &imguiData->uniform);

	glBindBuffer(GL_UNIFORM_BUFFER, imguiData->uniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ImVec2), nullptr, GL_DYNAMIC_DRAW);


	// Imgui Buffers
	glGenBuffers(1, &imguiData->quadVAO.vertexBufferObject);
	glGenVertexArrays(1, &imguiData->quadVAO.vao);

	glBindBuffer(GL_ARRAY_BUFFER, imguiData->quadVAO.vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ImDrawVert) * 0x10000, nullptr, GL_DYNAMIC_DRAW);

	glBindVertexArray(imguiData->quadVAO.vao);

	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	// Create bind buffer
	InstanceData instanceData = { glm::mat4(), glm::vec4(1,1,1,1) };


	FullFile shaderVS = File::ReadFullFile(L"../resources/shaders/Simple.vs");
	FullFile shaderFS = File::ReadFullFile(L"../resources/shaders/circle.fs");
	FullFile shaderImguiVS = File::ReadFullFile(L"../resources/shaders/DearImgui.vert");
	FullFile shaderImguiFS = File::ReadFullFile(L"../resources/shaders/DearImgui.frag");
	
	bool dataInitialized = Shader::initShaders(rendererData->programID, (char*)shaderVS.data, (char*)shaderFS.data);
	bool dataImguiInitialized = Shader::initShaders(imguiData->programID, (char*)shaderImguiVS.data, (char*)shaderImguiFS.data);

	GLuint arrow_texture = Texture::LoadTexture("../resources/images/arrow.png");

	imguiData->textures[0] = Texture::GenerateFontImgui();

	// Timing
	Timer::initTimer();
	
	glm::mat4 projection = glm::ortho(-400.0f, 400.0f ,-300.0f, 300.0f, -5.0f, 5.0f);
	//VSync
	if (WGLEW_EXT_swap_control) wglSwapIntervalEXT(1);
	Game::GameData* gameData = Game::InitGamedata({ 400.0f, 300.0f});
	Game::RenderData renderData = {};	

	glClearColor(0, 1, 1, 1);

	bool quit = false;
	Game::InputData inputData = {};
	

	// Optimizado 0.5 - 0.9 ms al bindear la textura antes de entrar al loop
	// Y al solo usar un programa, lo mismo
	glActiveTexture(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, arrow_texture);
	do
	{
		float horizontal = 0, vertical = 0;
		inputData.shoot = false;

		// update message, keyboard
#pragma region INPUT

		{
			MSG msg = {};
			if (!windowActive) {
				for (int i = 0; i < 255; i++) {
					keyboard[i] = false;
				}
			}
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{

				bool fHandled = false;
				if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
				{
					fHandled = HandleMouse(msg, inputData);
				}

				bool processed = false;
				if (msg.message == WM_QUIT)
				{
					quit = true;
					processed = true;
				}
				else if (msg.message == WM_KEYDOWN)
				{
					keyboard[msg.wParam & 255] = true;
					processed = true;
					if (msg.wParam == VK_ESCAPE)
					{
						PostQuitMessage(0);
					}
				}
				else if (msg.message == WM_KEYUP)
				{
					keyboard[msg.wParam & 255] = false;
					processed = true;
				}

				if (!processed)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			if (keyboard['A'])
			{
				horizontal += +1;
			}
			if (keyboard['D'])
			{
				horizontal += -1;
			}
			if (keyboard['W'])
			{
				vertical += +1;
			}
			if (keyboard['S'])
			{
				vertical += -1;
			}
			if (keyboard['Q'])
			{
				inputData.shoot = true;
			}
		}

#pragma endregion
		// Timing
		{
			Timer::update();

			inputData.dt = Timer::deltaTime();
			inputData.horizontal = horizontal;
			inputData.vertical = vertical;
			inputData.screenSize = { 400.0f, 300.0f };
		}

		ImGui::NewFrame();
		// Imgui update for the new frame
		io.DeltaTime = (float)inputData.dt;

		io.MouseDown[0] = inputData.clicking;
		io.MouseDown[1] = inputData.clickingRight;
		io.MousePos = ImVec2(inputData.mousePosition.x, inputData.mousePosition.y);

		// Update Game
		{
			auto g = mainThreadContext.CreateProfileMarkGuard("update");
			renderData = Game::Update(*gameData, inputData, profiler, mainThreadContext);
		}
		
		// Render
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(rendererData->programID);

		for (auto sprite : renderData.sprites) {
			glm::mat4 translate = glm::translate(glm::mat4(), glm::vec3(sprite.position, 0));
			glm::mat4 scale = glm::scale(glm::mat4(), glm::vec3(sprite.size, 0.0f));
			glm::mat4 rotation = glm::rotate(glm::mat4(), (float)sprite.angle, glm::vec3(0.0f, 0.0f, 1.0f));
			if (sprite.texture == Game::RenderData::TextureNames::LENA) {

				glm::mat4 initTranslate = glm::translate(glm::mat4(), glm::vec3(0.5, 0.0, 0.0f));
				instanceData = { projection * translate * rotation * scale * initTranslate , sprite.color };
			}
			else
			{
				instanceData = { projection * translate * rotation * scale, sprite.color };
			}
			Renderer::BindBuffer(rendererData, instanceData);
			Renderer::DrawElements(rendererData);
		}

		glUseProgram(0);

		profiler.DrawProfilerToImGUI(Utilities::MaxNumThreads);
		ImGui::Render();
		RenderDearImgui(*imguiData);

		SwapBuffers(s_WindowHandleToDeviceContext);

	} while (!quit);

	glBindTexture(GL_TEXTURE_2D, 0);
	Game::FinalizeGameData(gameData);
	Renderer::FinalizeRendererData(rendererData);
	return 0;
}

inline void RenderDearImgui(const RendererData &renderer)
{
	ImDrawData* draw_data = ImGui::GetDrawData();

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
		return;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	// We are using the OpenGL fixed pipeline to make the example code simpler to read!
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	GLuint program = renderer.programID;
	glUseProgram(program);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, renderer.uniform);

	// Setup viewport, orthographic projection matrix
	glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);

	glBindBuffer(GL_UNIFORM_BUFFER, renderer.uniform);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ImVec2), (GLvoid*)&io.DisplaySize);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	auto texture = renderer.textures[0];

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(renderer.quadVAO.vao);
	glBindBuffer(GL_ARRAY_BUFFER, renderer.quadVAO.vertexBufferObject);

	// Render command lists
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawVert* vtx_buffer = &cmd_list->VtxBuffer.front();
		const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ImDrawVert) * cmd_list->VtxBuffer.size(), (GLvoid*)vtx_buffer);

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				if (pcmd->TextureId == nullptr)
				{
					glBindTexture(GL_TEXTURE_2D, texture);
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				}
				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				static_assert(sizeof(ImDrawIdx) == 2 || sizeof(ImDrawIdx) == 4, "indexes are 2 or 4 bytes long");
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
			}
			idx_buffer += pcmd->ElemCount;
		}
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//// Restore modified state
	glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
	glDisable(GL_SCISSOR_TEST);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}