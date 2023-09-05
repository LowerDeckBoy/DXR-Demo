#pragma once
#include "Buffer/Buffer.hpp"
#include "Buffer/ConstantBuffer.hpp"
#include "Buffer/Vertex.hpp"
//#include <DirectXMath.h>

class Device;
using namespace DirectX;

class Cube
{
public:
	Cube() = default;
	Cube(DeviceContext* pDevice);
	~Cube();

	// Default colored Cube
	void Create(DeviceContext* pDevice);
	void Draw(const DirectX::XMMATRIX& ViewProjection);
	void Release();

	//private:
	DeviceContext* m_Device{ nullptr };
	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	ConstantBuffer<cbPerObject> m_ConstBuffer;
	cbPerObject m_cbData{};

	DirectX::XMMATRIX m_WorldMatrix{ XMMatrixIdentity() };
	DirectX::XMMATRIX m_Translation{ XMMatrixTranslation(0.0f, 0.0f, 0.0f) };

};
