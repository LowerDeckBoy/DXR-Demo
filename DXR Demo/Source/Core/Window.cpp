#include "Window.hpp"

#include <dwmapi.h>
#pragma comment(lib, "dwmapi")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE 
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace { Window* window = 0; }
LRESULT CALLBACK MsgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return window->WindowProc(hWnd, Msg, wParam, lParam);
}

Window::Window(HINSTANCE hInstance) 
{
	window = this;
	m_hInstance = hInstance;

	m_Resolution.Width = 1280;
	m_Resolution.Height = 800;
	m_Resolution.AspectRatio = static_cast<float>(m_Resolution.Width) / static_cast<float>(m_Resolution.Height);
}

Window::~Window()
{
	::UnregisterClassW(m_WindowClass, m_hInstance);

	if (m_hWnd) m_hWnd = nullptr;
	if (m_hInstance) m_hInstance = nullptr;
}

void Window::Initialize()
{
	if (bInitialized)
	{
		// Debug message here
		return;
	}

	WNDCLASSEX wcex{};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hInstance = m_hInstance;
	wcex.lpfnWndProc = MsgProc;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpszClassName = m_WindowClass;
	wcex.hbrBackground = ::CreateSolidBrush(RGB(20, 20, 20));

	if (!::RegisterClassEx(&wcex))
	{
		throw std::exception("Failed to Register Window Class!\n");
	}

	m_Rect = { 0, 0, static_cast<LONG>(m_Resolution.Width), static_cast<LONG>(m_Resolution.Height) };
	::AdjustWindowRect(&m_Rect, WS_OVERLAPPEDWINDOW, false);
	const int32_t width = static_cast<int32_t>(m_Rect.right - m_Rect.left);
	const int32_t height = static_cast<int32_t>(m_Rect.bottom - m_Rect.top);

	m_hWnd = ::CreateWindow(m_WindowClass, m_WindowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_hInstance, 0);

	if (!m_hWnd)
		throw std::exception("Failed to Create Window!\n");

	const int32_t xPos = (::GetSystemMetrics(SM_CXSCREEN) - m_Rect.right)  / 2;
	const int32_t yPos = (::GetSystemMetrics(SM_CYSCREEN) - m_Rect.bottom) / 2;
	// 
	::SetWindowPos(m_hWnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	const BOOL bDarkMode{ TRUE };
	::DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &bDarkMode, sizeof(bDarkMode));

	bInitialized = true;
}

void Window::Show()
{
	if (!m_hWnd)
		throw std::exception("Failed to get Window HWND!\n");

	//::SendMessageA(m_hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
	::ShowWindow(m_hWnd, SW_SHOW);
	::SetForegroundWindow(m_hWnd);
	::SetFocus(m_hWnd);
	::UpdateWindow(m_hWnd);
}

HINSTANCE Window::GetHInstance()
{
	return m_hInstance;
}

HWND Window::GetHWND()
{
	return m_hWnd;
}

Window::DisplayResolution Window::Resolution()
{
	return m_Resolution;
}

void Window::ShowCursor() noexcept
{
	while (::ShowCursor(TRUE) < 0)
		bCursorVisible = true;
}

void Window::HideCursor() noexcept
{
	while (::ShowCursor(FALSE) >= 0)
		bCursorVisible = false;
}
