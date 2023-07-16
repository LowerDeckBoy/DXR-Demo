#include "../Graphics/Buffer/Buffer.hpp"
#include "RaytracingContext.hpp"
#include <array>
#include "../Rendering/Camera.hpp"
#include "../Utilities/Utilities.hpp"
#include <imgui/imgui.h>

const wchar_t* RaytracingContext::m_HitGroupName			= L"HitGroup";
const wchar_t* RaytracingContext::m_RayGenShaderName		= L"RayGen";
const wchar_t* RaytracingContext::m_MissShaderName			= L"Miss";
const wchar_t* RaytracingContext::m_ClosestHitShaderName	= L"ClosestHit";

RaytracingContext::RaytracingContext(DeviceContext* pDeviceCtx, Camera* pCamera, VertexBuffer& Vertex, IndexBuffer& Index)
{
	{
		m_DeviceCtx = pDeviceCtx;
		assert(m_DeviceCtx);
		m_Camera = pCamera;

		m_VertexBuffer = Vertex;
		m_IndexBuffer = Index;
	}

	Create();
}

RaytracingContext::RaytracingContext(DeviceContext* pDeviceCtx, ShaderManager* pShaderManager, Camera* pCamera, VertexBuffer& Vertex, IndexBuffer& Index)
{
	{
		m_DeviceCtx = pDeviceCtx;
		assert(m_DeviceCtx);
		m_ShaderManager = pShaderManager;
		m_Camera = pCamera;

		m_VertexBuffer = Vertex;
		m_IndexBuffer = Index;
	}

	Create();
}

RaytracingContext::RaytracingContext(DeviceContext* pDeviceCtx, ShaderManager* pShaderManager, Camera* pCamera, std::vector<VertexBuffer>& Vertex, std::vector<IndexBuffer>& Index)
{
	{
		assert(m_DeviceCtx = pDeviceCtx);
		assert(m_ShaderManager = pShaderManager);
		assert(m_Camera = pCamera);

		m_VertexBuffers = Vertex;
		m_IndexBuffers  = Index;
	}

	Create();

}

RaytracingContext::~RaytracingContext() noexcept(false)
{
	SAFE_RELEASE(m_GlobalRootSignature);
	SAFE_RELEASE(m_HitRootSignature);
	SAFE_RELEASE(m_RayGenShader);
	SAFE_RELEASE(m_MissShader);
	SAFE_RELEASE(m_HitShader);

	SAFE_RELEASE(m_StateObjectProperties);
	SAFE_RELEASE(m_StateObject);

	if (m_Camera)
		m_Camera = nullptr;
	if (m_DeviceCtx)
		m_DeviceCtx = nullptr;
}

void RaytracingContext::Create()
{
	// Raytracing RTV Heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NumDescriptors = 64;
		m_RaytracingHeap = std::make_unique<DescriptorHeap>();
		ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_RaytracingHeap->GetHeapAddressOf())));
		m_RaytracingHeap->GetHeap()->SetName(L"Raytracing Heap");
	}

	// Const Buffers
	{
		m_SceneData = { XMMatrixIdentity() * m_Camera->GetViewProjection(),
			m_Camera->GetPosition(),
			XMVectorSet(0.0f, 1.5f, -8.0f, 1.0f),
			XMVectorSet(0.5f, 0.5f, 0.5f, 1.0f),
			XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f)
		};

		m_SceneBuffer.Create(m_DeviceCtx, &m_SceneData);
		m_CubeData = { XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f) };
		m_CubeBuffer.Create(m_DeviceCtx, &m_CubeData);

		XMVECTOR det{};
		m_CameraData = {
			m_Camera->GetView(),
			m_Camera->GetProjection(),
			XMMatrixTranspose(XMMatrixInverse(&det, m_Camera->GetView())),
			XMMatrixTranspose(XMMatrixInverse(&det, m_Camera->GetProjection()))
		};
		m_CameraBuffer.Create(m_DeviceCtx, &m_CameraData);
	}
	
	CreateRootSignatures();

	CreateStateObject();

	BuildAccelerationStructures();

	BuildShaderTables();

	CreateOutputResource();

}

void RaytracingContext::OnRaytrace()
{
	SetConstBufferData();
	UpdateCamera();

	DispatchRaytrace();
	OutputToBackbuffer();

	DrawGUI();
}

void RaytracingContext::DispatchRaytrace()
{
	auto commandList = m_DeviceCtx->GetCommandList();

	commandList->SetDescriptorHeaps(1, m_DeviceCtx->GetMainHeap()->GetHeapAddressOf());

	commandList->SetComputeRootSignature(m_GlobalRootSignature.Get());
	commandList->SetComputeRootDescriptorTable(0, m_OutputUAV.GetGPU());
	commandList->SetComputeRootShaderResourceView(1, m_AS.m_TopLevel.m_ResultBuffer->GetGPUVirtualAddress());
	// CBVs
	commandList->SetComputeRootConstantBufferView(2, m_CameraBuffer.GetBuffer(m_DeviceCtx->FRAME_INDEX)->GetGPUVirtualAddress());
	commandList->SetComputeRootConstantBufferView(3, m_SceneBuffer.GetBuffer(m_DeviceCtx->FRAME_INDEX)->GetGPUVirtualAddress());
	// Geometry Buffers
	commandList->SetComputeRootDescriptorTable(4, m_VertexBuffer.Buffer.m_Descriptor.GetGPU());
	commandList->SetComputeRootDescriptorTable(5, m_IndexBuffer.Buffer.m_Descriptor.GetGPU());

	D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
	// RayGen
	dispatchDesc.RayGenerationShaderRecord.StartAddress = m_RayGenTable.GetStorage()->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes	= m_RayGenTable.GetShaderRecordSize();
	// Miss
	dispatchDesc.MissShaderTable.StartAddress	= m_MissTable.GetStorage()->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes	= static_cast<uint64_t>(m_MissTable.GetShaderRecordSize() * m_MissTable.GetRecordsCount());
	dispatchDesc.MissShaderTable.StrideInBytes	= m_MissTable.GetShaderRecordSize();
	// Hit
	dispatchDesc.HitGroupTable.StartAddress  = m_HitTable.GetStorage()->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes	 = static_cast<uint64_t>(m_HitTable.GetShaderRecordSize() * m_HitTable.GetRecordsCount());
	dispatchDesc.HitGroupTable.StrideInBytes = m_HitTable.GetShaderRecordSize();
	// Output dimensions
	dispatchDesc.Width  = static_cast<uint32_t>(m_DeviceCtx->GetViewport().Width);
	dispatchDesc.Height = static_cast<uint32_t>(m_DeviceCtx->GetViewport().Height);
	// Primary Rays
	dispatchDesc.Depth  = m_MaxRecursiveDepth;
	// Dispatch
	commandList->SetPipelineState1(m_StateObject.Get());
	commandList->DispatchRays(&dispatchDesc);
}

void RaytracingContext::OutputToBackbuffer()
{
	auto renderTarget = m_DeviceCtx->GetRenderTarget(m_DeviceCtx->FRAME_INDEX);

	std::array<D3D12_RESOURCE_BARRIER, 2> preCopyBarriers{};
	preCopyBarriers.at(0) = CD3DX12_RESOURCE_BARRIER::Transition(m_RaytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	preCopyBarriers.at(1) = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	m_DeviceCtx->GetCommandList()->ResourceBarrier(static_cast<uint32_t>(preCopyBarriers.size()), preCopyBarriers.data());

	m_DeviceCtx->GetCommandList()->CopyResource(renderTarget, m_RaytracingOutput.Get());

	std::array<D3D12_RESOURCE_BARRIER, 2> postCopyBarriers{};
	postCopyBarriers.at(0) = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	postCopyBarriers.at(1) = CD3DX12_RESOURCE_BARRIER::Transition(m_RaytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_DeviceCtx->GetCommandList()->ResourceBarrier(static_cast<uint32_t>(postCopyBarriers.size()), postCopyBarriers.data());
}

void RaytracingContext::CreateRootSignatures()
{
	// Global Root Signature
	{
		D3D12_ROOT_SIGNATURE_DESC desc{};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		std::array<CD3DX12_DESCRIPTOR_RANGE, 4> ranges{};
		ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		ranges.at(1).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		ranges.at(2).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		ranges.at(3).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		std::array<CD3DX12_ROOT_PARAMETER, 6> params{};
		// Output buffer
		params.at(0).InitAsDescriptorTable(1, &ranges.at(0), D3D12_SHADER_VISIBILITY_ALL);
		// TLAS buffer
		params.at(1).InitAsShaderResourceView(0);
		// CBVs
		params.at(2).InitAsConstantBufferView(0, 0);
		params.at(3).InitAsConstantBufferView(1, 0);
		// Geometry Buffers
		params.at(4).InitAsDescriptorTable(1, &ranges.at(2));
		params.at(5).InitAsDescriptorTable(1, &ranges.at(3));

		desc.NumParameters = static_cast<uint32_t>(params.size());
		desc.pParameters = params.data();

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(
			&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(m_GlobalRootSignature.GetAddressOf())));
		m_GlobalRootSignature.Get()->SetName(L"Raytracing Global Root Signature");
	}

	// Local Root Signature
	{
		D3D12_ROOT_SIGNATURE_DESC desc{};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		std::array<CD3DX12_DESCRIPTOR_RANGE, 2> ranges{};
		ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		ranges.at(1).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		std::array<CD3DX12_ROOT_PARAMETER, 1> params{};
		params.at(0).InitAsConstants(sizeof(XMVECTOR), 0, 2);

		desc.NumParameters = static_cast<uint32_t>(params.size());
		desc.pParameters = params.data();

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(
			&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(m_HitRootSignature.GetAddressOf())));
		m_HitRootSignature.Get()->SetName(L"ClosestHit Local Root Signature");
	}
}

void RaytracingContext::CreateStateObject()
{
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	// DXIL
	m_RayGenShader	= m_ShaderManager->CreateDXIL("Assets/Raytracing/RayGen.hlsl", L"lib_6_3");
	m_MissShader	= m_ShaderManager->CreateDXIL("Assets/Raytracing/Miss.hlsl", L"lib_6_3");
	m_HitShader		= m_ShaderManager->CreateDXIL("Assets/Raytracing/Hit_3D.hlsl", L"lib_6_3");
	//m_HitShader		= Shader::CreateDXIL("Assets/Raytracing/Hit.hlsl");

	// RayGen
	auto raygenLib{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	{
		const auto raygenBytecode{ CD3DX12_SHADER_BYTECODE(m_RayGenShader->GetBufferPointer(), m_RayGenShader->GetBufferSize()) };
		raygenLib->SetDXILLibrary(&raygenBytecode);
		raygenLib->DefineExport(m_RayGenShaderName);
	}

	// Miss
	auto missLib{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	{
		const auto missBytecode{ CD3DX12_SHADER_BYTECODE(m_MissShader->GetBufferPointer(), m_MissShader->GetBufferSize()) };
		missLib->SetDXILLibrary(&missBytecode);
		missLib->DefineExport(m_MissShaderName);
	}

	// Closest Hit
	auto hitLib{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	{
		const auto hitBytecode{ CD3DX12_SHADER_BYTECODE(m_HitShader->GetBufferPointer(), m_HitShader->GetBufferSize()) };
		hitLib->SetDXILLibrary(&hitBytecode);
		hitLib->DefineExport(m_ClosestHitShaderName);
	}

	// HitGroup
	auto hitGroup{ raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>() };
	{
		hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
		hitGroup->SetHitGroupExport(m_HitGroupName);
		hitGroup->SetClosestHitShaderImport(m_ClosestHitShaderName);
	}

	// Shader Config
	auto shaderConfig{ raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>() };
	{
		m_PayloadSize = static_cast<uint32_t>(4 * sizeof(float));
		m_AttributeSize = static_cast<uint32_t>(2 * sizeof(float));
		shaderConfig->Config(m_PayloadSize, m_AttributeSize);
	}

	auto pipelineConfig{ raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>() };
	pipelineConfig->Config(m_MaxRecursiveDepth);

	// aka Local Root Signature
	auto hitSignature{ raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>() };
	hitSignature->SetRootSignature(m_HitRootSignature.Get());
	
	auto hitAssociation{ raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>() };
	hitAssociation->SetSubobjectToAssociate(*hitSignature);
	hitAssociation->AddExport(m_HitGroupName);

	auto globalRootSignature{ raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>() };
	globalRootSignature->SetRootSignature(m_GlobalRootSignature.Get());

	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(m_StateObject.GetAddressOf())));
	ThrowIfFailed(m_StateObject->QueryInterface(m_StateObjectProperties.ReleaseAndGetAddressOf()));
	m_StateObject.Get()->SetName(L"Raytracing State Object");

}

void RaytracingContext::CreateOutputResource()
{
	auto device{ m_DeviceCtx->GetDevice() };

	auto uavDesc{ CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 
		static_cast<uint64_t>(m_DeviceCtx->GetViewport().Width), 
		static_cast<uint32_t>(m_DeviceCtx->GetViewport().Height), 
		1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) };

	auto defaultHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_RaytracingOutput.GetAddressOf())));
	m_RaytracingOutput.Get()->SetName(L"Raytracing Output");

	m_DeviceCtx->GetMainHeap()->Allocate(m_OutputUAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(m_RaytracingOutput.Get(), nullptr, &UAVDesc, m_OutputUAV.GetCPU());

	m_DeviceCtx->GetMainHeap()->Allocate(m_TopLevelView);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = m_AS.m_TopLevel.m_ResultBuffer->GetGPUVirtualAddress();
	device->CreateShaderResourceView(nullptr, &srvDesc, m_TopLevelView.GetCPU());

}

void RaytracingContext::BuildAccelerationStructures()
{
	m_AS.Init(m_DeviceCtx);

	DirectX::XMMATRIX matrix{ DirectX::XMMatrixIdentity() };
	m_AS.CreateBottomLevel(m_VertexBuffer, m_IndexBuffer, true);
	m_AS.CreateTopLevel(m_AS.m_BottomLevel.m_ResultBuffer.Get(), matrix);
	//m_AS.CreateBottomLevel(m_VertexBuffers, m_IndexBuffers, true);
	//m_AS.CreateTopLevel(m_AS.m_BottomLevels, matrix);

	// Perhaps unnecessary for now
	m_DeviceCtx->WaitForGPU();
	m_DeviceCtx->FlushGPU();

}

void RaytracingContext::BuildShaderTables()
{
	constexpr uint32_t shaderIdentifierSize{ D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };
	
	// Unnecessary
	//const D3D12_GPU_DESCRIPTOR_HANDLE rtvHandle{ m_DeviceCtx->GetMainHeap()->GetHeap()->GetGPUDescriptorHandleForHeapStart() };
	//uint64_t* heapPointer{ reinterpret_cast<uint64_t*>(rtvHandle.ptr) };
	
	void* rayGenIdentifier	{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_RayGenShaderName) };
	void* missIdentifier	{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_MissShaderName) };
	void* hitIdentifier		{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_HitGroupName) };
	
	// RayGen Table
	{
		uint32_t argsSize{ shaderIdentifierSize };
		m_RayGenTable.Create(m_DeviceCtx->GetDevice(), 1, argsSize, L"RayGen Shader Table");
		TableRecord record(rayGenIdentifier, shaderIdentifierSize);
		m_RayGenTable.AddRecord(record);
	}
	// Miss Table
	{
		m_MissTable.Create(m_DeviceCtx->GetDevice(), 1, shaderIdentifierSize, L"Miss Shader Table");
		TableRecord record(missIdentifier, shaderIdentifierSize);
		m_MissTable.AddRecord(record);
	}
	// Hit Table
	{
		//auto* pVertex{ reinterpret_cast<void*>(m_VertexBuffer.Buffer.GetBuffer()->GetGPUVirtualAddress()) };
		//auto* pIndex{ reinterpret_cast<void*>(m_IndexBuffer.Buffer.GetBuffer()->GetGPUVirtualAddress()) };

		constexpr auto cbSize{ sizeof(m_CubeData) };
		m_CubeData = { XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f) };
		struct Args {
			XMVECTOR cb;
		} cubeBuffer{};
		cubeBuffer.cb = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);

		m_HitTable.Create(m_DeviceCtx->GetDevice(), 1, shaderIdentifierSize + sizeof(Args), L"Hit Shader Table");
		TableRecord record(hitIdentifier, shaderIdentifierSize, &cubeBuffer, sizeof(cubeBuffer));
		m_HitTable.AddRecord(record);
	}
}

void RaytracingContext::SerializeAndCreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc, ComPtr<ID3D12RootSignature>* ppRootSignature) const
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> error;

	ThrowIfFailed(D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, blob.GetAddressOf(), error.GetAddressOf()));
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*(ppRootSignature)))));
}

void RaytracingContext::SetConstBufferData()
{
	m_SceneData.ViewProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, m_Camera->GetViewProjection()));
	m_SceneData.CameraPosition = m_Camera->GetPosition();

	m_SceneBuffer.Update(m_SceneData, m_DeviceCtx->FRAME_INDEX);
		
	m_CubeData = { XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f) };
	m_CubeBuffer.Update(m_CubeData, m_DeviceCtx->FRAME_INDEX);
}

void RaytracingContext::DrawGUI()
{
	ImGui::Begin("Raytrace Lighting");

	if (ImGui::DragFloat3("Position", m_LightPosition.data()))
	{
		m_SceneData.LightPosition = XMVectorSet(m_LightPosition.at(0), m_LightPosition.at(1), m_LightPosition.at(2), 1.0f);
	}

	if (ImGui::ColorEdit4("Ambient", m_LightAmbient.data()))
	{
		m_SceneData.LightAmbient = XMVectorSet(m_LightAmbient.at(0), m_LightAmbient.at(1), m_LightAmbient.at(2), m_LightAmbient.at(3));
	}

	if (ImGui::ColorEdit4("Diffuse", m_LightDiffuse.data()))
	{
		m_SceneData.LightDiffuse = XMVectorSet(m_LightDiffuse.at(0), m_LightDiffuse.at(1), m_LightDiffuse.at(2), m_LightDiffuse.at(3));
	}

	//if (ImGui::ColorEdit4("Albedo"))
	//{
	//
	//}

	ImGui::End();
}

void RaytracingContext::UpdateCamera()
{
	XMVECTOR det{};
	m_CameraData = {
		m_Camera->GetView(),
		m_Camera->GetProjection(),
		XMMatrixInverse(&det, m_Camera->GetView()),
		XMMatrixInverse(&det, m_Camera->GetProjection())
	};
	m_CameraBuffer.Update(m_CameraData, m_DeviceCtx->FRAME_INDEX);
}
