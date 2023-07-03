#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstdint>
#include <memory>
#include "../Utilities/Timer.hpp"

// Done the ugly way but gets the jobs done
// Will rework later, or not idk
class Window
{
public:
	explicit Window(HINSTANCE hInstance);
	~Window();

	void Initialize();
	void Show();

	virtual void OnResize() = 0;

	virtual LRESULT WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) = 0;

private:
	inline static HINSTANCE m_hInstance{ nullptr };
	inline static HWND m_hWnd{ nullptr };

	LPCWSTR m_WindowClass{ L"Main" };
	LPCWSTR m_WindowName{ L"DXR Demo" };

	RECT m_Rect{};

	inline static bool bCursorVisible{ true };

	bool bInitialized{ false };

protected:
	static struct DisplayResolution
	{
		DisplayResolution() { }
		DisplayResolution(uint32_t Width, uint32_t Height)
		{
			Set(Width, Height);
		}

		static void Set(uint32_t Width, uint32_t Height)
		{
			Width = Width;
			Height = Height;
			AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
		}

		static inline uint32_t Width{ 1280 };
		static inline uint32_t Height{ 800 };
		static inline float AspectRatio{ static_cast<float>(Width) / static_cast<float>(Height) };
	} m_Resolution;

public:
	[[nodiscard]] inline static HINSTANCE GetHInstance() { return m_hInstance; }
	[[nodiscard]] inline static HWND GetHWND() { return m_hWnd; }

	static DisplayResolution Resolution() { return m_Resolution; }

	static void ShowCursor();
	static void HideCursor();

protected:	
	// Application timer
	// used for app pausing and resuming
	// also for Elapsed Time and DeltaTime
	std::unique_ptr<Timer> m_Timer;

	// Window states for window moving and resizing
	inline static bool bAppPaused { false };
	inline static bool bMinimized { false };
	inline static bool bMaximized { false };
	inline static bool bIsResizing{ false };

};
