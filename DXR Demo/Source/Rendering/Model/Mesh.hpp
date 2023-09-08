#pragma once
#include "../../Graphics/Texture.hpp"
#include "../../Graphics/Buffer/Vertex.hpp"
#include <DirectXMath.h>
using namespace DirectX;

namespace model 
{
	struct Material
	{
		XMFLOAT4 BaseColorFactor{ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) };
		XMFLOAT4 EmissiveFactor	{ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) };

		float MetallicFactor	{ 1.0f };
		float RoughnessFactor	{ 1.0f };
		float AlphaCutoff		{ 0.5f };
		BOOL bDoubleSided		{ FALSE };

		Texture* BaseColorTexture			{ nullptr };
		Texture* NormalTexture				{ nullptr };
		Texture* MetallicRoughnessTexture	{ nullptr };
		Texture* EmissiveTexture			{ nullptr };
	};

	struct MaterialData
	{
		XMFLOAT4 BaseColorFactor{ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) };
		XMFLOAT4 EmissiveFactor { XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) };

		float MetallicFactor { 1.0f };
		float RoughnessFactor{ 1.0f };
		float AlphaCutoff	 { 0.5f };
		BOOL bDoubleSided	 { FALSE };

		int32_t BaseColorIndex			{ -1 };
		int32_t NormalIndex				{ -1 };
		int32_t MetallicRoughnessIndex	{ -1 };
		int32_t EmissiveIndex			{ -1 };
	};

	// Meant for pushing constants
	// -1 equals invalid index
	struct MaterialIndices
	{
		int32_t BaseColorIndex			{ -1 };
		int32_t NormalIndex				{ -1 };
		int32_t MetallicRoughnessIndex	{ -1 };
		int32_t EmissiveIndex			{ -1 };
	};

	struct Mest_Test
	{
		std::string Name;
		XMMATRIX Matrix{ XMMatrixIdentity() };

		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;

		uint32_t MaterialIndex{ 0 };
	};

	struct Material_Test
	{
		std::string Name;
		DirectX::XMFLOAT4 BaseColorFactor;
		DirectX::XMFLOAT4 EmissiveFactor;

		float MetallicFactor	{ 1.0f };
		float RoughnessFactor	{ 1.0f };
		float AlphaCutoff		{ 0.5f };
		BOOL bDoubleSided		{ false };

		// Texture indices -> bindless
		int32_t BaseColorIndex		{ -1 };
		int32_t NormalIndex			{ -1 };
		int32_t MetalRoughnessIndex	{ -1 };
		int32_t EmissiveIndex		{ -1 };

		// TODO: 
		//float RefractionFactor;
		// TODO: Add enums
		//uint8_t ShadingModel;

	};


	struct Mesh
	{
		std::string Name;
		XMMATRIX Matrix{ XMMatrixIdentity() };

		uint32_t FirstIndexLocation{ 0 };
		uint32_t BaseVertexLocation{ 0 };

		uint32_t IndexCount{ 0 };
		uint32_t VertexCount{ 0 };
		uint32_t StartVertexLocation{ 0 };
		bool bHasIndices{ false };

	};

	struct Node
	{
		Node* Parent{ nullptr };
		std::vector<Node*> Children;
		std::string Name;

		XMMATRIX Matrix{ XMMatrixIdentity() };
		XMFLOAT3 Translation{ XMFLOAT3(0.0f, 0.0f, 0.0f) };
		XMFLOAT4 Rotation{ XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) };
		XMFLOAT3 Scale{ XMFLOAT3(1.0f, 1.0f, 1.0f) };
	};
}