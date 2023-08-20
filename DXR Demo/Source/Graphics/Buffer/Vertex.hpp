#pragma once
#include <DirectXMath.h>


struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT2 TexCoord;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Tangent;
	DirectX::XMFLOAT3 Bitangent;
};

struct VertexUV
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT2 TexCoord;
};

struct SimpleVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT4 Color;
};

struct VertexNormal
{
	DirectX::XMFLOAT3 Positon;
	DirectX::XMFLOAT3 Normal;
};

struct SkyboxVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT2 TexCoord;
};
