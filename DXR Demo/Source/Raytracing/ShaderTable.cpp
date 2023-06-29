#include "ShaderTable.hpp"
#include "../Utilities/Utilities.hpp"


ShaderTableBuilder::ShaderTableBuilder()
{
}

void ShaderTableBuilder::Create(ID3D12Device5* pDevice, ID3D12StateObjectProperties* pRaytracingPipeline, ID3D12Resource* pTableStorageBuffer)
{
	uint8_t* pData{ nullptr };
	ThrowIfFailed(pTableStorageBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

	uint32_t offset{ 0 };

	m_RayGenSize = GetShaderSize(*m_RayGen);
	offset = CopyShaderData(pRaytracingPipeline, pData, *m_RayGen, m_RayGenSize);
	pData += offset;

	m_MissSize = GetShaderSize(*m_Miss);
	offset = CopyShaderData(pRaytracingPipeline, pData, *m_Miss, m_MissSize);
	pData += offset;

	m_HitGroupSize = GetShaderSize(*m_HitGroup);
	offset = CopyShaderData(pRaytracingPipeline, pData, *m_HitGroup, m_HitGroupSize);

	pTableStorageBuffer->Unmap(0, nullptr);
}

void ShaderTableBuilder::Reset()
{
	m_RayGen	 = nullptr;
	m_Miss		 = nullptr;
	m_ClosestHit = nullptr;

	
}

void ShaderTableBuilder::AddRayGenShader(std::wstring Entrypoint, std::vector<void*> pInputData)
{
	m_RayGen = new RaytraceShader(Entrypoint, pInputData);
}

void ShaderTableBuilder::AddMissShader(std::wstring Entrypoint, std::vector<void*> pInputData)
{
	m_Miss = new RaytraceShader(Entrypoint, pInputData);
}

void ShaderTableBuilder::AddHitGroup(std::wstring GroupName, std::vector<void*> pInputData)
{
	m_HitGroup = new RaytraceShader(GroupName, pInputData);
}

uint32_t ShaderTableBuilder::GetTableSize()
{
	m_IdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	
	// Currently only one entry per shader so no need to multiply by vector sizes
	m_RayGenSize	= GetShaderSize(*m_RayGen);
	m_MissSize		= GetShaderSize(*m_Miss);
	m_HitGroupSize	= GetShaderSize(*m_HitGroup);

	// 256 bytes alignment
	return ALIGN(m_RayGenSize + m_MissSize + m_HitGroupSize, 256);
}

uint32_t ShaderTableBuilder::GetRayGenSize()
{
	return m_RayGenSize;
}

uint32_t ShaderTableBuilder::GetMissSize()
{
	return m_MissSize;
}

uint32_t ShaderTableBuilder::GetHitGroup()
{
	return m_HitGroupSize;
}

uint32_t ShaderTableBuilder::GetShaderSize(RaytraceShader& Shader)
{
	size_t sizeOfArgs{ 0 };
	sizeOfArgs = std::max(sizeOfArgs, Shader.InputData.size());

	uint32_t shaderSize{ m_IdentifierSize + 8 * static_cast<uint32_t>(sizeOfArgs) };

	return ALIGN(shaderSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
}

uint32_t ShaderTableBuilder::CopyShaderData(ID3D12StateObjectProperties* pRaytracingPipeline, uint8_t* pOutputData, RaytraceShader& Shader, const uint32_t ShaderSize)
{
	uint8_t* pData = pOutputData;

	void* id = pRaytracingPipeline->GetShaderIdentifier(Shader.Name.c_str());
	if (!id)
	{
		::OutputDebugStringA("Failed to get ID\n");
		throw std::logic_error("");
	}
	// Copy the shader identifier
	std::memcpy(pData, id, m_IdentifierSize);
	// Copy all its resources pointers or values in bulk
	std::memcpy(pData + m_IdentifierSize, Shader.InputData.data(), Shader.InputData.size() * 8);

	pData += ShaderSize;

	return ShaderSize;
}
