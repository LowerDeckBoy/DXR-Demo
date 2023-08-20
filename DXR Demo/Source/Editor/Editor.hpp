#pragma once
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

class DeviceContext;
class Camera;
class Timer;

class Editor
{
public:
	Editor();
	~Editor();

	void Initialize(DeviceContext* pDeviceCtx, Camera* pCamera, Timer* pTimer);

	void BeginFrame();
	void EndFrame();

private:
	// Seperate Heap for GUI usage
	// Gets the job done as along as 
	// there is no requirement for using viewports.
	// Temporal solution
	void CreateHeap();

	DeviceContext* m_DeviceCtx{ nullptr };
	Timer* m_Timer{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;

	ImFont* m_Font{ nullptr };

};
