#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "../Graphics/Buffer/Buffer.hpp"

inline constexpr uint32_t ALIGN(uint32_t Size, uint32_t Alignment)
{
	return (Size + (Alignment + 1)) & ~(Alignment - 1);
}

class TableRecord
{
public:
	TableRecord(void* pIdentifier, uint32_t Size)
	{
		m_Identifier.pData = pIdentifier;
		m_Identifier.Size = Size;
	}

	TableRecord(void* pIdentifier, uint32_t Size, void* pLocalRootArgs, uint32_t ArgsCount)
	{
		m_Identifier.pData = pIdentifier;
		m_Identifier.Size = Size;
		m_LocalRootArgs.pData = pLocalRootArgs;
		m_LocalRootArgs.Size = ArgsCount;
	}

	void CopyTo(void* pDestination)
	{
		uint8_t* pByteDestination{ static_cast<uint8_t*>(pDestination) };
		//uint8_t* pByteDestination{ reinterpret_cast<uint8_t*>(pDestination) };
		std::memcpy(pByteDestination, m_Identifier.pData, m_Identifier.Size);
		if (m_LocalRootArgs.pData != nullptr)
			std::memcpy(pByteDestination + m_Identifier.Size, m_LocalRootArgs.pData, m_LocalRootArgs.Size);
	}

	struct Identifier
	{
		void* pData{ nullptr };
		uint32_t Size{ 0 };
	};

	Identifier m_Identifier;
	Identifier m_LocalRootArgs;

};

class ShaderTable
{
public:
	ShaderTable() { }

	void Create(ID3D12Device5* pDevice, uint32_t NumShaderRecord, uint32_t ShaderRecordSize, std::wstring DebugName = L"")
	{
		m_ShaderRecordSize = ALIGN(ShaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		m_Records.reserve(NumShaderRecord);

		uint32_t bufferSize{ NumShaderRecord * m_ShaderRecordSize };
		//Buffer::Allocate(pDevice, m_Resource.Get(), bufferSize);
		//m_Storage = BufferUtils::Allocate(pDevice, bufferSize);
		auto uploadHeap{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
		BufferUtils::Create(pDevice, &m_Storage, bufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, uploadHeap);
		m_MappedData = BufferUtils::MapCPU(m_Storage.Get());

		if (!DebugName.empty())
			SetTableName(DebugName);
	}

	void AddRecord(TableRecord& Record)
	{
		m_Records.emplace_back(Record);
		Record.CopyTo(m_MappedData);
		m_MappedData += m_ShaderRecordSize;
	}

	void SetTableName(std::wstring Name)
	{
		m_Storage.Get()->SetName(Name.c_str());
	}

	uint32_t GetShaderRecordSize() const { return m_ShaderRecordSize; }
	ID3D12Resource* GetStorage() const { return m_Storage.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_Storage;
	uint8_t* m_MappedData{ nullptr };
	uint32_t m_ShaderRecordSize;

	std::vector<TableRecord> m_Records;

};

// For single buffer usage
class ShaderTableBuilder
{
public:
	ShaderTableBuilder();

	void Create(ID3D12Device5* pDevice, ID3D12StateObjectProperties* pRaytracingPipeline, ID3D12Resource* pTableStorageBuffer);
	void Reset();

	void AddRayGenShader(std::wstring Entrypoint, std::vector<void*> pInputData);
	void AddMissShader(std::wstring Entrypoint, std::vector<void*> pInputData);
	void AddHitGroup(std::wstring GroupName, std::vector<void*> pInputData);

	// Get total size based on shaders
	uint32_t GetTableSize();

	uint32_t GetRayGenSize();
	uint32_t GetMissSize();
	uint32_t GetHitGroup();

private:

	struct RaytraceShader
	{
		RaytraceShader(std::wstring Entrypoint, std::vector<void*> pInputData)
			: Name(std::move(Entrypoint)), InputData(std::move(pInputData))
		{ }

		const std::wstring Name;
		std::vector<void*> InputData;
	};

	RaytraceShader* m_RayGen{ nullptr };
	uint32_t m_RayGenSize{};
	RaytraceShader* m_Miss{ nullptr };
	uint32_t m_MissSize{};
	RaytraceShader* m_ClosestHit{ nullptr };
	uint32_t m_HitGroupSize{};
	RaytraceShader* m_HitGroup{ nullptr };

	uint32_t GetShaderSize(RaytraceShader& Shader);
	uint32_t CopyShaderData(ID3D12StateObjectProperties* pRaytracingPipeline, uint8_t* pOutputData,	RaytraceShader& Shader, const uint32_t ShaderSize);

	uint32_t m_IdentifierSize{ D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };

};
