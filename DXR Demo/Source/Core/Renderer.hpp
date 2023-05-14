#pragma once
#include <memory>
#include <array>
#include <vector>

#include "DeviceContext.hpp"
#include "../Graphics/Buffer/Buffer.hpp"
#include "../Graphics/Shader.hpp"
#include "PipelineStateObject.hpp"

#include "../Rendering/Camera.hpp"

#include "../Graphics/Cube.hpp"

class Renderer
{
public:
	Renderer();
	~Renderer();

	void Initalize();
	void LoadAssets();

	void Update(Camera* pCamera);
	void Render(Camera* pCamera);
	void Resize();
	void Destroy();
	

private:
	std::unique_ptr<DeviceContext> m_DeviceCtx;

	void RecordCommandList(uint32_t CurrentFrame, Camera* pCamera);
	void ExecuteCommandLists();

	void SetRenderTarget();
	void ClearRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle);

	void TransitToRender();
	void TransitToPresent();

	void FlushGPU();
	void MoveToNextFrame();
	void WaitForGPU();

	void CreateDepthStencil();
	void CreatePipelines();

	bool bRaster{ true };

	std::array<const float, 4> m_ClearColor{ 0.5f, 0.5f, 1.0f, 1.0f };

	// DepthStencil
	inline ID3D12DescriptorHeap* GetDepthHeap() const { return m_DepthHeap.Get(); };
	ComPtr<ID3D12Resource> m_DepthStencil;
	ComPtr<ID3D12DescriptorHeap> m_DepthHeap;

	//
	ComPtr<ID3D12RootSignature> m_RootSignature;
	ComPtr<ID3D12PipelineState> m_PipelineState;

	// Shaders
	Shader m_VertexShader;
	Shader m_PixelShader;

	// Triangle data
	VertexBuffer<SimpleVertex> m_VertexBuffer;

	// Test
	Cube m_Cube;

};

