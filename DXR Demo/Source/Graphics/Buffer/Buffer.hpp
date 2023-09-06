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

enum class BufferType : uint8_t
{
	eVertex = 0,
	eIndex,
	eConstant,
	eStructured
};

class Buffer
{
public:
	~Buffer();

	void Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc, BufferType TypeOf);
	
	void MapMemory();
	
	void Release();

	inline ID3D12Resource* GetBuffer() const noexcept { return m_Buffer.Get(); }

	inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return m_Buffer.Get()->GetGPUVirtualAddress(); }

	inline BufferDesc GetDesc() const noexcept { return m_BufferDesc; }
	inline BufferData GetData() const noexcept { return m_BufferData; }

	Descriptor m_Descriptor{};

protected:
	DeviceContext* m_DeviceCtx{ nullptr };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer;
	//D3D12MA::Allocation* m_Allocation{ nullptr };

	BufferDesc m_BufferDesc{};
	BufferData m_BufferData{};
};

class VertexBuffer : public Buffer
{
public:
	VertexBuffer() noexcept {}
	VertexBuffer(DeviceContext* pDevice, BufferData Data, BufferDesc Desc);
	~VertexBuffer();
	
	void Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc);
	
	D3D12_VERTEX_BUFFER_VIEW View{};

private:
	void SetView();
};

class IndexBuffer : public Buffer
{
public:
	IndexBuffer() noexcept {}
	IndexBuffer(DeviceContext* pDevice, BufferData Data, BufferDesc Desc);
	~IndexBuffer();
	
	void Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc);
	
	// TODO:
	// Check whether indices count is greater than 16bits
	// if so, use DXGI_FORMAT_R16_UINT format
	bool IsR32bits();

	D3D12_INDEX_BUFFER_VIEW View{};
	uint32_t Count{ 0 };

private:
	void SetView();
	
};
