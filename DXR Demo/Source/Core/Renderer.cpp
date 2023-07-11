#include "Renderer.hpp"
#include "../Utilities/Utilities.hpp"

Renderer::Renderer(Camera* pCamera)
{
	Initalize(pCamera);
}

Renderer::~Renderer()
{
	Destroy();
}

void Renderer::Initalize(Camera* pCamera)
{
	m_DeviceCtx = std::make_unique<DeviceContext>();
	CreateDepthStencil();
	CreatePipelines();

	m_ShaderManager = std::make_shared<ShaderManager>();

	LoadAssets();

	// Pass current object geometry data
	//m_RaytracingContext = std::make_unique<RaytracingContext>(m_DeviceCtx.get(), m_VertexBuffer, m_IndexBuffer);
	m_RaytracingContext = std::make_unique<RaytracingContext>(m_DeviceCtx.get(), m_ShaderManager.get(), pCamera, m_Cube.m_VertexBuffer, m_Cube.m_IndexBuffer);
	//std::vector<VertexBuffer> vertices{ m_Cube.m_VertexBuffer, m_Plane.m_VertexBuffer };
	//std::vector<IndexBuffer> indices{ m_Cube.m_IndexBuffer, m_Plane.m_IndexBuffer };

	//m_RaytracingContext = std::make_unique<RaytracingContext>(m_DeviceCtx.get(), m_ShaderManager.get(), pCamera, vertices, indices);

	m_DeviceCtx->ExecuteCommandLists();
	m_DeviceCtx->WaitForGPU();

}

void Renderer::LoadAssets()
{
	m_Cube.Create(m_DeviceCtx.get());
	m_Plane.Create(m_DeviceCtx.get());

	// Triangle data
	float aspectRatio{ m_DeviceCtx->GetViewport().Width / m_DeviceCtx->GetViewport().Height };
	std::vector<SimpleVertex> triangleVertices = {
			{ { +0.0f,  +0.25f * aspectRatio, 0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
			{ { +0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
			{ { -0.25f, -0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } }
	};

	std::vector<uint32_t> indices {
		0, 1, 2
	};

	m_VertexBuffer.Create(m_DeviceCtx.get(), BufferData(triangleVertices.data(), triangleVertices.size(), sizeof(triangleVertices.at(0)) * triangleVertices.size(), sizeof(SimpleVertex)), BufferDesc());

	m_IndexBuffer.Create(m_DeviceCtx.get(), BufferData(indices.data(), indices.size(), sizeof(uint32_t) * indices.size(), sizeof(uint32_t)), BufferDesc());
}

void Renderer::OnRaytrace()
{
	m_RaytracingContext->OnRaytrace();
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
		throw std::logic_error("Device Removed!");
	}

	m_DeviceCtx->MoveToNextFrame();
}

void Renderer::Resize()
{
	// TODO:
}

void Renderer::Destroy()
{
	m_DeviceCtx->WaitForGPU();

	SAFE_RELEASE(m_DepthStencil);
	SAFE_RELEASE(m_DepthHeap);
	SAFE_RELEASE(m_PipelineState);
	SAFE_RELEASE(m_RootSignature);

	::CloseHandle(m_DeviceCtx->m_FenceEvent);
}

void Renderer::RecordCommandList(uint32_t CurrentFrame, Camera* pCamera)
{
	BeginFrame();
	SetRenderTarget();

	// Spacebar to switch modes
	if (bRaster)
	{
		std::array<ID3D12DescriptorHeap*, 1> ppHeaps{ m_DeviceCtx->GetMainHeap()->GetHeap() };
		m_DeviceCtx->GetCommandList()->SetDescriptorHeaps(static_cast<uint32_t>(ppHeaps.size()), ppHeaps.data());


		ClearRenderTarget();
		m_DeviceCtx->GetCommandList()->SetPipelineState(m_PipelineState.Get());
		m_DeviceCtx->GetCommandList()->SetGraphicsRootSignature(m_RootSignature.Get());
		m_Plane.Draw(pCamera->GetViewProjection());
		m_Cube.Draw(pCamera->GetViewProjection());
	}
	else // Raytrace
	{
		OnRaytrace();
	}

	EndFrame();
}

void Renderer::BeginFrame()
{
	ThrowIfFailed(m_DeviceCtx->GetCommandAllocator(m_DeviceCtx->FRAME_INDEX)->Reset());
	ThrowIfFailed(m_DeviceCtx->GetCommandList()->Reset(m_DeviceCtx->GetCommandAllocator(m_DeviceCtx->FRAME_INDEX), nullptr));

	auto viewport{ m_DeviceCtx->GetViewport() };
	auto rect{ m_DeviceCtx->GetViewportRect() };
	m_DeviceCtx->GetCommandList()->RSSetViewports(1, &viewport);
	m_DeviceCtx->GetCommandList()->RSSetScissorRects(1, &rect);

	TransitToRender();
}

void Renderer::EndFrame()
{
	TransitToPresent(D3D12_RESOURCE_STATE_RENDER_TARGET);
	ThrowIfFailed(m_DeviceCtx->GetCommandList()->Close());
}

void Renderer::SetRenderTarget()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_DeviceCtx->GetRenderTargetHeap()->GetCPUDescriptorHandleForHeapStart(), m_DeviceCtx->FRAME_INDEX, m_DeviceCtx->GetRenderTargetHeapDescriptorSize());
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle(m_DepthHeap.Get()->GetCPUDescriptorHandleForHeapStart());
	m_DeviceCtx->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &depthHandle);
}

void Renderer::ClearRenderTarget()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_DeviceCtx->GetRenderTargetHeap()->GetCPUDescriptorHandleForHeapStart(), m_DeviceCtx->FRAME_INDEX, m_DeviceCtx->GetRenderTargetHeapDescriptorSize());
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle(m_DepthHeap.Get()->GetCPUDescriptorHandleForHeapStart());
	m_DeviceCtx->GetCommandList()->ClearRenderTargetView(rtvHandle, m_ClearColor.data(), 0, nullptr);
	m_DeviceCtx->GetCommandList()->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void Renderer::TransitToRender()
{
	auto presentToRender = CD3DX12_RESOURCE_BARRIER::Transition(m_DeviceCtx->GetRenderTarget(m_DeviceCtx->FRAME_INDEX), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_DeviceCtx->GetCommandList()->ResourceBarrier(1, &presentToRender);
}

void Renderer::TransitToPresent(D3D12_RESOURCE_STATES StateBefore)
{
	if (bRaster)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_DeviceCtx->GetRenderTarget(m_DeviceCtx->FRAME_INDEX), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_DeviceCtx->GetCommandList()->ResourceBarrier(1, &barrier);
	} 
	else
	{
		if (StateBefore != D3D12_RESOURCE_STATE_PRESENT)
		{
			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_DeviceCtx->GetRenderTarget(m_DeviceCtx->FRAME_INDEX), StateBefore, D3D12_RESOURCE_STATE_PRESENT);
			m_DeviceCtx->GetCommandList()->ResourceBarrier(1, &barrier);
		}
	}
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
												1, 1) };
	heapDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&heapDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(m_DepthStencil.GetAddressOf())));
	m_DepthHeap.Get()->SetName(L"Depth Heap");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsView{};
	dsView.Format = DXGI_FORMAT_D32_FLOAT;
	dsView.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsView.Texture2D.MipSlice = 0;

	m_DeviceCtx->GetDevice()->CreateDepthStencilView(m_DepthStencil.Get(), &dsView, m_DepthHeap.Get()->GetCPUDescriptorHandleForHeapStart());
	m_DepthStencil.Get()->SetName(L"Depth Stencil");
}

void Renderer::CreatePipelines()
{
	//m_VertexShader.Create("Assets/Shaders/Vertex_Triangle.hlsl", "vs_5_1");
	//m_PixelShader.Create("Assets/Shaders/Pixel_Triangle.hlsl", "ps_5_1");
	m_VertexShader.Create("Assets/Shaders/Global_Vertex.hlsl", "vs_5_1");
	m_PixelShader.Create("Assets/Shaders/Global_Pixel.hlsl", "ps_5_1");

	std::array<CD3DX12_DESCRIPTOR_RANGE, 1> ranges{};
	// Vertex
	ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	std::array<CD3DX12_ROOT_PARAMETER, 1> parameters{};
	// Vertex 
	parameters.at(0).InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init(static_cast<uint32_t>(parameters.size()), parameters.data(), 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)));

	std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputElementDescs{};
	inputElementDescs.at(0) = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs.at(1) = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	//inputElementDescs.at(1) = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs.data(), static_cast<uint32_t>(inputElementDescs.size())};
	psoDesc.pRootSignature = m_RootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_VertexShader.GetData());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_PixelShader.GetData());
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf())));

}
