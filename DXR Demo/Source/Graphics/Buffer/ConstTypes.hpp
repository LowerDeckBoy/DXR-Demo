#pragma once

using namespace DirectX;

struct cbPerObject
{
	XMMATRIX WVP{ XMMatrixIdentity() };
	XMMATRIX World{ XMMatrixIdentity() };
	float padding[32]{};
};

struct cbCamera
{
	XMFLOAT4 CameraPosition;
};

struct cbMaterial
{
	XMFLOAT4 Ambient { XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) };
	XMFLOAT4 Diffuse { XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f) };
	// 3 floats for color, 1 for intensity
	XMFLOAT4 Specular{ XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f) };
	XMFLOAT4 Direction{ XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f) };

	XMFLOAT4 BaseColorFactor{ XMFLOAT4(1.0f, 1.0f,1.0f, 1.0f) };
	XMFLOAT4 EmissiveFactor{ XMFLOAT4(1.0f, 1.0f,1.0f, 1.0f) };

	float MetallicFactor{ 1.0f };
	float RoughnessFactor{ 1.0f };
	float AlphaCutoff{ 0.5f };
	float padding;

	XMFLOAT4 padding2[9];
};
