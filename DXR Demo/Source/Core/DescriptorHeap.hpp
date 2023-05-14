#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

class Descriptor
{
private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle{};

public:
	void SetCPU(D3D12_CPU_DESCRIPTOR_HANDLE Handle) { m_cpuHandle = Handle; }
	void SetGPU(D3D12_GPU_DESCRIPTOR_HANDLE Handle) { m_gpuHandle = Handle; }

	[[nodiscard]] inline D3D12_CPU_DESCRIPTOR_HANDLE GetCPU() const { return m_cpuHandle; }
	[[nodiscard]] inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPU() const { return m_gpuHandle; }

};

class DescriptorHeap
{
public:
	void Allocate(Descriptor& targetDescriptor)
	{
		targetDescriptor.SetCPU(GetCPUptr(m_Allocated));
		targetDescriptor.SetGPU(GetGPUptr(m_Allocated));
		m_Allocated++;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUptr(uint32_t Index)
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE(m_Heap.Get()->GetCPUDescriptorHandleForHeapStart().ptr + Index * m_DescriptorSize);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUptr(uint32_t Index)
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE(m_Heap.Get()->GetGPUDescriptorHandleForHeapStart().ptr + Index * m_DescriptorSize);
	}

	inline ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }
	inline ID3D12DescriptorHeap** GetHeapAddressOf() { return m_Heap.GetAddressOf(); }

	[[maybe_unused]] inline uint32_t GetDescriptorSize() const { return m_DescriptorSize; }
	[[maybe_unused]] inline uint32_t GetDescriptorsCount() const { return m_NumDescriptors; }
	[[maybe_unused]] inline uint32_t GetAllocatedCount() const { return m_Allocated; }

	void SetDescriptorSize(const uint32_t NewSize)
	{
		m_DescriptorSize = NewSize;
	}
	void SetDescriptorsCount(const uint32_t Count)
	{
		m_NumDescriptors = Count;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
	//D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	uint32_t m_DescriptorSize{ 32 };
	uint32_t m_NumDescriptors{ 0 };
	uint32_t m_Allocated{ 0 };
};
