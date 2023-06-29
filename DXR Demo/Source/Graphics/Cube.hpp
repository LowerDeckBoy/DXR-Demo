#pragma once
#include "Buffer/Buffer.hpp"
#include "Buffer/ConstantBuffer.hpp"
#include "Buffer/Vertex.hpp"
//#include <DirectXMath.h>

class Device;
using namespace DirectX;
struct CubeVertex
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
};

struct CubeNormal
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
};

class Cube
{
public:
	Cube() {}
	~Cube();

	// Default colored Cube
	void Create(DeviceContext* pDevice);
	void Draw();
	void Draw(DirectX::XMMATRIX ViewProjection);
	void Release();

//private:
	DeviceContext* m_Device{ nullptr };
	//VertexBuffer<CubeVertex> m_VertexBuffer;
	//IndexBuffer m_IndexBuffer;
	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;
	

	cbPerObject m_cbData{};
	ConstantBuffer<cbPerObject> m_ConstBuffer;


	/*
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
	*/
};
