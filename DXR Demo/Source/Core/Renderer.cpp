#include "Renderer.hpp"
#include "../Utilities/Utilities.hpp"

Renderer::Renderer()
{
	Initalize();
}

Renderer::~Renderer()
{
	Destroy();
}

void Renderer::Initalize()
{
	m_DeviceCtx = std::make_unique<DeviceContext>();
	CreateDepthStencil();

	CreatePipelines();

	LoadAssets();

	ExecuteCommandLists();
	WaitForGPU();
}

void Renderer::LoadAssets()
{
	// Load models

	m_Cube.Create(m_DeviceCtx.get());
}

void Renderer::Update(Camera* pCamera)
{
}

void Renderer::Render(Camera* pCamera)
{
	RecordCommandList(m_DeviceCtx->FRAME_INDEX, pCamera);

	ID3D12CommandList* commandLists[]{ m_DeviceCtx->GetCommandList() };
	m_DeviceCtx->GetCommandQueue()->ExecuteCommandLists(_countof(commandLists), commandLists);

	HRESULT hResult{ m_DeviceCtx->GetSwapChain()->Present(1, 0) };
	if (hResult == DXGI_ERROR_DEVICE_REMOVED || hResult == DXGI_ERROR_DEVICE_RESET)
	{
		throw std::exception();
	}

	MoveToNextFrame();
}

void Renderer::Resize()
{
}

void Renderer::Destroy()
{
	WaitForGPU();
	::CloseHandle(m_DeviceCtx->m_FenceEvent);
}

void Renderer::RecordCommandList(uint32_t CurrentFrame, Camera* pCamera)
{
	//auto frameIndex{ m_FrameIndex };
	ThrowIfFailed(m_DeviceCtx->GetCommandAllocator(m_DeviceCtx->FRAME_INDEX)->Reset());
	ThrowIfFailed(m_DeviceCtx->GetCommandList()->Reset(m_DeviceCtx->GetCommandAllocator(m_DeviceCtx->FRAME_INDEX), nullptr));

	m_DeviceCtx->GetCommandList()->SetPipelineState(m_PipelineState.Get());
	m_DeviceCtx->GetCommandList()->SetGraphicsRootSignature(m_RootSignature.Get());
	auto viewport{ m_DeviceCtx->GetViewport() };
	auto rect{ m_DeviceCtx->GetViewportRect() };
	m_DeviceCtx->GetCommandList()->RSSetViewports(1, &viewport);
	m_DeviceCtx->GetCommandList()->RSSetScissorRects(1, &rect);

	TransitToRender();

	ID3D12DescriptorHeap* ppHeaps[] = { m_DeviceCtx->GetMainHeap()->GetHeap() };
	m_DeviceCtx->GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	SetRenderTarget();
	// Draw Triangle
	//m_DeviceCtx->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//m_DeviceCtx->GetCommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer.GetBufferView());
	//m_DeviceCtx->GetCommandList()->DrawInstanced(3, 1, 0, 0);

	m_Cube.Draw(pCamera->GetViewProjection());

	TransitToPresent();

	ThrowIfFailed(m_DeviceCtx->GetCommandList()->Close());
}

void Renderer::ExecuteCommandLists()
{
	ThrowIfFailed(m_DeviceCtx->GetCommandList()->Close());

	ID3D12CommandList* ppCommandLists[] = { m_DeviceCtx->GetCommandList() };
	m_DeviceCtx->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForGPU();
}

void Renderer::SetRenderTarget()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_DeviceCtx->GetRenderTargetHeap()->GetCPUDescriptorHandleForHeapStart(), m_DeviceCtx->FRAME_INDEX, m_DeviceCtx->GetRenderTargetHeapDescriptorSize());
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle(m_DepthHeap.Get()->GetCPUDescriptorHandleForHeapStart());
	m_DeviceCtx->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &depthHandle);

	ClearRenderTarget(rtvHandle, depthHandle);
}

void Renderer::ClearRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle)
{
	m_DeviceCtx->GetCommandList()->ClearRenderTargetView(rtvHandle, m_ClearColor.data(), 0, nullptr);
	m_DeviceCtx->GetCommandList()->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void Renderer::TransitToRender()
{
	auto presentToRender = CD3DX12_RESOURCE_BARRIER::Transition(m_DeviceCtx->GetRenderTarget(m_DeviceCtx->FRAME_INDEX), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_DeviceCtx->GetCommandList()->ResourceBarrier(1, &presentToRender);
}

void Renderer::TransitToPresent()
{
	auto renderToPresent = CD3DX12_RESOURCE_BARRIER::Transition(m_DeviceCtx->GetRenderTarget(m_DeviceCtx->FRAME_INDEX), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_DeviceCtx->GetCommandList()->ResourceBarrier(1, &renderToPresent);
}

void Renderer::FlushGPU()
{
	for (uint32_t i = 0; i < m_DeviceCtx->FRAME_COUNT; i++)
	{
		const uint64_t currentValue{ m_DeviceCtx->FRAME_INDEX };

		ThrowIfFailed(m_DeviceCtx->GetCommandQueue()->Signal(m_DeviceCtx->GetFence(), currentValue));
		m_DeviceCtx->m_FenceValues[i]++;

		if (m_DeviceCtx->GetFence()->GetCompletedValue() < currentValue)
		{
			ThrowIfFailed(m_DeviceCtx->GetFence()->SetEventOnCompletion(currentValue, m_DeviceCtx->m_FenceEvent));

			WaitForSingleObject(m_DeviceCtx->m_FenceEvent, INFINITE);
		}
	}

	m_DeviceCtx->FRAME_INDEX = 0;
}

void Renderer::MoveToNextFrame()
{
	const UINT64 currentFenceValue = m_DeviceCtx->m_FenceValues[m_DeviceCtx->FRAME_INDEX];
	ThrowIfFailed(m_DeviceCtx->GetCommandQueue()->Signal(m_DeviceCtx->GetFence(), currentFenceValue));

	// Update the frame index.
	m_DeviceCtx->FRAME_INDEX = m_DeviceCtx->GetSwapChain()->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_DeviceCtx->GetFence()->GetCompletedValue() < m_DeviceCtx->m_FenceValues[m_DeviceCtx->FRAME_INDEX])
	{
		ThrowIfFailed(m_DeviceCtx->GetFence()->SetEventOnCompletion(m_DeviceCtx->m_FenceValues[m_DeviceCtx->FRAME_INDEX], m_DeviceCtx->m_FenceEvent));
		WaitForSingleObjectEx(m_DeviceCtx->m_FenceEvent, INFINITE, FALSE);
	}

	m_DeviceCtx->m_FenceValues[m_DeviceCtx->FRAME_INDEX] = currentFenceValue + 1;
}

void Renderer::WaitForGPU()
{
	ThrowIfFailed(m_DeviceCtx->GetCommandQueue()->Signal(m_DeviceCtx->GetFence(), m_DeviceCtx->m_FenceValues[m_DeviceCtx->FRAME_INDEX]));

	ThrowIfFailed(m_DeviceCtx->m_Fence->SetEventOnCompletion(m_DeviceCtx->m_FenceValues[m_DeviceCtx->FRAME_INDEX], m_DeviceCtx->m_FenceEvent));
	::WaitForSingleObjectEx(m_DeviceCtx->m_FenceEvent, INFINITE, FALSE);

	m_DeviceCtx->m_FenceValues[m_DeviceCtx->FRAME_INDEX]++;
}

void Renderer::CreateDepthStencil()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsHeap{};
	dsHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsHeap.NumDescriptors = 1;
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateDescriptorHeap(&dsHeap, IID_PPV_ARGS(m_DepthHeap.GetAddressOf())));

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	auto heapDesc{ CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
												static_cast<uint64_t>(m_DeviceCtx->GetViewport().Width),
												static_cast<uint32_t>(m_DeviceCtx->GetViewport().Height),
												1, 0, 1, 0,
												D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) };

	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&heapDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(m_DepthStencil.GetAddressOf())));
	m_DepthHeap.Get()->SetName(L"Depth Heap");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsView{};
	dsView.Flags = D3D12_DSV_FLAG_NONE;
	dsView.Format = DXGI_FORMAT_D32_FLOAT;
	dsView.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsView.Texture2D.MipSlice = 0;

	m_DeviceCtx->GetDevice()->CreateDepthStencilView(m_DepthStencil.Get(), &dsView, m_DepthHeap.Get()->GetCPUDescriptorHandleForHeapStart());
	m_DepthStencil.Get()->SetName(L"Depth Stencil");
}

void Renderer::CreatePipelines()
{
	//m_VertexShader.Create("Assets/Shaders/Vertex_01.hlsl", "vs_5_1");
	//m_PixelShader.Create("Assets/Shaders/Pixel_01.hlsl", "ps_5_1");
	m_VertexShader.Create("Assets/Shaders/Global_Vertex.hlsl", "vs_5_1");
	m_PixelShader.Create("Assets/Shaders/Global_Pixel.hlsl", "ps_5_1");

	std::array<CD3DX12_DESCRIPTOR_RANGE, 1> ranges{};
	// Vertex
	ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	std::array<CD3DX12_ROOT_PARAMETER, 1> parameters{};
	// Vertex 
	parameters.at(0).InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);


	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	//rootSignatureDesc.Init(
	//	0, nullptr, 0, nullptr,
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	rootSignatureDesc.Init(static_cast<uint32_t>(parameters.size()), parameters.data(), 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)));

	std::vector<SimpleVertex> triangleVertices = {
			{ { 0.0f,   0.25f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f} },
			{ { 0.25f, -0.25f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} },
			{ {-0.25f, -0.25f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f} }
	};
	m_VertexBuffer.Create(m_DeviceCtx->GetDevice(), triangleVertices);

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_RootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_VertexShader.GetData());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_PixelShader.GetData());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf())));


}
