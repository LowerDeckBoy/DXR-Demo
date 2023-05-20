#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>

#include "../Graphics/Buffer/Buffer.hpp"


inline uint32_t ALIGN(uint32_t Size, uint32_t Alignment)
{
	return (Size + (Alignment + 1)) & ~(Alignment - 1);
}

class ShaderRecord
{
public:
	ShaderRecord(void* pShaderIdentifier, uint32_t ShaderIdentifierSize)
		: m_ShaderIdentifier(pShaderIdentifier, ShaderIdentifierSize) { }

	ShaderRecord(void* pShaderIdentifier, uint32_t ShaderIdentifierSize, void* pLocalRootArguments, uint32_t LocalRootArgumentSize)
		: m_ShaderIdentifier(pShaderIdentifier, ShaderIdentifierSize), m_LocalRootArguments(pLocalRootArguments, LocalRootArgumentSize) { }

	void CopyTo(void* pDestination)
	{
		uint8_t* pByteDestination{ static_cast<uint8_t*>(pDestination) };
		std::memcpy(pByteDestination, m_ShaderIdentifier.ptr, m_ShaderIdentifier.size);
		if (m_LocalRootArguments.ptr)
		{
			std::memcpy(pByteDestination + m_ShaderIdentifier.size, m_LocalRootArguments.ptr, m_LocalRootArguments.size);
		}
	}

	struct PointerWithSize
	{
		PointerWithSize() {}
		PointerWithSize(void* Ptr, uint32_t Size)
			: ptr(Ptr), size(Size) {}

		void* ptr{ nullptr };
		uint32_t size{ 0 };
	};

	PointerWithSize m_ShaderIdentifier;
	PointerWithSize m_LocalRootArguments;
};


class ShaderTable
{
public:
	ShaderTable(ID3D12Device5* pDevice, uint32_t NumShaderRecord, uint32_t ShaderRecordSize)
	{
		m_ShaderRecordSize = ALIGN(ShaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		m_ShaderRecords.reserve(NumShaderRecord);

		uint32_t bufferSize{ NumShaderRecord * m_ShaderRecordSize };
		//Buffer::Allocate(pDevice, m_Resource.Get(), bufferSize);
		m_Resource = BufferUtils::Allocate(pDevice, bufferSize);

		m_MappedShaderRecord = BufferUtils::MapCPU(m_Resource.Get());
	}

	void AddRecord(ShaderRecord& NewRecord)
	{
		m_ShaderRecords.push_back(NewRecord);
		NewRecord.CopyTo(m_MappedShaderRecord);

		m_MappedShaderRecord += m_ShaderRecordSize;
	}

	ID3D12Resource* GetResource() const
	{
		return m_Resource.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;

	uint32_t m_ShaderRecordSize{};
	uint8_t* m_MappedShaderRecord{ nullptr };

	std::vector<ShaderRecord> m_ShaderRecords;

};