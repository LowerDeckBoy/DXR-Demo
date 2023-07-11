#pragma once

#include "Buffer/ConstantBuffer.hpp"
#include "Buffer/Vertex.hpp"

class DeviceContext;
class VertexBuffer;
class IndexBuffer;

class Plane
{
public:
	Plane();
	~Plane();

	void Create(DeviceContext* pDevice);
	void Draw(DirectX::XMMATRIX ViewProjection);

	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;
private:
	DeviceContext* m_DeviceCtx{ nullptr };

	ConstantBuffer<cbPerObject> m_ConstBuffer;
	cbPerObject m_cbData{};

};
