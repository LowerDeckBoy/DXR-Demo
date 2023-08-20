#include "Buffer/Buffer.hpp"
#include "Plane.hpp"


Plane::Plane(DeviceContext* pDevice)
{
	Create(pDevice);
}

Plane::~Plane()
{
}

void Plane::Create(DeviceContext* pDevice)
{
	m_DeviceCtx = pDevice;
	
	std::array<VertexNormal, 4> vertices = {
		VertexNormal{ { -8.0f, -1.0f, -8.0f }, { 0.0f, 1.0f, 0.0f }},
		VertexNormal{ { +8.0f, -1.0f, -8.0f }, { 0.0f, 1.0f, 0.0f }},
		VertexNormal{ { +8.0f, -1.0f, +8.0f }, { 0.0f, 1.0f, 0.0f }},
		VertexNormal{ { -8.0f, -1.0f, +8.0f }, { 0.0f, 1.0f, 0.0f }}
	};

	std::array<uint32_t, 6> indices = {
		0, 1, 2,
		2, 3, 0
	};

	m_VertexBuffer.Create(m_DeviceCtx, BufferData(vertices.data(), vertices.size(), sizeof(vertices.at(0)) * vertices.size(), sizeof(vertices.at(0))), BufferDesc());
	m_IndexBuffer.Create(m_DeviceCtx, BufferData(indices.data(), indices.size(), sizeof(uint32_t) * indices.size(), sizeof(uint32_t)), BufferDesc());
	m_ConstBuffer.Create(m_DeviceCtx, &m_cbData);
	
}

void Plane::Draw(const DirectX::XMMATRIX& ViewProjection)
{
	m_DeviceCtx->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_DeviceCtx->GetCommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer.View);
	m_DeviceCtx->GetCommandList()->IASetIndexBuffer(&m_IndexBuffer.View);

	const auto frameIndex{ m_DeviceCtx->FRAME_INDEX };
	m_ConstBuffer.Update({ XMMatrixTranspose(DirectX::XMMatrixIdentity() * ViewProjection), DirectX::XMMatrixIdentity() }, frameIndex);
	m_DeviceCtx->GetCommandList()->SetGraphicsRootConstantBufferView(0, m_ConstBuffer.GetBuffer(frameIndex)->GetGPUVirtualAddress());

	m_DeviceCtx->GetCommandList()->DrawIndexedInstanced(m_IndexBuffer.Count, 1, 0, 0, 0);
}
