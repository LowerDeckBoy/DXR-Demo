#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include "../Core/DeviceContext.hpp"
#include "../Graphics/Buffer/ConstantBuffer.hpp"
#include "../Graphics/Shader.hpp"
#include "ShaderTable.hpp"
#include "AccelerationStructures.hpp"

class Camera;

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

class RaytracingContext
{
public:
	RaytracingContext(DeviceContext* pDeviceCtx, Camera* pCamera, VertexBuffer& Vertex, IndexBuffer& Index);
	RaytracingContext(DeviceContext* pDeviceCtx, ShaderManager* pShaderManager, Camera* pCamera, VertexBuffer& Vertex, IndexBuffer& Index);
	~RaytracingContext();

	void Create();
	void OnRaytrace();
	void DispatchRaytrace();

	void BuildAccelerationStructures();
	void CreateRootSignatures();
	void CreateStateObject();
	void CreateOutputResource();

	void BuildShaderTables();

	void OutputToBackbuffer();

	void SerializeAndCreateRootSignature(D3D12_ROOT_SIGNATURE_DESC& Desc, ComPtr<ID3D12RootSignature>* ppRootSignature);

	//test
	void SetConstBufferData();

private:
	DeviceContext* m_DeviceCtx{ nullptr };
	ShaderManager* m_ShaderManager{ nullptr };
	Camera* m_Camera{ nullptr };

	std::unique_ptr<DescriptorHeap> m_RaytracingHeap;

	//ComPtr<ID3D12RootSignature> m_LocalRootSignature;
	//ComPtr<ID3D12RootSignature> m_GlobalRootSignature;

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

	// Shader names
	// Those are associate with names given inside Raytracing shaders
	// Thus those MUST match
	static const wchar_t* m_HitGroupName;
	static const wchar_t* m_RayGenShaderName;
	static const wchar_t* m_MissShaderName;
	static const wchar_t* m_ClosestHitShaderName;

};
