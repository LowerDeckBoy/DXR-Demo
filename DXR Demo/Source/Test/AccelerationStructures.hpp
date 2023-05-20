#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <span>

// https://alain.xyz/blog/ray-tracing-acceleration-structures
// https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/01-Dx12DXRTriangle/Project/D3D12HelloTriangle.cpp
// https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1
using Microsoft::WRL::ComPtr;
class AccelerationStructures
{
public:
	void Create();
	void CreateBottomLevel();
	void CreateTopLevel();

	ComPtr<ID3D12Resource> m_VertexBuffer;
	ComPtr<ID3D12Resource> m_IndexBuffer;

	ComPtr<ID3D12Resource> m_BottomLevelAS;
	ComPtr<ID3D12Resource> m_TopLevelAS;

	struct ASBuffers
	{
		ComPtr<ID3D12Resource> Scratch;
		ComPtr<ID3D12Resource> Result;
		ComPtr<ID3D12Resource> InstanceDesc;
	};

};

