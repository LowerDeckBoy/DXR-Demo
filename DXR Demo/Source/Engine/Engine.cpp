#include "Engine.hpp"
#include "../Inputs/CameraInputs.hpp"
#include "../Rendering/Camera.hpp"
#include <imgui/imgui.h>


Engine::Engine(HINSTANCE hInstance)
	: Window(hInstance)
{
	
}

Engine::~Engine()
{
	Destroy();
}

void Engine::Initialize()
{
	m_Timer = std::make_unique<Timer>();
	Window::Initialize();

	m_Camera = std::make_unique<Camera>();
	m_Renderer = std::make_unique<Renderer>(m_Camera.get(), m_Timer.get());
	m_Camera->Initialize(Window::Resolution().AspectRatio);

	m_Timer->Initialize();

	CameraInputs::Init();
}

void Engine::Run()
{
	m_Camera->ResetCamera();

	Show();
	m_Timer->Reset();
	m_Timer->Start();

	MSG msg{};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		CameraInputs::ReadInputs(m_Camera.get(), m_Timer->DeltaTime());

		m_Timer->Tick();
		m_Timer->GetFrameStats();

		if (!bAppPaused)
		{
			m_Renderer->Update(m_Camera.get());
			m_Renderer->Render(m_Camera.get());
			m_Camera->Update();
		}
		else
		{
			::Sleep(100);
		}
	}
}

void Engine::Destroy()
{
	CameraInputs::Release();
}

void Engine::OnResize()
{
	m_Renderer->OnResize();
	m_Camera->OnAspectRatioChange(Window::Resolution().AspectRatio);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT Engine::WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam))
		return true;

	switch (Msg)
	{
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			bAppPaused = true;
			m_Timer->Stop();
		}
		else
		{
			bAppPaused = false;
			m_Timer->Start();
		}
		return 0;
	}
	case WM_SIZE:
	{
		m_Resolution.Set(static_cast<uint32_t>(LOWORD(lParam)), static_cast<uint32_t>(HIWORD(lParam)));

		if (wParam == SIZE_MINIMIZED)
		{
			bAppPaused = true;
			bMinimized = true;
			bMaximized = false;

		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			bAppPaused = false;
			bMinimized = false;
			bMaximized = true;
			OnResize();
		}
		else if (wParam == SIZE_RESTORED)
		{
			if (bMinimized)
			{
				bAppPaused = false;
				bMinimized = false;
			}
			else if (bMaximized)
			{
				bAppPaused = false;
				bMaximized = false;
			}

			OnResize();
		}
		return 0;
	}
	case WM_ENTERSIZEMOVE:
	{
		bAppPaused = true;
		bIsResizing = true;

		m_Timer->Stop();

		break;
	}

	case WM_EXITSIZEMOVE:
	{
		bAppPaused = false;
		bIsResizing = false;

		m_Timer->Start();

		break;
	}
	case WM_KEYDOWN:
	{
		if (GetKeyState(VK_SPACE) & 0x800)
		{
			m_Renderer->bRaster = !m_Renderer->bRaster;
		}
		return 0;
	}

	case WM_CLOSE:
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		return 0;
	}
	}

	return ::DefWindowProcW(hWnd, Msg, wParam, lParam);
}
