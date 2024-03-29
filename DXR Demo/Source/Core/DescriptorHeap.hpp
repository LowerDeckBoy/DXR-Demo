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
	void SetCPU(D3D12_CPU_DESCRIPTOR_HANDLE Handle) noexcept { m_cpuHandle = Handle; }
	void SetGPU(D3D12_GPU_DESCRIPTOR_HANDLE Handle) noexcept { m_gpuHandle = Handle; }

	[[nodiscard]] inline D3D12_CPU_DESCRIPTOR_HANDLE GetCPU() const noexcept { return m_cpuHandle; }
	[[nodiscard]] inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPU() const noexcept { return m_gpuHandle; }

	uint32_t Index{ 0 };

	inline bool IsValid() {
		return m_cpuHandle.ptr != 0;
	}

};

class DescriptorHeap
{
public:
	void Allocate(Descriptor& TargetDescriptor)
	{
		if (TargetDescriptor.IsValid())
		{
			Override(TargetDescriptor);
		}
		else
		{
			TargetDescriptor.SetCPU(GetCPUptr(m_Allocated));
			TargetDescriptor.SetGPU(GetGPUptr(m_Allocated));
			TargetDescriptor.Index = m_Allocated;
			m_Allocated++;
		}
	}

	void Override(Descriptor& TargetDescriptor)
	{
		TargetDescriptor.SetCPU(GetCPUptr(TargetDescriptor.Index));
		TargetDescriptor.SetGPU(GetGPUptr(TargetDescriptor.Index));
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUptr(uint32_t Index)
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE(m_Heap.Get()->GetCPUDescriptorHandleForHeapStart().ptr + Index * m_DescriptorSize);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUptr(uint32_t Index)
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE(m_Heap.Get()->GetGPUDescriptorHandleForHeapStart().ptr + Index * m_DescriptorSize);
	}

	inline ID3D12DescriptorHeap* GetHeap() noexcept { return m_Heap.Get(); }
	inline ID3D12DescriptorHeap** GetHeapAddressOf() noexcept { return m_Heap.GetAddressOf(); }

	[[maybe_unused]] inline uint32_t GetDescriptorSize() const noexcept { return m_DescriptorSize; }
	[[maybe_unused]] inline uint32_t GetDescriptorsCount() const noexcept { return m_NumDescriptors; }
	[[maybe_unused]] inline uint32_t GetAllocatedCount() const noexcept { return m_Allocated; }

	void SetDescriptorSize(const uint32_t NewSize) noexcept
	{
		m_DescriptorSize = NewSize;
	}
	void SetDescriptorsCount(const uint32_t Count) noexcept
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
