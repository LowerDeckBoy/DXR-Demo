#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include "../Core/DeviceContext.hpp"
#include "../Graphics/Buffer/ConstantBuffer.hpp"
#include "../Graphics/Shader.hpp"
#include "ShaderTable.hpp"
#include "AccelerationStructures.hpp"


class RaytracingContext
{
public:
	RaytracingContext(DeviceContext* pDeviceCtx, VertexBuffer& Vertex, IndexBuffer& Index);
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

private:
	DeviceContext* m_DeviceCtx{ nullptr };

	std::unique_ptr<DescriptorHeap> m_RaytracingHeap;

	ComPtr<ID3D12RootSignature> m_LocalRootSignature;
	ComPtr<ID3D12RootSignature> m_GlobalRootSignature;

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
	ComPtr<ID3D12RootSignature> m_RayGenSignature;
	ComPtr<ID3D12RootSignature> m_MissSignature;
	ComPtr<ID3D12RootSignature> m_HitSignature;

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

	// Shader names
	// Those are associate with names given inside Raytracing shaders
	// Thus those MUST match
	static const wchar_t* m_HitGroupName;
	static const wchar_t* m_RayGenShaderName;
	static const wchar_t* m_MissShaderName;
	static const wchar_t* m_ClosestHitShaderName;

};

