#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <cstring>
#include "../Core/DeviceContext.hpp"
#include "../Graphics/Cube.hpp"
#include "../Graphics/Buffer/ConstantBuffer.hpp"
#include "../Graphics/Shader.hpp"
#include "ShaderTable.hpp"

// Constants
struct SceneConstant
{
	DirectX::XMMATRIX ViewProjection;
	DirectX::XMVECTOR CameraPosition;
	DirectX::XMVECTOR LightPosition;
	DirectX::XMVECTOR LightAmbient;
	DirectX::XMVECTOR LightDiffuse;
};

struct CubeConstant
{
	DirectX::XMFLOAT4 Albedo;
};

struct VertexConstant
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
};


class RaytracingContext
{
public:
	//RaytracingContext();
	explicit RaytracingContext(DeviceContext* pDeviceCtx);
	~RaytracingContext();

	void Create();
	void OnRaytrace();
	void DoRaytrace();

	void CreateRootSignatures();
	void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* pRaytracingPipeline);
	void CreateStateObject();
	void CreateOutputResource();

	void BuildGeometry();
	void BuildAccelerationStructures();

	void BuildShaderTables();

	void OutputToBackbuffer();

	void SerializeAndCreateRootSignature(D3D12_ROOT_SIGNATURE_DESC& Desc, ComPtr<ID3D12RootSignature>* ppRootSignature);

private:
	DeviceContext* m_DeviceCtx{ nullptr };

	//ComPtr<ID3D12Device5> m_dxrDevice;
	//ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;

	ComPtr<ID3D12RootSignature> m_LocalRootSignature;
	ComPtr<ID3D12RootSignature> m_GlobalRootSignature;

	ComPtr<ID3D12StateObject> m_StateObject;
	ComPtr<ID3D12Resource> m_RaytracingOutput;

	Descriptor m_DescriptorUAV;

	//ConstantBuffer<SceneConstant> m_SceneCB;
	SceneConstant m_SceneCB[2]{};
	ConstantBuffer<CubeConstant>  m_CubeCB;

	ComPtr<IDxcBlob> m_RaytracingBlob;

	uint32_t m_PayloadSize{ 0 };
	uint32_t m_AttributeSize{ 0 };
	uint32_t m_MaxRecursiveDepth{ 1 };

	struct RaytracingBackbuffer
	{
		DXGI_FORMAT BackbufferFormat;
		uint64_t	Width;
		uint32_t	Height;
	} m_RaytracingBackbuffer;

	// ???
	union AlignedSceneConstantBuffer
	{
		SceneConstant constants;
		uint8_t alignmentPadding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
	};
	AlignedSceneConstantBuffer* m_mappedConstantData;
	ComPtr<ID3D12Resource>       m_perFrameConstants;

	// Structs
	ComPtr<ID3D12Resource> m_BottomLevelAccelerationStructure;
	ComPtr<ID3D12Resource> m_TopLevelAccelerationStructure;

	// Shader Table
	ComPtr<ID3D12Resource> m_RayGenShaderTable;
	ComPtr<ID3D12Resource> m_MissShaderTable;
	ComPtr<ID3D12Resource> m_HitGroupShaderTable;

	// SAMPLE CUBE
	struct RTCube
	{
		//VertexBuffer<CubeNormal> Vertex;
		VertexBuffer Vertex;
		IndexBuffer Index;

		//BufferSRV VertexSRV{};
		//BufferSRV IndexSRV{};

		CubeConstant m_cubeConst;
	} m_RTCube;

	// Shader names
	// Those are associate with names given inside Raytracing shaders
	// Thus those MUST match
	static const wchar_t* c_HitGroupName;
	static const wchar_t* c_RaygenShaderName;
	static const wchar_t* c_ClosestHitShaderName;
	static const wchar_t* c_MissShaderName;

};

