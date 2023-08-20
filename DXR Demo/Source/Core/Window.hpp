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
	Window(const Window&) = delete;
	virtual ~Window();

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

public:
	inline static struct DisplayResolution
	{
		void Set(uint32_t Width, uint32_t Height)
		{
			this->Width = Width;
			this->Height = Height;
			this->AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
		}

		uint32_t Width;
		uint32_t Height;
		float AspectRatio;
	} m_Resolution;

public:
	[[nodiscard]] 
	static HINSTANCE GetHInstance();
	[[nodiscard]] 
	static HWND GetHWND();

	[[nodiscard]]
	static DisplayResolution Resolution();

	static void ShowCursor() noexcept;
	static void HideCursor() noexcept;

protected:	
	// Application timer
	// used for app pausing,resuming elapsed time and DeltaTime
	std::unique_ptr<Timer> m_Timer;

	// Window states for window moving and resizing
	bool bAppPaused { false };
	bool bMinimized { false };
	bool bMaximized { false };
	bool bIsResizing{ false };

};
