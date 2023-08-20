#pragma once
#include <memory>
#include <vector>

#include "DeviceContext.hpp"
#include "../Graphics/Buffer/Buffer.hpp"
#include "../Graphics/Shader.hpp"
#include "PipelineStateObject.hpp"
#include "../Graphics/Cube.hpp"
#include "../Graphics/Plane.hpp"
#include "../Raytracing/RaytracingContext.hpp"

#include "../Editor/Editor.hpp"

class Camera;
class Timer;

class Renderer
{
public:
	Renderer(Camera* pCamera, Timer* pTimer);
	~Renderer();

	void Initalize(Camera* pCamera, Timer* pTimer);
	void LoadAssets();

	void OnRaytrace();
	void Update(Camera* pCamera);
	void Render(Camera* pCamera);
	void OnResize();
	void Release();
	
	bool bRaster{ false };
	static bool bVSync;
private:
	std::unique_ptr<DeviceContext> m_DeviceCtx;

	void RecordCommandList(uint32_t CurrentFrame, Camera* pCamera);

	void BeginFrame();
	void EndFrame();

	void TransitToRender();
	void TransitToPresent(D3D12_RESOURCE_STATES StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET);

	void SetHeaps(ID3D12DescriptorHeap** ppHeap);

	void CreatePipelines();

	std::unique_ptr<Editor> m_Editor;

	ComPtr<ID3D12RootSignature> m_RootSignature;
	ComPtr<ID3D12PipelineState> m_PipelineState;

	std::shared_ptr<ShaderManager> m_ShaderManager;
	// Shaders
	ComPtr<IDxcBlob> m_VertexShader;
	ComPtr<IDxcBlob> m_PixelShader;

	Cube* m_Cube{ nullptr };
	Plane* m_Plane{ nullptr };

	// Raytracing
	std::unique_ptr<RaytracingContext> m_RaytracingContext;

};
