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
#include "../Graphics/Plane.hpp"
#include "../Raytracing/RaytracingContext.hpp"


class Renderer
{
public:
	Renderer(Camera* pCamera);
	~Renderer();

	void Initalize(Camera* pCamera);
	void LoadAssets();

	void OnRaytrace();
	void Update(Camera* pCamera);
	void Render(Camera* pCamera);
	void Resize();
	void Destroy();
	
	bool bRaster{ true };
private:
	std::unique_ptr<DeviceContext> m_DeviceCtx;

	void RecordCommandList(uint32_t CurrentFrame, Camera* pCamera);

	void BeginFrame();
	void EndFrame();

	void SetRenderTarget();
	void ClearRenderTarget();

	void TransitToRender();
	void TransitToPresent(D3D12_RESOURCE_STATES StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET);

	void CreateDepthStencil();
	void CreatePipelines();
	
	std::array<const float, 4> m_ClearColor{ 0.5f, 0.5f, 1.0f, 1.0f };
	//std::array<const float, 4> m_ClearColor{ 0.5f, 0.2f, 0.7f, 1.0f };

	// DepthStencil
	inline ID3D12DescriptorHeap* GetDepthHeap() const { return m_DepthHeap.Get(); };
	ComPtr<ID3D12Resource> m_DepthStencil;
	ComPtr<ID3D12DescriptorHeap> m_DepthHeap;

	//
	ComPtr<ID3D12RootSignature> m_RootSignature;
	ComPtr<ID3D12PipelineState> m_PipelineState;

	std::shared_ptr<ShaderManager> m_ShaderManager;
	// Shaders
	Shader m_VertexShader;
	Shader m_PixelShader;

	// Triangle data
	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	Cube m_Cube;
	Plane m_Plane;

	// Raytracing
	std::unique_ptr<RaytracingContext> m_RaytracingContext;

};
