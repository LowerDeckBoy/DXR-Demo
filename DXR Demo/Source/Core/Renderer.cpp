#include "../Rendering/Camera.hpp"
#include "Renderer.hpp"
#include "../Utilities/Utilities.hpp"

bool Renderer::bVSync = true;

Renderer::Renderer(Camera* pCamera, Timer* pTimer)
{
	Initalize(pCamera, pTimer);
}

Renderer::~Renderer()
{
	Release();
}

void Renderer::Initalize(Camera* pCamera, Timer* pTimer)
{
	m_DeviceCtx = std::make_unique<DeviceContext>();

	m_ShaderManager = std::make_shared<ShaderManager>();
	m_Editor = std::make_unique<Editor>();
	m_Editor->Initialize(m_DeviceCtx.get(), pCamera, pTimer);

	CreatePipelines();

	LoadAssets();

	// Pass current objects geometry data
	//m_Cube->m_VertexBuffer
	
	std::vector<VertexBuffer> vertex{ *m_Models.at(0)->m_VertexBuffer.get(), m_Plane->m_VertexBuffer};
	std::vector<IndexBuffer> index{ *m_Models.at(0)->m_IndexBuffer.get(), m_Plane->m_IndexBuffer};
	std::vector<ConstantBuffer<cbPerObject>> constants{ m_Cube->m_ConstBuffer, m_Plane->m_ConstBuffer };
		
	std::vector<Texture> textures{ *m_Models.at(0)->m_Textures.at(0) };
	//m_RaytracingContext = std::make_unique<RaytracingContext>(m_DeviceCtx.get(), m_ShaderManager.get(), pCamera, vertex, index, constants);
	m_RaytracingContext = std::make_unique<RaytracingContext>(m_DeviceCtx.get(), m_ShaderManager.get(), pCamera, vertex, index, textures);

	m_DeviceCtx->ExecuteCommandLists();
	m_DeviceCtx->WaitForGPU();
	
}

void Renderer::LoadAssets()
{
	m_Cube  = new Cube(m_DeviceCtx.get());
	m_Plane = new Plane(m_DeviceCtx.get());

	// Load models
	//m_Models.emplace_back(new Model(m_DeviceCtx.get(), "Assets/glTF/damaged_helmet/scene.gltf", "Helmet"));
	//m_Models.emplace_back(new Model(m_DeviceCtx.get(), "Assets/glTF/mathilda/scene.gltf", "Helmet"));
	m_Models.emplace_back(new Model(m_DeviceCtx.get(), "Assets/glTF/suzanne/Suzanne.gltf", "Helmet"));
	//m_Models.emplace_back(new Model(m_DeviceCtx.get(), "Assets/glTF/sponza/Sponza.gltf", "Helmet"));

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

	const HRESULT hResult{ m_DeviceCtx->GetSwapChain()->Present(bVSync ? 1 : 0, 0) };
	if (hResult == DXGI_ERROR_DEVICE_REMOVED || hResult == DXGI_ERROR_DEVICE_RESET)
	{
		throw std::logic_error("Device Removed!");
	}

	m_DeviceCtx->MoveToNextFrame();
}

void Renderer::OnResize()
{
	m_DeviceCtx->OnResize();
	m_RaytracingContext->OnResize();

	m_DeviceCtx->WaitForGPU(); 
	m_DeviceCtx->FlushGPU();
	m_DeviceCtx->ExecuteCommandLists();
}

void Renderer::Release()
{
	m_DeviceCtx->WaitForGPU();

	SAFE_RELEASE(m_PipelineState);
	SAFE_RELEASE(m_RootSignature);

	if (m_ShaderManager)
	{
		m_ShaderManager.reset();
		m_ShaderManager = nullptr;
	}

	::CloseHandle(m_DeviceCtx->m_FenceEvent);
}

void Renderer::RecordCommandList(uint32_t CurrentFrame, Camera* pCamera)
{
	BeginFrame();

	SetHeaps(m_DeviceCtx->GetMainHeap()->GetHeapAddressOf());

	// Spacebar to switch modes
	if (bRaster)
	{
		m_DeviceCtx->ClearRenderTarget();
		m_DeviceCtx->GetCommandList()->SetPipelineState(m_PipelineState.Get());
		m_DeviceCtx->GetCommandList()->SetGraphicsRootSignature(m_RootSignature.Get());
		m_DeviceCtx->GetCommandList()->SetGraphicsRootDescriptorTable(3, m_DeviceCtx->GetMainHeap()->GetHeap()->GetGPUDescriptorHandleForHeapStart());
		m_Plane->Draw(pCamera->GetViewProjection());
		//m_Cube->Draw(pCamera->GetViewProjection());
		for (const auto& model : m_Models)
			model->Draw(pCamera);
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

	const auto viewport{ m_DeviceCtx->GetViewport() };
	const auto rect{ m_DeviceCtx->GetViewportRect() };
	m_DeviceCtx->GetCommandList()->RSSetViewports(1, &viewport);
	m_DeviceCtx->GetCommandList()->RSSetScissorRects(1, &rect);

	m_Editor->BeginFrame();

	TransitToRender();
	m_DeviceCtx->SetRenderTarget();

}

void Renderer::EndFrame()
{
	m_Editor->EndFrame();

	TransitToPresent(D3D12_RESOURCE_STATE_RENDER_TARGET);
	ThrowIfFailed(m_DeviceCtx->GetCommandList()->Close());
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

void Renderer::SetHeaps(ID3D12DescriptorHeap** ppHeap)
{
	m_DeviceCtx->GetCommandList()->SetDescriptorHeaps(1, ppHeap);
}

void Renderer::CreatePipelines()
{
	//m_VertexShader = m_ShaderManager->CreateDXIL("Assets/Shaders/Global_Vertex.hlsl", L"vs_6_0");
	//m_PixelShader  = m_ShaderManager->CreateDXIL("Assets/Shaders/Global_Pixel.hlsl", L"ps_6_0");
	m_VertexShader = m_ShaderManager->CreateDXIL("Assets/Shaders/Model_Vertex.hlsl", L"vs_6_0");
	m_PixelShader = m_ShaderManager->CreateDXIL("Assets/Shaders/Model_Pixel.hlsl", L"ps_6_0");

	std::array<CD3DX12_DESCRIPTOR_RANGE1, 1> ranges{};
	ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	std::array<CD3DX12_ROOT_PARAMETER1, 5> parameters{};
	// Vertex 
	parameters.at(0).InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	// Camera Position
	parameters.at(1).InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	// Pixel Constant Buffer
	parameters.at(2).InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	// Bindless
	parameters.at(3).InitAsDescriptorTable(1, &ranges.at(0), D3D12_SHADER_VISIBILITY_ALL);
	// Material indices
	parameters.at(4).InitAsConstants(4 * sizeof(int32_t), 0, 2);

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = static_cast<float>(UINT32_MAX);
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init_1_1(static_cast<uint32_t>(parameters.size()), parameters.data(), 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)));

	SAFE_RELEASE(signature);
	SAFE_RELEASE(error);

	std::array<D3D12_INPUT_ELEMENT_DESC, 3> inputElementDescs{};
	inputElementDescs.at(0) = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs.at(1) = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs.at(2) = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	//std::array<D3D12_INPUT_ELEMENT_DESC, 5> inputElementDescs{};
	//inputElementDescs.at(0) = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	//inputElementDescs.at(1) = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	//inputElementDescs.at(2) = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	//inputElementDescs.at(3) = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	//inputElementDescs.at(4) = { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs.data(), static_cast<uint32_t>(inputElementDescs.size())};
	psoDesc.pRootSignature = m_RootSignature.Get();
	psoDesc.VS = { m_VertexShader.Get()->GetBufferPointer(), m_VertexShader.Get()->GetBufferSize() };
	psoDesc.PS = { m_PixelShader.Get()->GetBufferPointer(), m_PixelShader.Get()->GetBufferSize() };
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
