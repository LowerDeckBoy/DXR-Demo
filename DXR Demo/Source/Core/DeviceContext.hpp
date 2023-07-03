#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <wrl.h>

#include <d3dx12.h>
//#include <D3D12MA/D3D12MemAlloc.h>
#include "DescriptorHeap.hpp"
#include <string>


using Microsoft::WRL::ComPtr;
class DeviceContext
{
public:
	DeviceContext();
	~DeviceContext();

	void Create();

	void CreateDevice();
	void CreateCommandQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	void CreateCommandAllocators(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	void CreateCommandList(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT, ID3D12PipelineState* pInitialState = nullptr);

	void CreateDescriptorHeaps();

	void CreateSwapChain();
	void CreateBackbuffers();
	void CreateFence();

	void SetViewport();

	bool CheckRaytracingSupport(IDXGIAdapter1* pAdapter);

	void ExecuteCommandLists();

	void FlushGPU();
	void MoveToNextFrame();
	void WaitForGPU();

	void Destroy();

	static const uint32_t FRAME_COUNT{ 2 };

private:
	ComPtr<IDXGIAdapter1> m_Adapter;
	ComPtr<IDXGIFactory2> m_Factory;

	ComPtr<ID3D12Device5> m_Device;
	ComPtr<ID3D12Debug> m_DebugDevice;

	ComPtr<ID3D12GraphicsCommandList4> m_CommandList;
	ComPtr<ID3D12CommandAllocator> m_CommandAllocators[FRAME_COUNT];

	ComPtr<ID3D12CommandQueue> m_CommandQueue;

	ComPtr<IDXGISwapChain3> m_SwapChain;
	ComPtr<ID3D12Resource> m_RenderTargets[FRAME_COUNT];
	ComPtr<ID3D12DescriptorHeap> m_RenderTargetHeap;
	uint32_t m_RenderTargetHeapDescriptorSize{ 0 };

	DXGI_FORMAT m_RenderTargetFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };

	D3D12_VIEWPORT m_Viewport{};
	D3D12_RECT m_ViewportRect{};

	D3D_FEATURE_LEVEL m_FeatureLevel{ D3D_FEATURE_LEVEL_12_0 };

	std::unique_ptr<DescriptorHeap> m_MainHeap;

	bool bRaytracingSupport{ false };

public:
	ComPtr<ID3D12Fence> m_Fence;

	inline static uint32_t FRAME_INDEX{ 0 }; 
	HANDLE m_FenceEvent;
	uint64_t m_FenceValues[FRAME_COUNT]{};

	inline bool RaytraceSupported() { return bRaytracingSupport; }

	// Getters
	IDXGIAdapter1* GetAdapter() const { return m_Adapter.Get(); }
	IDXGIFactory2* GetFactory() const { return m_Factory.Get(); }

	ID3D12Device5* GetDevice() const { return m_Device.Get(); }

	ID3D12GraphicsCommandList4* GetCommandList() const { return m_CommandList.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator(uint32_t Index) const;
	ID3D12CommandQueue* GetCommandQueue() const { return m_CommandQueue.Get(); }

	IDXGISwapChain3* GetSwapChain() const { return m_SwapChain.Get(); }
	ID3D12DescriptorHeap* GetRenderTargetHeap() const { return m_RenderTargetHeap.Get(); }

	DXGI_FORMAT GetRenderTargetFormat() const { return m_RenderTargetFormat; }

	ID3D12Fence* GetFence() const { return m_Fence.Get(); }

	D3D_FEATURE_LEVEL GetFeatureLevel() const { return m_FeatureLevel; }

	D3D12_VIEWPORT GetViewport() { return m_Viewport; }
	D3D12_RECT GetViewportRect() { return m_ViewportRect; }

	ID3D12Resource* GetRenderTarget(uint32_t Index) const;

	uint32_t GetRenderTargetHeapDescriptorSize() const { return m_RenderTargetHeapDescriptorSize; }

	DescriptorHeap* GetMainHeap() { return m_MainHeap.get(); }

};
