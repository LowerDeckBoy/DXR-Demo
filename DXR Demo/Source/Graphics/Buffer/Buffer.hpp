#pragma once
#include "Vertex.hpp"
#include "../../Core/DeviceContext.hpp"

struct BufferData
{
	BufferData() noexcept {}
	BufferData(void* pData, size_t Count, size_t Size, size_t Stride) noexcept :
		pData(pData), ElementsCount(static_cast<uint32_t>(Count)), Size(Size), Stride(static_cast<uint32_t>(Stride))
	{ }
	
	void*	 pData{ nullptr };
	uint32_t ElementsCount{ 0 };
	size_t	 Size{ 0 };
	uint32_t Stride{ 0 };
};

// Default properties
// Properties:	Upload
// Flags:		None
// State:		Common
// Format:		Unknown
struct BufferDesc
{
	CD3DX12_HEAP_PROPERTIES HeapProperties{ D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_FLAGS		HeapFlags{ D3D12_HEAP_FLAG_NONE };
	D3D12_RESOURCE_STATES	State{ D3D12_RESOURCE_STATE_GENERIC_READ };
	DXGI_FORMAT				Format{ DXGI_FORMAT_UNKNOWN };
};

class Buffer
{
public:
	void Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc);
	
	void MapMemory();
	
	inline ID3D12Resource* GetBuffer() const noexcept { return m_Buffer.Get(); }

	inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return m_Buffer.Get()->GetGPUVirtualAddress(); }

	inline BufferDesc GetDesc() const noexcept { return m_BufferDesc; }
	inline BufferData GetData() const noexcept { return m_BufferData; }

	Descriptor m_Descriptor{};

protected:
	DeviceContext* m_DeviceCtx{ nullptr };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer;

	BufferDesc m_BufferDesc{};
	BufferData m_BufferData{};
};

class VertexBuffer
{
public:
	VertexBuffer() noexcept {}
	VertexBuffer(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
	{
		Buffer.Create(pDevice, Data, Desc);
		SetView();
	}

	void Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
	{
		Buffer.Create(pDevice, Data, Desc);
		SetView();
	}

	Buffer Buffer;
	D3D12_VERTEX_BUFFER_VIEW View{};

	void SetView()
	{
		View.BufferLocation = this->Buffer.GetGPUAddress();
		View.SizeInBytes = static_cast<uint32_t>(this->Buffer.GetData().Size);
		View.StrideInBytes = static_cast<uint32_t>(this->Buffer.GetData().Size) / this->Buffer.GetData().ElementsCount;
	}
};

class IndexBuffer
{
public:
	IndexBuffer() noexcept {}
	IndexBuffer(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
	{
		Buffer.Create(pDevice, Data, Desc);
		SetView();
	}

	void Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
	{
		Buffer.Create(pDevice, Data, Desc);
		SetView();
	}

	Buffer Buffer;
	D3D12_INDEX_BUFFER_VIEW View{};
	uint32_t Count{ 0 };

	void SetView()
	{
		View.BufferLocation = this->Buffer.GetGPUAddress();
		View.Format = DXGI_FORMAT_R32_UINT;
		View.SizeInBytes = static_cast<uint32_t>(this->Buffer.GetData().Size);
		Count = this->Buffer.GetData().ElementsCount;
	}
};
