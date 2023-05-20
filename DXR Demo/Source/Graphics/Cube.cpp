#include "../Core/DeviceContext.hpp"
#include "Cube.hpp"

Cube::~Cube()
{
	Release();
}

void Cube::Create(DeviceContext* pDevice)
{
	assert(m_Device = pDevice);

	std::vector<CubeVertex> m_Vertices{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) }
	};

	std::vector<uint32_t> m_Indices{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

	// Create Buffers
	{
		m_VertexBuffer.Buffer.Create(m_Device, {}, { m_Vertices.data(), static_cast<uint32_t>(m_Vertices.size()), sizeof(m_Vertices.at(0)) * m_Vertices.size() });
		m_VertexBuffer.BufferView.Set(&m_VertexBuffer.Buffer);
	}
	{
		m_IndexBuffer.Buffer.Create(m_Device, {}, { m_Indices.data(), static_cast<uint32_t>(m_Indices.size()), m_Indices.size() * sizeof(uint32_t)});
		m_IndexBuffer.BufferView.Set(&m_IndexBuffer.Buffer);
	}

	//m_VertexBuffer.Create(pDevice, m_Vertices);
	//m_IndexBuffer.Create(pDevice->GetDevice(), m_Indices);
	m_ConstBuffer.Create(pDevice, &m_cbData);
}

void Cube::Draw()
{
	//m_Device->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//m_Device->GetCommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer.GetBufferView());
	//m_Device->GetCommandList()->IASetIndexBuffer(&m_IndexBuffer.GetBufferView());
	//m_Device->GetCommandList()->DrawIndexedInstanced(m_IndexBuffer.GetSize(), 1, 0, 0, 0);
}

void Cube::Draw(DirectX::XMMATRIX ViewProjection)
{
	m_Device->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_Device->GetCommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer.BufferView.BufferView);
	m_Device->GetCommandList()->IASetIndexBuffer(&m_IndexBuffer.BufferView.BufferView);

	auto frameIndex{ m_Device->FRAME_INDEX };
	m_ConstBuffer.Update({ XMMatrixTranspose(DirectX::XMMatrixIdentity() * ViewProjection), DirectX::XMMatrixIdentity() }, frameIndex);
	m_Device->GetCommandList()->SetGraphicsRootConstantBufferView(0, m_ConstBuffer.GetBuffer(frameIndex)->GetGPUVirtualAddress());

	m_Device->GetCommandList()->DrawIndexedInstanced(m_IndexBuffer.BufferView.Count, 1, 0, 0, 0);
}

void Cube::Release()
{
	//delete m_Vertices;
	//delete m_Indices;
}
