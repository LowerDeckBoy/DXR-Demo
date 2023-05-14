#pragma once
#include "Vertex.hpp"
#include "../../Utilities/Utilities.hpp"
#include "../../Core/DeviceContext.hpp"

class Buffer
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

};

template<typename T>
class VertexBuffer
{
public:
	VertexBuffer() = default;
	VertexBuffer(ID3D12Device5* pDevice, std::vector<T>& pData) { Create(pDevice, pData); }

	void Create(ID3D12Device5* pDevice, std::vector<T>& pData)
	{
		size_t bufferSize{ sizeof(T) * pData.size() };
		auto heapDesc{ CD3DX12_RESOURCE_DESC::Buffer(bufferSize) };

		auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
		ThrowIfFailed(pDevice->CreateCommittedResource(&heapProperties,
						D3D12_HEAP_FLAG_NONE, &heapDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(Buffer.GetAddressOf())));
		
		uint8_t* pDataBegin{};
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(Buffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
		std::memcpy(pDataBegin, pData.data(), bufferSize);
		Buffer.Get()->Unmap(0, nullptr);

		BufferView.BufferLocation	= Buffer.Get()->GetGPUVirtualAddress();
		BufferView.SizeInBytes		= static_cast<uint32_t>(bufferSize);
		BufferView.StrideInBytes	= static_cast<uint32_t>(sizeof(T));

	}
	/*
	static ID3D12Resource* Create(ID3D12Device5* pDevice, const uint64_t Size, const D3D12_RESOURCE_FLAGS Flags, const D3D12_RESOURCE_STATES InitState,
		const D3D12_HEAP_PROPERTIES& HeapProps)
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
		ThrowIfFailed(pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &desc, Flags, nullptr, IID_PPV_ARGS(&buffer)));

		return buffer;
	}
	*/

	[[nodiscard]]
	inline ID3D12Resource* GetBuffer() const { return Buffer.Get(); }

	[[nodiscard]]
	inline D3D12_VERTEX_BUFFER_VIEW& GetBufferView() { return BufferView; }


private:
	Microsoft::WRL::ComPtr<ID3D12Resource> BufferUploadHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> Buffer;
	D3D12_VERTEX_BUFFER_VIEW BufferView{};

};

class IndexBuffer
{
public:
	IndexBuffer() = default;
	IndexBuffer(ID3D12Device5* pDevice, std::vector<uint32_t>& pData) { Create(pDevice, pData); }

	void Create(ID3D12Device5* pDevice, std::vector<uint32_t>& pData)
	{
		// indices count
		BufferSize = static_cast<uint32_t>(pData.size());

		size_t bufferSize = sizeof(uint32_t) * pData.size();
		auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)};
		auto heapDesc{ CD3DX12_RESOURCE_DESC::Buffer(bufferSize) };
		ThrowIfFailed(pDevice->CreateCommittedResource(&heapProperties,
					  D3D12_HEAP_FLAG_NONE,
					  &heapDesc,
					  D3D12_RESOURCE_STATE_GENERIC_READ,
					  nullptr,
					  IID_PPV_ARGS(&Buffer)));

		uint8_t* pDataBegin{};
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(Buffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
		memcpy(pDataBegin, pData.data(), bufferSize);
		Buffer.Get()->Unmap(0, nullptr);

		BufferView.BufferLocation = Buffer->GetGPUVirtualAddress();
		BufferView.Format = DXGI_FORMAT_R32_UINT;
		BufferView.SizeInBytes = static_cast<uint32_t>(bufferSize);
	}


	[[nodiscard]]
	inline ID3D12Resource* GetBuffer() const { return Buffer.Get(); }

	[[nodiscard]]
	inline D3D12_INDEX_BUFFER_VIEW& GetBufferView() { return BufferView; }

	[[nodiscard]]
	inline uint32_t GetSize() const { return BufferSize; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> Buffer;
	D3D12_INDEX_BUFFER_VIEW BufferView;
	uint32_t BufferSize{};
};