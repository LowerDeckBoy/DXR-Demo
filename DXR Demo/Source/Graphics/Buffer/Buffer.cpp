#include "../../Core/DeviceContext.hpp"
#include "Buffer.hpp"
#include "../../Utilities/Utilities.hpp"


void Buffer::Create(DeviceContext* pDevice, BufferData Data, BufferDesc Desc)
{
	m_BufferData = Data;

	const auto heapDesc{ CD3DX12_RESOURCE_DESC::Buffer(Data.Size) };

	D3D12MA::Allocation* allocationDefault{ nullptr };
	D3D12MA::ALLOCATION_DESC allocDesc{};
	allocDesc.Flags = D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_COMMITTED | D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY;
	allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	pDevice->GetAllocator()->CreateResource(&allocDesc, &heapDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, &allocationDefault, IID_PPV_ARGS(m_Buffer.ReleaseAndGetAddressOf()));

	allocDesc.Flags = D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_COMMITTED | D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY;
	allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	D3D12MA::Allocation* allocationUpload{ nullptr };
	ID3D12Resource* uploadBuffer{ nullptr };
	pDevice->GetAllocator()->CreateResource(&allocDesc, &heapDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &allocationUpload, IID_PPV_ARGS(&uploadBuffer));

	D3D12_SUBRESOURCE_DATA subresource{};
	subresource.pData = Data.pData;
	subresource.RowPitch = Data.Size;
	subresource.SlicePitch = Data.Size;
	UpdateSubresources(pDevice->GetCommandList(), m_Buffer.Get(), uploadBuffer, 0, 0, 1, &subresource);

	auto barrier{ CD3DX12_RESOURCE_BARRIER::Transition(m_Buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON) };
	pDevice->GetCommandList()->ResourceBarrier(1, &barrier);

	//MapMemory();

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

	//allocation->Release();
	allocationUpload->Release();
	allocationDefault->Release();
	uploadBuffer = nullptr;
}
/*
void Buffer::MapMemory()
{
	uint8_t* pDataBegin{};
	const CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_Buffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
	std::memcpy(pDataBegin, m_BufferData.pData, m_BufferData.Size);
	m_Buffer.Get()->Unmap(0, nullptr);
}
*/
