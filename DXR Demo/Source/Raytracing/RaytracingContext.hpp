#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include "../Core/DeviceContext.hpp"
#include "../Graphics/Buffer/ConstantBuffer.hpp"
#include "../Graphics/Shader.hpp"
#include "AccelerationStructures.hpp"
#include "ShaderTable.hpp"

class Camera;
class VertexBuffer;
class IndexBuffer;

// Scene buffer for 3D
struct RaytraceBuffer
{
	DirectX::XMMATRIX ViewProjection;
	DirectX::XMVECTOR CameraPosition;
	DirectX::XMVECTOR LightPosition;
	DirectX::XMVECTOR LightAmbient;
	DirectX::XMVECTOR LightDiffuse;
};

struct CameraBuffer
{
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Projection;
	DirectX::XMMATRIX InversedView;
	DirectX::XMMATRIX InversedProjection;
};

struct CubeBuffer
{
	DirectX::XMVECTOR CubeColor;
};

// TODO:
enum class LocalRootArguments : uint8_t
{

};

enum class GlobalRootArguments : uint8_t
{

};

class RaytracingContext
{
public:
	RaytracingContext(DeviceContext* pDeviceCtx, Camera* pCamera, VertexBuffer& Vertex, IndexBuffer& Index);
	RaytracingContext(DeviceContext* pDeviceCtx, ShaderManager* pShaderManager, Camera* pCamera, VertexBuffer& Vertex, IndexBuffer& Index);
	RaytracingContext(DeviceContext* pDeviceCtx, ShaderManager* pShaderManager, Camera* pCamera, std::vector<VertexBuffer>& Vertex, std::vector<IndexBuffer>& Index);
	~RaytracingContext() noexcept(false);

	void Create();
	void OnRaytrace();
	void DispatchRaytrace();

	void BuildAccelerationStructures();
	void CreateRootSignatures();
	void CreateStateObject();
	void CreateOutputResource();

	void BuildShaderTables();

	void OutputToBackbuffer();

	//void OnResize();

	[[maybe_unused]]
	void SerializeAndCreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc, ComPtr<ID3D12RootSignature>* ppRootSignature) const;

	//test
	void SetConstBufferData();

	void DrawGUI();

private:
	DeviceContext* m_DeviceCtx{ nullptr };
	ShaderManager* m_ShaderManager{ nullptr };
	Camera* m_Camera{ nullptr };

	std::unique_ptr<DescriptorHeap> m_RaytracingHeap;

	ComPtr<ID3D12StateObject> m_StateObject;
	ComPtr<ID3D12StateObjectProperties> m_StateObjectProperties;

	ComPtr<ID3D12Resource> m_RaytracingOutput;
	Descriptor m_OutputUAV;
	Descriptor m_TopLevelView;

	// Shaders data
	ComPtr<IDxcBlob> m_RayGenShader;
	ComPtr<IDxcBlob> m_MissShader;
	ComPtr<IDxcBlob> m_HitShader;

	// Shader Root Signatures
	ComPtr<ID3D12RootSignature> m_GlobalRootSignature;
	ComPtr<ID3D12RootSignature> m_HitRootSignature;

	uint32_t m_PayloadSize		{ 0 };
	uint32_t m_AttributeSize	{ 0 };
	// 1 -> Primary rays
	uint32_t m_MaxRecursiveDepth{ 1 };

	AccelerationStructures m_AS;

	// Geometry for lookup
	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	std::vector<VertexBuffer> m_VertexBuffers;
	std::vector<IndexBuffer> m_IndexBuffers;

	// Shader Table
	ShaderTable m_RayGenTable;
	ShaderTable m_MissTable;
	ShaderTable m_HitTable;

	// For single buffer shader table storage
	//std::unique_ptr<ShaderTableBuilder> m_ShaderTableBuilder;
	//ComPtr<ID3D12Resource> m_ShaderTableStorage;

	void UpdateCamera();
	// Const buffer for 3D
	ConstantBuffer<RaytraceBuffer> m_SceneBuffer;
	RaytraceBuffer m_SceneData{};
	ConstantBuffer<CameraBuffer> m_CameraBuffer;
	CameraBuffer m_CameraData{};
	ConstantBuffer<CubeBuffer> m_CubeBuffer;
	CubeBuffer m_CubeData{};

	// Light data for shading
	std::array<float, 3> m_LightPosition{ 0.0f, 1.5f, -8.0f };
	std::array<float, 4> m_LightAmbient{ 0.5f, 0.5f, 0.5f, 1.0f };
	std::array<float, 4> m_LightDiffuse{ 1.0f, 0.0f, 0.0f, 1.0f };

	// Shader names
	// Those are associate with names given inside Raytracing shaders
	// Thus those MUST match
	static const wchar_t* m_HitGroupName;
	static const wchar_t* m_RayGenShaderName;
	static const wchar_t* m_MissShaderName;
	static const wchar_t* m_ClosestHitShaderName;

};
