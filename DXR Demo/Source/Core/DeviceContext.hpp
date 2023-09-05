#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <wrl.h>

#include <d3dx12.h>
#include <D3D12MA/D3D12MemAlloc.h>
#include "DescriptorHeap.hpp"
#include <string>
#include <array>

#include "DescriptorHeap.hpp"

using Microsoft::WRL::ComPtr;

struct AdapterInfo
{
	static ComPtr<IDXGIAdapter3> Adapter;
	static std::string AdapterName;
};

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

	void CreateDepthStencil();

	void SetViewport();

	bool CheckRaytracingSupport(IDXGIAdapter1* pAdapter);

	void ExecuteCommandLists();
	void ExecuteCommandLists(bool bResetAllocator);

	void FlushGPU();
	void MoveToNextFrame();
	void WaitForGPU();

	void SetRenderTarget();
	void ClearRenderTarget();

	void ReleaseRenderTargets();
	void OnResize();

	void Release();

	static uint32_t QueryAdapterMemory();

	static const uint32_t FRAME_COUNT{ 3 };

	// Note: Mendatory
	inline bool RaytraceSupported() const noexcept { return bRaytracingSupport; }

	/* ===========================================Getters =========================================== */
	// Getters
	inline IDXGIAdapter3* GetAdapter() const noexcept { return m_Adapter.Get(); }
	inline IDXGIFactory2* GetFactory() const noexcept { return m_Factory.Get(); }

	inline ID3D12Device5* GetDevice() const noexcept { return m_Device.Get(); }

	inline ID3D12GraphicsCommandList4* GetCommandList() const noexcept { return m_CommandList.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator(uint32_t Index) const;
	ID3D12CommandAllocator* GetCommandAllocator() const;
	inline ID3D12CommandQueue* GetCommandQueue() const noexcept { return m_CommandQueue.Get(); }

	inline IDXGISwapChain3* GetSwapChain() const noexcept { return m_SwapChain.Get(); }
	inline ID3D12DescriptorHeap* GetRenderTargetHeap() const noexcept { return m_RenderTargetHeap.Get(); }
	ID3D12Resource* GetRenderTarget() const;

	inline DXGI_FORMAT GetRenderTargetFormat() const noexcept { return m_RenderTargetFormat; }

	inline ID3D12Fence* GetFence() const noexcept { return m_Fence.Get(); }

	inline D3D_FEATURE_LEVEL GetFeatureLevel() const noexcept { return m_FeatureLevel; }

	inline D3D12_VIEWPORT GetViewport() noexcept { return m_Viewport; }
	inline D3D12_RECT GetViewportRect() noexcept { return m_ViewportRect; }

	ID3D12Resource* GetRenderTarget(uint32_t Index) const;

	inline uint32_t GetRenderTargetHeapDescriptorSize() const noexcept { return m_RenderTargetHeapDescriptorSize; }

	inline DescriptorHeap* GetMainHeap() noexcept { return m_MainHeap.get(); }
	inline D3D12MA::Allocator* GetAllocator() { return m_Allocator.Get(); }


private:
	ComPtr<IDXGIAdapter3> m_Adapter;
	ComPtr<IDXGIFactory2> m_Factory;

	ComPtr<ID3D12Device5> m_Device;
	ComPtr<ID3D12Debug> m_DebugDevice;

	ComPtr<ID3D12GraphicsCommandList4> m_CommandList;
	std::array<ComPtr<ID3D12CommandAllocator>, FRAME_COUNT> m_CommandAllocators;

	ComPtr<ID3D12CommandQueue> m_CommandQueue;

	ComPtr<IDXGISwapChain3> m_SwapChain;
	std::array<ComPtr<ID3D12Resource>, FRAME_COUNT> m_RenderTargets;
	ComPtr<ID3D12DescriptorHeap> m_RenderTargetHeap;
	uint32_t m_RenderTargetHeapDescriptorSize{ 0 };

	DXGI_FORMAT m_RenderTargetFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };

	D3D12_VIEWPORT m_Viewport{};
	D3D12_RECT m_ViewportRect{};

	D3D_FEATURE_LEVEL m_FeatureLevel{ D3D_FEATURE_LEVEL_12_0 };

	std::unique_ptr<DescriptorHeap> m_MainHeap;

	// DepthStencil
	inline ID3D12DescriptorHeap* GetDepthHeap() const noexcept { return m_DepthHeap.Get(); };
	ComPtr<ID3D12Resource> m_DepthStencil;
	ComPtr<ID3D12DescriptorHeap> m_DepthHeap;

	// D3D12MA
	ComPtr<D3D12MA::Allocator> m_Allocator;

	//std::array<const float, 4> m_ClearColor{ 0.5f, 0.5f, 1.0f, 1.0f };
	std::array<const float, 4> m_ClearColor{ 0.5f, 0.2f, 0.7f, 1.0f };

	bool bRaytracingSupport{ false };

public:
	ComPtr<ID3D12Fence> m_Fence;

	inline static uint32_t FRAME_INDEX{ 0 }; 
	HANDLE m_FenceEvent;
	std::array<uint64_t, FRAME_COUNT> m_FenceValues{};

	
};
