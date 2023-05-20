#pragma once
#include "Vertex.hpp"
#include "../../Utilities/Utilities.hpp"
#include "../../Core/DeviceContext.hpp"


struct BufferData
{
	void*	 pData			{ nullptr };
	uint32_t ElementsCount	{ 0 };
	size_t	 Size			{ 0 };
};

// Default properties
// Properties:	Upload
// Flags:		None
// State:		Common
// Format:		Unknown
struct BufferDesc
{
	CD3DX12_HEAP_PROPERTIES HeapProperties	{ D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_FLAGS		HeapFlags		{ D3D12_HEAP_FLAG_NONE };
	D3D12_RESOURCE_STATES	State			{ D3D12_RESOURCE_STATE_COMMON };
	DXGI_FORMAT				Format			{ DXGI_FORMAT_UNKNOWN };
};

// TODO: Make general Buffer class as a Base for different buffer types
class Buffer
{
public:
	void Create(DeviceContext* pDevice, BufferDesc Desc, BufferData Data)
	{
		m_DeviceCtx = pDevice;
		m_BufferDesc = Desc;
		m_BufferData = Data;

		auto heapDesc{ CD3DX12_RESOURCE_DESC::Buffer(Data.Size) };
		//auto 
		ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(
			&Desc.HeapProperties, 
			Desc.HeapFlags, 
			&heapDesc, 
			Desc.State, 
			nullptr, 
			IID_PPV_ARGS(m_Buffer.GetAddressOf())));

		MapMemory();

		pDevice->GetMainHeap()->Allocate(m_Descriptor);
	}

	void MapMemory()
	{
		uint8_t* pDataBegin{};
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_Buffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
		std::memcpy(pDataBegin, m_BufferData.pData, m_BufferData.Size);
		m_Buffer.Get()->Unmap(0, nullptr);
	}

	ID3D12Resource* GetBuffer() const
	{
		return m_Buffer.Get();
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
	{
		return m_Buffer.Get()->GetGPUVirtualAddress();
	}

	BufferDesc GetDesc() { return m_BufferDesc; }
	BufferData GetData() { return m_BufferData; }

	Descriptor m_Descriptor{};
	Descriptor DescriptorSRV{};

protected:
	DeviceContext* m_DeviceCtx{ nullptr };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_BufferUploadHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer;

	BufferDesc m_BufferDesc{};
	BufferData m_BufferData{};
};

struct VertexView
{
	void Set(Buffer* pBuffer)
	{
		BufferView.BufferLocation = pBuffer->GetGPUAddress();
		BufferView.SizeInBytes = pBuffer->GetData().Size;
		BufferView.StrideInBytes = static_cast<uint32_t>(pBuffer->GetData().Size) / pBuffer->GetData().ElementsCount;
	}

	D3D12_VERTEX_BUFFER_VIEW BufferView{};
};

struct IndexView
{
	void Set(Buffer* pBuffer)
	{
		BufferView.BufferLocation = pBuffer->GetGPUAddress();
		BufferView.Format = DXGI_FORMAT_R32_UINT;
		BufferView.SizeInBytes = pBuffer->GetData().Size;

		Count = pBuffer->GetData().ElementsCount;
	}

	D3D12_INDEX_BUFFER_VIEW BufferView{};
	uint32_t Count{ 0 };
};

struct VertexBuffer
{
	Buffer		Buffer;
	VertexView	BufferView;
};

struct IndexBuffer
{
	Buffer		Buffer;
	IndexView	BufferView;
};

// TEST
struct BufferSRV
{
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	Descriptor DescriptorSRV;
};

struct BufferUAV
{

};

// Utils
class BufferUtils
{
public:
	static ID3D12Resource* Create(ID3D12Device5* pDevice, const uint64_t Size, const D3D12_RESOURCE_FLAGS Flags, const D3D12_RESOURCE_STATES InitState, const D3D12_HEAP_PROPERTIES& HeapProps, D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE)
	{

		D3D12_RESOURCE_DESC desc{ };
		desc.Flags = Flags;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.MipLevels = 1;
		desc.DepthOrArraySize = 1;
		desc.Height = 1;
		desc.Width = Size;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.SampleDesc = { 1, 0 };

		ID3D12Resource* buffer{ nullptr };

		ThrowIfFailed(pDevice->CreateCommittedResource(&HeapProps, HeapFlags, &desc, InitState, nullptr, IID_PPV_ARGS(&buffer)));

		return buffer;
	}

	static void CreateUAV(DeviceContext* pDevice, ID3D12Resource** ppTargetResource, size_t BufferSize, D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON)
	{
		auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			InitialState,
			nullptr,
			IID_PPV_ARGS(ppTargetResource)));
	}

	// For Shaders
	/*
	static void CreateBufferSRV(DeviceContext* pDevice, Descriptor& DescriptorRef, ID3D12Resource* pBuffer, uint32_t BufferElements, uint32_t BufferElementSize)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = BufferElements;
		if (BufferElementSize == 0)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.StructureByteStride = 0;
		}
		else
		{
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.StructureByteStride = BufferElementSize;
		}
		
		pDevice->GetMainHeap()->Allocate(DescriptorRef);
		pDevice->GetDevice()->CreateShaderResourceView(pBuffer, &srvDesc, DescriptorRef.GetCPU());
	}*/
	static void CreateBufferSRV(DeviceContext* pDevice, Descriptor& DescriptorRef, ID3D12Resource* pBuffer, uint32_t BufferElements, uint32_t BufferElementSize)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = BufferElements;
		if (BufferElementSize == 0)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.StructureByteStride = 0;
		}
		else
		{
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.StructureByteStride = BufferElementSize;
		}

		pDevice->GetMainHeap()->Allocate(DescriptorRef); 
		try {
			pDevice->GetDevice()->CreateShaderResourceView(pBuffer, &srvDesc, DescriptorRef.GetCPU());
		}
		catch (...)
		{
			throw std::exception();
		}
	}

	// !TEST
	/*
	static void CreateSRV(DeviceContext* pDevice, Buffer* pBuffer, BufferSRV& TargetBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = pBuffer->GetData().ElementsCount;

		if (pBuffer->GetData().ElementsCount == 0)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.StructureByteStride = 0;
		}
		else
		{
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.StructureByteStride = pBuffer->GetData().ElementsCount;
		}

		pDevice->GetMainHeap()->Allocate(TargetBuffer.DescriptorSRV);
		pDevice->GetDevice()->CreateShaderResourceView(TargetBuffer.Resource.Get(), &srvDesc, TargetBuffer.DescriptorSRV.GetCPU());
	}
	*/
	
	static void CreateSRV(DeviceContext* pDevice, Buffer* pBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = pBuffer->GetData().ElementsCount;

		if (pBuffer->GetData().ElementsCount == 0)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.StructureByteStride = 0;
		}
		else
		{
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.StructureByteStride = pBuffer->GetData().ElementsCount;
		}

		pDevice->GetMainHeap()->Allocate(pBuffer->DescriptorSRV);
		pDevice->GetDevice()->CreateShaderResourceView(pBuffer->GetBuffer(), &srvDesc, pBuffer->DescriptorSRV.GetCPU());
	}
	static void UploadBuffer(DeviceContext* pDevice, ID3D12Resource** ppTargetResource, void* pData, size_t DataSize)
	{
		auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(DataSize);
		ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(ppTargetResource)));
	
		void* pMappedData;
		ThrowIfFailed((*ppTargetResource)->Map(0, nullptr, &pMappedData));
		std::memcpy(pMappedData, pData, DataSize);
		(*ppTargetResource)->Unmap(0, nullptr);
	}

	static void Allocate(ID3D12Device5* pDevice, ID3D12Resource* pResource, uint32_t BufferSize)
	{
		auto uploadHeap{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
		auto bufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(BufferSize) };

		ThrowIfFailed(pDevice->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&pResource)));
	}

	static ID3D12Resource* Allocate(ID3D12Device5* pDevice, uint32_t BufferSize)
	{
		auto uploadHeap{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
		auto bufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(BufferSize) };

		ID3D12Resource* resource{ nullptr };
		ThrowIfFailed(pDevice->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&resource)));

		return resource;
	}

	// Write only
	static uint8_t* MapCPU(ID3D12Resource* pResource)
	{
		uint8_t* pMapped{ nullptr };
		CD3DX12_RANGE range(0, 0);
		ThrowIfFailed(pResource->Map(0, &range, reinterpret_cast<void**>(&pMapped)));

		return pMapped;
	}

};
