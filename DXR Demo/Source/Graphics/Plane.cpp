#include "Buffer/Buffer.hpp"
#include "Plane.hpp"

Plane::Plane()
{
}

Plane::~Plane()
{
}

void Plane::Create(DeviceContext* pDevice)
{
	m_DeviceCtx = pDevice;

	std::vector<SimpleVertex> vertices = {
		{ { -8.0f, -1.0f, -8.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
		{ { +8.0f, -1.0f, -8.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
		{ { +8.0f, -1.0f, +8.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
		{ { -8.0f, -1.0f, +8.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
	};

	std::vector<uint32_t> indices = {
		0, 1, 2,
		2, 3, 0
	};

	m_VertexBuffer.Create(m_DeviceCtx, BufferData(vertices.data(), vertices.size(), sizeof(vertices.at(0)) * vertices.size(), sizeof(SimpleVertex)), BufferDesc());
	m_IndexBuffer.Create(m_DeviceCtx, BufferData(indices.data(), indices.size(), sizeof(uint32_t) * indices.size(), sizeof(uint32_t)), BufferDesc());
	m_ConstBuffer.Create(m_DeviceCtx, &m_cbData);
	
}

void Plane::Draw(DirectX::XMMATRIX ViewProjection)
{
	m_DeviceCtx->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_DeviceCtx->GetCommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer.View);
	m_DeviceCtx->GetCommandList()->IASetIndexBuffer(&m_IndexBuffer.View);

	const auto frameIndex{ m_DeviceCtx->FRAME_INDEX };
	//auto inversed}
	m_ConstBuffer.Update({ XMMatrixTranspose(DirectX::XMMatrixIdentity() * ViewProjection), DirectX::XMMatrixIdentity() }, frameIndex);
	m_DeviceCtx->GetCommandList()->SetGraphicsRootConstantBufferView(0, m_ConstBuffer.GetBuffer(frameIndex)->GetGPUVirtualAddress());

	m_DeviceCtx->GetCommandList()->DrawIndexedInstanced(m_IndexBuffer.Count, 1, 0, 0, 0);
}
