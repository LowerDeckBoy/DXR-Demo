#include "../../Core/DeviceContext.hpp"
#include "Buffer.hpp"
#include "../../Utilities/Utilities.hpp"


Buffer::~Buffer()
{
	//Release();
}

void Buffer::Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc, BufferType TypeOf)
{
	m_BufferData = Data;

	const auto heapDesc{ CD3DX12_RESOURCE_DESC::Buffer(Data.Size) };

	D3D12_RESOURCE_DESC desc{};
	const auto heapProps{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
	ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &heapDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_Buffer.ReleaseAndGetAddressOf())));

	MapMemory();

	pDevice->GetMainHeap()->Allocate(m_Descriptor);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = Data.ElementsCount;
	srvDesc.Buffer.StructureByteStride = Data.Stride;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	pDevice->GetDevice()->CreateShaderResourceView(m_Buffer.Get(), &srvDesc, m_Descriptor.GetCPU());

}

void Buffer::MapMemory()
{
	uint8_t* pDataBegin{ nullptr };
	const CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_Buffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
	std::memcpy(pDataBegin, m_BufferData.pData, m_BufferData.Size);
	m_Buffer.Get()->Unmap(0, nullptr);
}

void Buffer::Release()
{
	SAFE_RELEASE(m_Buffer);
}

VertexBuffer::VertexBuffer(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
{
	Buffer::Create(pDevice, Data, Desc, BufferType::eVertex);
	SetView();
}

VertexBuffer::~VertexBuffer()
{
	Buffer::Release();
}

void VertexBuffer::Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
{
	Buffer::Create(pDevice, Data, Desc, BufferType::eVertex);
	SetView();
}

void VertexBuffer::SetView()
{
	View.BufferLocation = Buffer::GetGPUAddress();
	View.SizeInBytes = static_cast<uint32_t>(Buffer::GetData().Size);
	View.StrideInBytes = static_cast<uint32_t>(Buffer::GetData().Size) / Buffer::GetData().ElementsCount;
}

IndexBuffer::IndexBuffer(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
{
	Buffer::Create(pDevice, Data, Desc, BufferType::eIndex);
	SetView();
}

IndexBuffer::~IndexBuffer()
{
	Buffer::Release();
}

void IndexBuffer::Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
{
	Buffer::Create(pDevice, Data, Desc, BufferType::eIndex);
	SetView();
}

bool IndexBuffer::IsR32bits()
{
	return Buffer::GetData().ElementsCount > UINT16_MAX;
}

void IndexBuffer::SetView()
{
	View.BufferLocation = Buffer::GetGPUAddress();
	View.Format = DXGI_FORMAT_R32_UINT;
	View.SizeInBytes = static_cast<uint32_t>(Buffer::GetData().Size);
	Count = Buffer::GetData().ElementsCount;
}
