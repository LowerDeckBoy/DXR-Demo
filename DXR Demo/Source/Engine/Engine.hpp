#pragma once
#include "../Core/Window.hpp"
#include "../Core/Renderer.hpp"

class Camera;

// Application entry point
class Engine : public Window
{
public:
	explicit Engine(HINSTANCE hInstance);
	~Engine();

	void Initialize();

	void Run();
	void Destroy();

	virtual void OnResize() final;

private:
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<Camera> m_Camera;

	LRESULT WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) override;

};

