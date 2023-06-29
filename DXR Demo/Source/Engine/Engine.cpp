#include "Engine.hpp"

Engine::Engine(HINSTANCE hInstance)
	: Window(hInstance)
{
	
}

Engine::~Engine()
{
}

void Engine::Initialize()
{
	m_Timer = std::make_unique<Timer>();
	Window::Initialize();

	m_Renderer = std::make_unique<Renderer>();
	m_Camera = std::make_unique<Camera>();
	m_Camera->Initialize(Window::Resolution().AspectRatio);

	m_Timer->Initialize();
}

void Engine::Run()
{
	MSG msg{};

	m_Camera->ResetCamera();

	Show();
	m_Timer->Reset();
	m_Timer->Start();

	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

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
}

void Engine::OnResize()
{
}

LRESULT Engine::WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
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
				//OnResize();
			}
			else if (bMaximized)
			{
				bAppPaused = false;
				bMaximized = false;
				//OnResize();
			}
			else
			{
				//OnResize();
			}
			//else if (bIsResizing)
			//{
			//	//OnResize();
			//}
			OnResize();
			return 0;
		}
	}
	case WM_ENTERSIZEMOVE:
	{
		bAppPaused = true;
		bIsResizing = true;
		m_Timer->Stop();

		return 0;
	}

	case WM_EXITSIZEMOVE:
	{
		bAppPaused = false;
		bIsResizing = false;
		m_Timer->Start();

		OnResize();

		return 0;
	}
	case WM_KEYDOWN:
	{
		
		if (GetKeyState(VK_SPACE) & 0x800)
		{
			m_Renderer->bRaster = !m_Renderer->bRaster;
		}

		if (GetKeyState(VK_ESCAPE) & 0x800)
		{
			::PostQuitMessage(0);
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
