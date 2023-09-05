#include "../Core/DeviceContext.hpp"
#include "Cube.hpp"

Cube::~Cube()
{
	Release();
}

Cube::Cube(DeviceContext* pDevice)
{
	Create(pDevice);
}

void Cube::Create(DeviceContext* pDevice)
{
	assert(m_Device = pDevice);
	/*
	std::vector<CubeVertex> vertices{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) }
	};

	std::vector<uint32_t> indices{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};
	
	// Create Buffers
	{
		m_VertexBuffer.Create(m_Device, BufferData(vertices.data(), static_cast<uint32_t>(vertices.size()), sizeof(vertices.at(0)) * vertices.size()), BufferDesc());
	}
	{
		m_IndexBuffer.Create(m_Device, BufferData(indices.data(), static_cast<uint32_t>(indices.size()), indices.size() * sizeof(uint32_t)), BufferDesc());
	}
	*/
	m_ConstBuffer.Create(pDevice, &m_cbData);
	
	std::array<uint32_t, 36> indices =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};

	// Cube vertices positions and corresponding triangle normals.
	std::vector<VertexNormal> vertices =
	{
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
	};
	
	// Create Buffers
	{
		m_VertexBuffer.Create(m_Device, BufferData(vertices.data(), vertices.size(), sizeof(vertices.at(0)) * vertices.size(), sizeof(vertices.at(0))), BufferDesc());
		m_IndexBuffer.Create(m_Device, BufferData(indices.data(), indices.size(), indices.size() * sizeof(uint32_t), sizeof(uint32_t)), BufferDesc());
	}

}

void Cube::Draw(const DirectX::XMMATRIX& ViewProjection)
{
	m_Device->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_Device->GetCommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer.View);
	m_Device->GetCommandList()->IASetIndexBuffer(&m_IndexBuffer.View);

	auto frameIndex{ m_Device->FRAME_INDEX };
	m_ConstBuffer.Update({ XMMatrixTranspose(m_WorldMatrix * m_Translation *ViewProjection), DirectX::XMMatrixIdentity() }, frameIndex);
	m_Device->GetCommandList()->SetGraphicsRootConstantBufferView(0, m_ConstBuffer.GetBuffer(frameIndex)->GetGPUVirtualAddress());

	m_Device->GetCommandList()->DrawIndexedInstanced(m_IndexBuffer.Count, 1, 0, 0, 0);
}

void Cube::Release()
{
	if (m_Device)
		m_Device = nullptr;

}
