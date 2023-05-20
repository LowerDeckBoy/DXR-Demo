#include "AccelerationStructures.hpp"
#include "../Graphics/Buffer/Vertex.hpp"

void AccelerationStructures::Create()
{
}

void AccelerationStructures::CreateBottomLevel()
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

	geometryDesc.Triangles.IndexBuffer	= m_IndexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount	= 3;
	geometryDesc.Triangles.IndexFormat	= DXGI_FORMAT_R32_UINT;

	geometryDesc.Triangles.VertexBuffer.StartAddress = m_VertexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(SimpleVertex);
	geometryDesc.Triangles.VertexCount = 3;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

}

void AccelerationStructures::CreateTopLevel()
{
}
