#include "Engine/Engine.hpp"

// Reference
// https://alextardif.com/DXRQuickStart.html
// https://github.com/michal-z/SimpleRaytracer

// https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/01-Dx12DXRTriangle/CONCEPTS.md
// https://developer.nvidia.com/rtx/raytracing/dxr/DX12-Raytracing-tutorial-Part-2
// https://developer.nvidia.com/blog/introduction-nvidia-rtx-directx-ray-tracing/
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12Raytracing/src/D3D12RaytracingHelloWorld/D3D12RaytracingHelloWorld.cpp

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	Engine app(hInstance);
	try
	{
		app.Initialize();
		app.Run();
	}
	catch (...)
	{
		::MessageBoxA(app.GetHWND(), "Failed to run app!", "Error", MB_OK);
		return -1;
	}
	
	return 0;
}