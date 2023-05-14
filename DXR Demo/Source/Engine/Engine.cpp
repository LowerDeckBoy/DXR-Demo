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
	// Create Timer

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
			//OutputDebugStringA("Timer started\n");
		}
		return 0;
	}
	case WM_SIZE:
	{
		m_Resolution.Set(static_cast<uint32_t>(LOWORD(lParam)), static_cast<uint32_t>(HIWORD(lParam)));

		//if (!m_Renderer)
		//
		//	//OutputDebugStringA("Failed to get Renderer on resize event!\n");
		//	return 0;
		//}
		if (wParam == SIZE_MINIMIZED)
		{
			//OutputDebugStringA("SIZE_MINIMIZED\n");
			bAppPaused = true;
			bMinimized = true;
			bMaximized = false;

		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			//OutputDebugStringA("SIZE_MAXIMIZED\n");
			bAppPaused = false;
			bMinimized = false;
			bMaximized = true;
			OnResize();
		}
		else if (wParam == SIZE_RESTORED)
		{
			//OutputDebugStringA("SIZE_RESTORED\n");
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
		//OutputDebugStringA("WM_ENTERSIZEMOVE\n");
		bAppPaused = true;
		bIsResizing = true;
		m_Timer->Stop();
		//OutputDebugStringA("Timer stopped\n");

		return 0;
	}

	case WM_EXITSIZEMOVE:
	{
		//OutputDebugStringA("WM_EXITSIZEMOVE\n");
		bAppPaused = false;
		bIsResizing = false;
		m_Timer->Start();
		//OutputDebugStringA("Timer started\n");

		OnResize();

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
