#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <wrl.h>

class DeviceContext;
class VertexBuffer;
class IndexBuffer;

class BottomLevel
{
public:
	~BottomLevel();

	void Create(ID3D12GraphicsCommandList4* pCommandList, ID3D12Resource* pScratch, ID3D12Resource* pResult, bool bAllowUpdate = false);
	// TODO: Change buffer into Mesh reference for models
	void AddBuffers(VertexBuffer Vertex, IndexBuffer Index, bool bOpaque);

	void GetBufferSizes(ID3D12Device5* pDevice, uint64_t* pScratchSize, uint64_t* pResultSize, bool bAllowUpdate);

	void Reset();

	// Temporal storage
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScratchBuffer;
	// Estimated memory requirement
	// aka output
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ResultBuffer;

private:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_Buffers;

	uint64_t m_ScratchSize{ 0 };
	uint64_t m_ResultSize { 0 };

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_Flags{ D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY };
};

class TopLevel
{
public:
	~TopLevel();

	void Create(ID3D12GraphicsCommandList4* pCommandList,
		ID3D12Resource* pScratch,
		ID3D12Resource* pResult,
		ID3D12Resource* pDescriptors,
		bool bUpdateOnly = false,
		ID3D12Resource* pPreviousResult = nullptr);

	void AddInstance(ID3D12Resource* pBottomLevelResult, DirectX::XMMATRIX Matrix, uint32_t InstanceID, uint32_t HitGroupID);

	void GetBufferSizes(ID3D12Device5* pDevice, uint64_t* pScratchSize, uint64_t* pResultSize, uint64_t* pDescSize, bool bAllowUpdate);

	void Reset();

	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScratchBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ResultBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InstanceDescsBuffer;

private:

	struct Instance
	{
		Instance(ID3D12Resource* pBottomLevelBuffer, DirectX::XMMATRIX& Matrix, uint32_t InstanceID, uint32_t HitGroupID) noexcept
			: BottomLevel(pBottomLevelBuffer), Matrix(Matrix), InstanceID(InstanceID), HitGroupID(HitGroupID)
		{ }

		ID3D12Resource*		BottomLevel{ nullptr };
		DirectX::XMMATRIX	Matrix;
		uint32_t			InstanceID;
		uint32_t			HitGroupID;
	};

	std::vector<Instance> m_Instances;

	uint64_t m_ScratchSize		{ 0 };
	uint64_t m_ResultSize		{ 0 };
	uint64_t m_InstanceDescsSize{ 0 };

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_Flags{ D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE };

};

class AccelerationStructures
{
public:
	void Init(DeviceContext* pDeviceCtx) noexcept { m_Device = pDeviceCtx; }

	void CreateBottomLevel(VertexBuffer& Vertex, IndexBuffer& Index, bool bOpaque = true);
	//void CreateBottomLevel(std::vector<VertexBuffer>& Vertex, std::vector<IndexBuffer>& Index, bool bOpaque = true);

	void CreateTopLevel(ID3D12Resource* pBuffer, DirectX::XMMATRIX& Matrix);
	//void CreateTopLevel(std::vector<BottomLevel>& pBuffers, DirectX::XMMATRIX& Matrix);
	//void CreateTopLevel(std::vector<ID3D12Resource*> pBuffers, DirectX::XMMATRIX& Matrix);

	BottomLevel m_BottomLevel;
	TopLevel	m_TopLevel;

	std::vector<BottomLevel> m_BottomLevels;

private:
	DeviceContext* m_Device{ nullptr };

};
