#include "../Core/DeviceContext.hpp"
#include "Buffer/Buffer.hpp"
#include "Buffer/ConstantBuffer.hpp"
#include "Cube.hpp"

Cube::~Cube()
{
	Release();
}

void Cube::Create(DeviceContext* pDevice)
{
	assert(m_Device = pDevice);

	m_VertexBuffer.Create(pDevice->GetDevice(), m_Vertices);
	m_IndexBuffer.Create(pDevice->GetDevice(), m_Indices);
	m_ConstBuffer.Create(pDevice, &m_cbData);
}

void Cube::Draw()
{
	m_Device->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_Device->GetCommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer.GetBufferView());
	m_Device->GetCommandList()->IASetIndexBuffer(&m_IndexBuffer.GetBufferView());
	m_Device->GetCommandList()->DrawIndexedInstanced(m_IndexBuffer.GetSize(), 1, 0, 0, 0);
}

void Cube::Draw(DirectX::XMMATRIX ViewProjection)
{
	m_Device->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_Device->GetCommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer.GetBufferView());
	m_Device->GetCommandList()->IASetIndexBuffer(&m_IndexBuffer.GetBufferView());

	auto frameIndex{ m_Device->FRAME_INDEX };
	m_ConstBuffer.Update({ XMMatrixTranspose(DirectX::XMMatrixIdentity() * ViewProjection), DirectX::XMMatrixIdentity() }, frameIndex);
	m_Device->GetCommandList()->SetGraphicsRootConstantBufferView(0, m_ConstBuffer.GetBuffer(frameIndex)->GetGPUVirtualAddress());

	m_Device->GetCommandList()->DrawIndexedInstanced(m_IndexBuffer.GetSize(), 1, 0, 0, 0);
}

void Cube::Release()
{
	//delete m_Vertices;
	//delete m_Indices;
}
