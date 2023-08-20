#pragma once

#include "Buffer/ConstantBuffer.hpp"
#include "Buffer/Vertex.hpp"

class DeviceContext;
class VertexBuffer;
class IndexBuffer;

class Plane
{
public:
	Plane() {}
	Plane(DeviceContext* pDevice);
	~Plane();

	void Create(DeviceContext* pDevice);
	void Draw(const DirectX::XMMATRIX& ViewProjection);

	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	ConstantBuffer<cbPerObject> m_ConstBuffer;

private:
	DeviceContext* m_DeviceCtx{ nullptr };

	cbPerObject m_cbData{};

};
