#include "AccelerationStructures.hpp"
#include "../Graphics/Buffer/Buffer.hpp"
#include "../Graphics/Buffer/BufferUtils.hpp"
#include "../Utilities/Utilities.hpp"

#ifndef ALIGN
#define ALIGN(v, powerOf2Alignment)									\
	(((v) + (powerOf2Alignment) - 1) & ~((powerOf2Alignment) - 1))
#endif

BottomLevel::~BottomLevel()
{
	Reset();
}

void BottomLevel::Create(ID3D12GraphicsCommandList4* pCommandList, ID3D12Resource* pScratch, ID3D12Resource* pResult, bool bAllowUpdate)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc{};
	desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	desc.Inputs.Flags = (bAllowUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE);
	desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	desc.Inputs.NumDescs = static_cast<uint32_t>(m_Buffers.size());
	desc.Inputs.pGeometryDescs = m_Buffers.data();

	desc.ScratchAccelerationStructureData	= { pScratch->GetGPUVirtualAddress() };
	desc.DestAccelerationStructureData		= { pResult->GetGPUVirtualAddress() };
	desc.SourceAccelerationStructureData	= { 0 };

	pCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier{};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pResult;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	pCommandList->ResourceBarrier(1, &uavBarrier);
}

void BottomLevel::AddBuffers(VertexBuffer Vertex, IndexBuffer Index, bool bOpaque)
{
	D3D12_RAYTRACING_GEOMETRY_DESC desc{};
	desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	desc.Flags = (bOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE);

	desc.Triangles.VertexBuffer.StartAddress  = Vertex.View.BufferLocation;
	desc.Triangles.VertexBuffer.StrideInBytes = Vertex.View.StrideInBytes;
	desc.Triangles.VertexCount = Vertex.Buffer::GetData().ElementsCount;
	desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

	desc.Triangles.IndexBuffer = Index.View.BufferLocation;
	desc.Triangles.IndexCount  = Index.Count;
	desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

	desc.Triangles.Transform3x4 = 0;

	m_Buffers.emplace_back(desc);
}

void BottomLevel::GetBufferSizes(ID3D12Device5* pDevice, uint64_t* pScratchSize, uint64_t* pResultSize, bool bAllowUpdate)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc{};
	prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	prebuildDesc.Flags = (bAllowUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE);
	prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildDesc.NumDescs = static_cast<uint32_t>(m_Buffers.size());
	prebuildDesc.pGeometryDescs = m_Buffers.data();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};

	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &prebuildInfo);

	*pScratchSize = ALIGN(prebuildInfo.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	*pResultSize  = ALIGN(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	m_ScratchSize = *pScratchSize;
	m_ResultSize  = *pResultSize;
}

void BottomLevel::Reset()
{
	m_ScratchBuffer = nullptr;
	m_ResultBuffer = nullptr;

	m_ScratchSize = 0;
	m_ResultSize = 0;
}

TopLevel::~TopLevel()
{
	Reset();
}

void TopLevel::Create(ID3D12GraphicsCommandList4* pCommandList, ID3D12Resource* pScratch, ID3D12Resource* pResult, ID3D12Resource* pDescriptors, bool bUpdateOnly, ID3D12Resource* pPreviousResult)
{
	D3D12_RAYTRACING_INSTANCE_DESC* descs{};
	ThrowIfFailed(pDescriptors->Map(0, nullptr, reinterpret_cast<void**>(&descs)));

	if (!descs)
		throw std::exception("Failed to cast Descriptor Buffers");
	
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_Instances.size()); i++)
	{
		descs[i].AccelerationStructure = m_Instances.at(i)->BottomLevel->GetGPUVirtualAddress();
		DirectX::XMMATRIX transform = DirectX::XMMatrixTranspose(m_Instances.at(i)->Matrix);
		std::memcpy(descs[i].Transform, &transform, sizeof(descs[i].Transform));
		descs[i].InstanceID = m_Instances.at(i)->InstanceID;
		descs[i].InstanceContributionToHitGroupIndex = m_Instances.at(i)->HitGroupID;
		descs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
		descs[i].InstanceMask = 0xFF;
	}
	pDescriptors->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	buildDesc.Inputs.NumDescs = static_cast<uint32_t>(m_Instances.size());
	buildDesc.Inputs.InstanceDescs = pDescriptors->GetGPUVirtualAddress();
	buildDesc.Inputs.Flags = m_Flags;

	buildDesc.SourceAccelerationStructureData	= 0;
	buildDesc.ScratchAccelerationStructureData	= pScratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData		= pResult->GetGPUVirtualAddress();

	pCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier{};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pResult;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	pCommandList->ResourceBarrier(1, &uavBarrier);

	SAFE_RELEASE(m_ScratchBuffer);
}

void TopLevel::AddInstance(ID3D12Resource* pBottomLevelResult, DirectX::XMMATRIX Matrix, uint32_t InstanceID, uint32_t HitGroupID)
{
	m_Instances.emplace_back(new Instance(pBottomLevelResult, Matrix, InstanceID, HitGroupID));
}

void TopLevel::AddInstance(Instance* pInstance)
{
	m_Instances.emplace_back(pInstance);
}

void TopLevel::GetBufferSizes(ID3D12Device5* pDevice, uint64_t* pScratchSize, uint64_t* pResultSize, uint64_t* pDescSize, bool bAllowUpdate)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc{};
	prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildDesc.NumDescs = static_cast<uint32_t>(m_Instances.size());
	prebuildDesc.Flags = m_Flags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &prebuildInfo);

	prebuildInfo.ScratchDataSizeInBytes   = ALIGN(prebuildInfo.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	prebuildInfo.ResultDataMaxSizeInBytes = ALIGN(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	m_InstanceDescsSize = ALIGN(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * static_cast<UINT64>(m_Instances.size()), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	m_ScratchSize = prebuildInfo.ScratchDataSizeInBytes;
	m_ResultSize  = prebuildInfo.ResultDataMaxSizeInBytes;

	*pScratchSize = m_ScratchSize;
	*pResultSize  = m_ResultSize;
	*pDescSize	  = m_InstanceDescsSize;
}

void TopLevel::Reset()
{
	m_ScratchBuffer			= nullptr;
	m_ResultBuffer			= nullptr;
	m_InstanceDescsBuffer	= nullptr;

	m_ScratchSize		= 0;
	m_ResultSize		= 0;
	m_InstanceDescsSize = 0;
}

void AccelerationStructures::CreateBottomLevels(std::vector<VertexBuffer>& VertexBuffers, std::vector<IndexBuffer>& IndexBuffers, bool bOpaque)
{
	for (size_t i = 0; i < VertexBuffers.size(); i++)
	{
		uint64_t scratchSize{ 0 };
		uint64_t resultSize{ 0 };
		BottomLevel* blas = new BottomLevel();
		blas->AddBuffers(VertexBuffers.at(i), IndexBuffers.at(i), true);
		blas->GetBufferSizes(m_Device->GetDevice(), &scratchSize, &resultSize, false);

		auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
		BufferUtils::Create(m_Device->GetDevice(), &blas->m_ScratchBuffer, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, heapProperties);
		BufferUtils::Create(m_Device->GetDevice(), &blas->m_ResultBuffer, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, heapProperties);

		m_Device->ExecuteCommandLists(true);

		blas->Create(m_Device->GetCommandList(), blas->m_ScratchBuffer.Get(), blas->m_ResultBuffer.Get());
		m_BottomLevels.emplace_back(blas);
	}
}

/*
void AccelerationStructures::CreateBottomLevel(std::vector<VertexBuffer>& Vertex, std::vector<IndexBuffer>& Index, bool bOpaque)
{
	
	for (size_t i = 0; i < Vertex.size(); i++)
	{
		BottomLevel bottomLevel;

		bottomLevel.AddBuffers(Vertex.at(i), Index.at(i), bOpaque);

		uint64_t scratchSize{ 0 };
		uint64_t resultSize{ 0 };
		bottomLevel.GetBufferSizes(m_Device->GetDevice(), &scratchSize, &resultSize, false);

		auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
		BufferUtils::Create(m_Device->GetDevice(), &bottomLevel.m_ScratchBuffer, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, heapProperties);
		BufferUtils::Create(m_Device->GetDevice(), &bottomLevel.m_ResultBuffer, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, heapProperties);

		bottomLevel.Create(m_Device->GetCommandList(), bottomLevel.m_ScratchBuffer.Get(), bottomLevel.m_ResultBuffer.Get());
		m_BottomLevels.emplace_back(bottomLevel);
	}
}
*/

void AccelerationStructures::CreateTopLevel()
{
	for (size_t i = 0; i < m_BottomLevels.size(); i++)
	{
		const auto matrix{ DirectX::XMMatrixIdentity() };

		m_TopLevel.AddInstance(m_BottomLevels.at(i)->m_ResultBuffer.Get(), matrix, static_cast<uint32_t>(i), static_cast<uint32_t>(i * 2));
	}

	uint64_t scratchSize{};
	uint64_t resultSize{};
	uint64_t instancesDescSize{};

	auto device{ m_Device->GetDevice() };
	m_TopLevel.GetBufferSizes(device, &scratchSize, &resultSize, &instancesDescSize, false);

	auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	m_TopLevel.m_ScratchBuffer = BufferUtils::Create(device, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, heapProperties);
	m_TopLevel.m_ResultBuffer = BufferUtils::Create(device, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, heapProperties);

	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	m_TopLevel.m_InstanceDescsBuffer = BufferUtils::Create(device, instancesDescSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, heapProperties);

	m_Device->ExecuteCommandLists(true);

	m_TopLevel.Create(m_Device->GetCommandList(), m_TopLevel.m_ScratchBuffer.Get(), m_TopLevel.m_ResultBuffer.Get(), m_TopLevel.m_InstanceDescsBuffer.Get());

}

void AccelerationStructures::CreateTopLevel(ID3D12Resource* pBuffer, DirectX::XMMATRIX& Matrix)
{
	const auto matrix { DirectX::XMMatrixIdentity() };
	m_TopLevel.AddInstance(pBuffer, matrix, 0, 0);

	uint64_t scratchSize{};
	uint64_t resultSize{};
	uint64_t instancesDescSize{};

	auto device{ m_Device->GetDevice() };
	m_TopLevel.GetBufferSizes(device, &scratchSize, &resultSize, &instancesDescSize, false);

	auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	m_TopLevel.m_ScratchBuffer = BufferUtils::Create(device, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, heapProperties);
	m_TopLevel.m_ResultBuffer = BufferUtils::Create(device, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, heapProperties);

	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	m_TopLevel.m_InstanceDescsBuffer = BufferUtils::Create(device, instancesDescSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, heapProperties);

	m_Device->ExecuteCommandLists(true);

	m_TopLevel.Create(m_Device->GetCommandList(), m_TopLevel.m_ScratchBuffer.Get(), m_TopLevel.m_ResultBuffer.Get(), m_TopLevel.m_InstanceDescsBuffer.Get());

}
