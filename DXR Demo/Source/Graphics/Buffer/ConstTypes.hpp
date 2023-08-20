#pragma once

using namespace DirectX;

struct cbPerObject
{
	XMMATRIX WVP{ XMMatrixIdentity() };
	XMMATRIX World{ XMMatrixIdentity() };
};

struct cbCamera
{
	XMFLOAT4 CameraPosition;
};

struct cbMaterial
{
	XMFLOAT4 CameraPosition{ XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f) };

	XMFLOAT4 BaseColorFactor{ XMFLOAT4(1.0f, 1.0f,1.0f, 1.0f) };
	XMFLOAT4 EmissiveFactor{ XMFLOAT4(1.0f, 1.0f,1.0f, 1.0f) };

	float MetallicFactor{ 1.0f };
	float RoughnessFactor{ 1.0f };
	float AlphaCutoff{ 0.5f };
	BOOL  bDoubleSided{ false };

	int32_t BaseColorIndex{ -1 };
	int32_t NormalIndex{ -1 };
	int32_t MetallicRoughnessIndex{ -1 };
	int32_t EmissiveIndex{ -1 };

	//XMFLOAT4 padding2[8]{};
};
