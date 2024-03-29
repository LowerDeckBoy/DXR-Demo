#include "../Graphics/Buffer/Buffer.hpp"
#include "RaytracingContext.hpp"
#include <array>
#include "../Rendering/Camera.hpp"
#include "../Rendering/Model/Model.hpp"
#include "../Utilities/Utilities.hpp"
#include <imgui.h>
#include "../Core/DeviceContext.hpp"
#include "../Graphics/Shader.hpp"
#include <vector>

const wchar_t* RaytracingContext::m_HitGroupName			= L"HitGroup";
const wchar_t* RaytracingContext::m_RayGenShaderName		= L"RayGen";
const wchar_t* RaytracingContext::m_MissShaderName			= L"Miss";
const wchar_t* RaytracingContext::m_ClosestHitShaderName	= L"ClosestHit";
const wchar_t* RaytracingContext::m_ShadowHitGroupName		= L"ShadowHitGroup";
const wchar_t* RaytracingContext::m_ShadowMissName			= L"ShadowMiss";
const wchar_t* RaytracingContext::m_PlaneHitGroupName		= L"PlaneHitGroup";

RaytracingContext::RaytracingContext(DeviceContext* pDeviceCtx, ShaderManager* pShaderManager, Camera* pCamera, std::vector<VertexBuffer>& Vertex, std::vector<IndexBuffer>& Index, std::vector<ConstantBuffer<cbPerObject>>& ConstBuffers)
	: m_DeviceCtx(pDeviceCtx), m_ShaderManager(pShaderManager), m_Camera(pCamera), m_VertexBuffers(Vertex), m_IndexBuffers(Index), m_ConstBuffers(ConstBuffers)
{
	Create();
}

RaytracingContext::RaytracingContext(DeviceContext* pDeviceCtx, ShaderManager* pShaderManager, Camera* pCamera, std::vector<VertexBuffer>& Vertex, std::vector<IndexBuffer>& Index, std::vector<Texture> Textures)
	: m_DeviceCtx(pDeviceCtx), m_ShaderManager(pShaderManager), m_Camera(pCamera), m_VertexBuffers(Vertex), m_IndexBuffers(Index)
{
	m_Textures.insert(m_Textures.begin(), Textures.begin(), Textures.end());
	Create();
}

RaytracingContext::~RaytracingContext() noexcept(false)
{

	SAFE_RELEASE(m_GlobalRootSignature);
	SAFE_RELEASE(m_ClosestHitRootSignature);
	SAFE_RELEASE(m_MissRootSignature);
	SAFE_RELEASE(m_ShadowRootSignature);

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
	// Unused
	{
		//D3D12_DESCRIPTOR_HEAP_DESC desc{};
		//desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		//desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		//desc.NumDescriptors = 64;
		//m_RaytracingHeap = std::make_unique<DescriptorHeap>();
		//ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_RaytracingHeap->GetHeapAddressOf())));
		//m_RaytracingHeap->GetHeap()->SetName(L"Raytracing Heap");
	}

	// Const Buffers
	{
		m_SceneData = { XMMatrixIdentity() * m_Camera->GetViewProjection(),
			m_Camera->GetPosition(),
			XMVectorSet(0.0f, 1.5f, -8.0f, 1.0f),
			XMVectorSet(0.5f, 0.5f, 0.5f, 1.0f),
			XMVectorSet(0.960f, 0.416f, 0.416f, 1.0f)
		};
		//XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f)

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

	SetConstBufferData();


}

void RaytracingContext::OnRaytrace()
{
	UpdateCamera();

	DispatchRaytrace();
	OutputToBackbuffer();

	DrawGUI();
}

void RaytracingContext::DispatchRaytrace()
{
	//m_DeviceCtx->GetCommandList()->SetDescriptorHeaps(1, m_DeviceCtx->GetMainHeap()->GetHeapAddressOf());

	const auto dispatch = [&](ID3D12GraphicsCommandList4* pCommandList, const D3D12_DISPATCH_RAYS_DESC& Desc) {

		pCommandList->SetComputeRootSignature(m_GlobalRootSignature.Get());
		pCommandList->SetComputeRootDescriptorTable(GlobalRootArguments::eOutputUAV, m_OutputUAV.GetGPU());
		pCommandList->SetComputeRootShaderResourceView(GlobalRootArguments::eTopLevel, m_AS.m_TopLevel.m_ResultBuffer->GetGPUVirtualAddress());
		// CBVs
		pCommandList->SetComputeRootConstantBufferView(GlobalRootArguments::eCameraBuffer, m_CameraBuffer.GetBuffer(m_DeviceCtx->FRAME_INDEX)->GetGPUVirtualAddress());
		pCommandList->SetComputeRootConstantBufferView(GlobalRootArguments::eSceneBuffer, m_SceneBuffer.GetBuffer(m_DeviceCtx->FRAME_INDEX)->GetGPUVirtualAddress());
		// Geometry Buffers
		pCommandList->SetComputeRootDescriptorTable(GlobalRootArguments::eVertex, m_VertexBuffers.at(0).m_Descriptor.GetGPU());
		pCommandList->SetComputeRootDescriptorTable(GlobalRootArguments::eIndex, m_IndexBuffers.at(0).m_Descriptor.GetGPU());
		pCommandList->SetComputeRootDescriptorTable(6, m_Textures.at(0).m_Descriptor.GetGPU());
		// Execute
		pCommandList->SetPipelineState1(m_StateObject.Get());
		pCommandList->DispatchRays(&Desc);
		};

	D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
	{
		// RayGen
		dispatchDesc.RayGenerationShaderRecord.StartAddress = m_RayGenTable.GetAddressOf();
		dispatchDesc.RayGenerationShaderRecord.SizeInBytes  = m_RayGenTable.GetShaderRecordSize();
		// Miss
		dispatchDesc.MissShaderTable.StartAddress  = m_MissTable.GetAddressOf();
		dispatchDesc.MissShaderTable.SizeInBytes   = static_cast<uint64_t>(m_MissTable.GetShaderRecordSize());
		dispatchDesc.MissShaderTable.StrideInBytes = static_cast<uint64_t>(m_MissTable.Stride());
		// ClosestHit
		dispatchDesc.HitGroupTable.StartAddress  = m_HitTable.GetAddressOf();
		dispatchDesc.HitGroupTable.SizeInBytes	 = static_cast<uint64_t>(m_HitTable.GetShaderRecordSize() * m_HitTable.GetRecordsCount());
		dispatchDesc.HitGroupTable.StrideInBytes = static_cast<uint64_t>(m_HitTable.Stride());
		// Output dimensions
		dispatchDesc.Width  = static_cast<uint32_t>(m_DeviceCtx->GetViewport().Width);
		dispatchDesc.Height = static_cast<uint32_t>(m_DeviceCtx->GetViewport().Height);
		// Primary Rays
		dispatchDesc.Depth  = m_MaxRecursiveDepth;
	}

	// Execute
	dispatch(m_DeviceCtx->GetCommandList(), dispatchDesc);
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
	// Note: bindings in local Root Signatures CANNOT overlap with Global Root Signature

	// Global Root Signature
	{
		D3D12_ROOT_SIGNATURE_DESC desc{};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		
		std::array<CD3DX12_DESCRIPTOR_RANGE1, 5> ranges{};
		ranges.at(GlobalRootArguments::eOutputUAV).Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		ranges.at(GlobalRootArguments::eTopLevel).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		ranges.at(2).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		ranges.at(3).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		ranges.at(4).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 2, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		std::array<CD3DX12_ROOT_PARAMETER1, 7> params{};
		// UAV Output buffer
		params.at(GlobalRootArguments::eOutputUAV).InitAsDescriptorTable(1, &ranges.at(0), D3D12_SHADER_VISIBILITY_ALL);
		// TLAS buffer
		params.at(GlobalRootArguments::eTopLevel).InitAsShaderResourceView(0, 0);
		// CBVs
		params.at(GlobalRootArguments::eCameraBuffer).InitAsConstantBufferView(0, 0);
		params.at(GlobalRootArguments::eSceneBuffer).InitAsConstantBufferView(1, 0);
		// Geometry Buffers
		params.at(GlobalRootArguments::eVertex).InitAsDescriptorTable(1, &ranges.at(2));
		params.at(GlobalRootArguments::eIndex).InitAsDescriptorTable(1, &ranges.at(3));
		// Test
		params.at(6).InitAsDescriptorTable(1, &ranges.at(4));

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC vdesc{};
		vdesc.Init_1_1(static_cast<uint32_t>(params.size()), params.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

		RaytracingContext::SerializeAndCreateRootSignature(vdesc, m_GlobalRootSignature.GetAddressOf(), L"Raytracing Global Root Signature");
	}

	// Local Root Signatures
	
	// Miss
	{
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
		desc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

		RaytracingContext::SerializeAndCreateRootSignature(desc, m_MissRootSignature.GetAddressOf(), L"Miss Local Root Signature");
	}

	// ClosestHit
	{
		std::array<CD3DX12_DESCRIPTOR_RANGE1, 1> ranges{};
		ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 2, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		std::array<CD3DX12_ROOT_PARAMETER1, 1> params{};
		params.at(LocalRootArguments::eAlbedo).InitAsConstants(sizeof(XMVECTOR), 0, 2); 
		//params.at(1).InitAsDescriptorTable(1, &ranges.at(0));

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
		desc.Init_1_1(static_cast<uint32_t>(params.size()), params.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

		RaytracingContext::SerializeAndCreateRootSignature(desc, m_ClosestHitRootSignature.GetAddressOf(), L"ClosestHit Local Root Signature");
	}

	// Shadows
	{
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
		desc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

		RaytracingContext::SerializeAndCreateRootSignature(desc, m_ShadowRootSignature.GetAddressOf(), L"Shadows Local Root Signature");
	}
}

void RaytracingContext::CreateStateObject()
{
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	// DXIL
	m_RayGenShader	= m_ShaderManager->CreateDXIL("Assets/Raytracing/RayGen.hlsl", L"lib_6_3");
	m_MissShader	= m_ShaderManager->CreateDXIL("Assets/Raytracing/Miss.hlsl", L"lib_6_3");
	m_HitShader		= m_ShaderManager->CreateDXIL("Assets/Raytracing/HitRay.hlsl", L"lib_6_3");
	//m_HitShader		= m_ShaderManager->CreateDXIL("Assets/Raytracing/Hit_Test.hlsl", L"lib_6_3");
	m_ShadowShader	= m_ShaderManager->CreateDXIL("Assets/Raytracing/ShadowRay.hlsl", L"lib_6_3");

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
		std::vector<LPCWSTR> exports{ m_MissShaderName };
		missLib->DefineExports(exports.data(), static_cast<uint32_t>(exports.size()));
	}

	// Ray Closest Hit 
	auto hitLib{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	{
		const auto hitBytecode{ CD3DX12_SHADER_BYTECODE(m_HitShader->GetBufferPointer(), m_HitShader->GetBufferSize()) };
		hitLib->SetDXILLibrary(&hitBytecode);
		std::vector<LPCWSTR> exports{ m_ClosestHitShaderName, L"PlaneClosestHit" };
		hitLib->DefineExports(exports.data(), static_cast<uint32_t>(exports.size()));
	}

	// Ray Shadow
	auto shadowLib{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	{
		const auto shadowBytecode{ CD3DX12_SHADER_BYTECODE(m_ShadowShader->GetBufferPointer(), m_ShadowShader->GetBufferSize()) };
		shadowLib->SetDXILLibrary(&shadowBytecode);
		//L"ShadowClosestHit", 
		std::vector<LPCWSTR> exports{ m_ShadowMissName };
		shadowLib->DefineExports(exports.data(), static_cast<uint32_t>(exports.size()));
	}
	
	// HitGroup
	auto hitGroup{ raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>() };
	{
		hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
		hitGroup->SetHitGroupExport(m_HitGroupName);
		hitGroup->SetClosestHitShaderImport(m_ClosestHitShaderName);
	}

	// PlaneHitGroup
	auto planeHitGroup{ raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>() };
	{
		planeHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
		planeHitGroup->SetHitGroupExport(m_PlaneHitGroupName);
		planeHitGroup->SetClosestHitShaderImport(L"PlaneClosestHit");
	}

	// ShadowHitGroup
	// Note: Shadow Rays don't actually require ClosestHit shader to set hit value to true
	// Miss shader Payload is sufficient
	auto shadowHitGroup{ raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>() };
	{
		shadowHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
		shadowHitGroup->SetHitGroupExport(m_ShadowHitGroupName);
		// Not required
		//shadowHitGroup->SetClosestHitShaderImport(L"ShadowClosestHit");
	}

	// Shader Config
	auto shaderConfig{ raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>() };
	{
		m_PayloadSize = static_cast<uint32_t>(4 * sizeof(float));
		m_AttributeSize = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
		shaderConfig->Config(m_PayloadSize, m_AttributeSize);
	}

	auto pipelineConfig{ raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>() };
	pipelineConfig->Config(m_MaxRecursiveDepth);

	auto missSignature{ raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>() };
	missSignature->SetRootSignature(m_MissRootSignature.Get());

	auto missAssociation{ raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>() };
	{
		missAssociation->SetSubobjectToAssociate(*missSignature);
		missAssociation->AddExport(m_MissShaderName);
		missAssociation->AddExport(m_ShadowMissName);
	}

	// aka ClosestHit Local Root Signature
	auto hitSignature{ raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>() };
	hitSignature->SetRootSignature(m_ClosestHitRootSignature.Get());
	
	auto hitAssociation{ raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>() };
	{
		hitAssociation->SetSubobjectToAssociate(*hitSignature);
		std::vector<LPCWSTR> exports{ m_HitGroupName, m_PlaneHitGroupName };
		hitAssociation->AddExports(exports.data(), static_cast<uint32_t>(exports.size()));
	}

	// Shadow Root Signature
	auto shadowSignature{ raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>() };
	shadowSignature->SetRootSignature(m_ShadowRootSignature.Get());

	auto shadowAssociation{ raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>() };
	{
		shadowAssociation->SetSubobjectToAssociate(*shadowSignature);
		shadowAssociation->AddExport(m_ShadowHitGroupName);
	}

	auto globalRootSignature{ raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>() };
	globalRootSignature->SetRootSignature(m_GlobalRootSignature.Get());

	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(m_StateObject.ReleaseAndGetAddressOf())));
	ThrowIfFailed(m_StateObject->QueryInterface(m_StateObjectProperties.ReleaseAndGetAddressOf()));
	m_StateObject.Get()->SetName(L"Raytracing State Object");

}

void RaytracingContext::CreateOutputResource()
{
	const auto device{ m_DeviceCtx->GetDevice() };

	const auto uavDesc{ CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 
		static_cast<uint64_t>(m_DeviceCtx->GetViewport().Width), 
		static_cast<uint32_t>(m_DeviceCtx->GetViewport().Height), 
		1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) };

	const auto defaultHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_RaytracingOutput.ReleaseAndGetAddressOf())));
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
	m_AS.CreateBottomLevels(m_VertexBuffers, m_IndexBuffers, true);
	m_AS.CreateTopLevel();

	m_DeviceCtx->ExecuteCommandLists(true);
	m_DeviceCtx->WaitForGPU();
	//m_DeviceCtx->FlushGPU();
}

void RaytracingContext::BuildShaderTables()
{
	constexpr uint32_t shaderIdentifierSize{ D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };
	
	const D3D12_GPU_DESCRIPTOR_HANDLE heapHandle{ m_DeviceCtx->GetMainHeap()->GetHeap()->GetGPUDescriptorHandleForHeapStart() };
	auto* heapPointer{ reinterpret_cast<uint64_t*>(heapHandle.ptr) };
	
	void* rayGenIdentifier	    { m_StateObjectProperties.Get()->GetShaderIdentifier(m_RayGenShaderName) };
	void* missIdentifier	    { m_StateObjectProperties.Get()->GetShaderIdentifier(m_MissShaderName) };
	void* hitIdentifier		    { m_StateObjectProperties.Get()->GetShaderIdentifier(m_HitGroupName) };

	void* shadowHitIdentifier	{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_ShadowHitGroupName) };
	void* shadowMissIdentifier	{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_ShadowMissName) };

	void* planeHitIdentifier	{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_PlaneHitGroupName) };
	
	// https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial/extra/dxr_tutorial_extra_another_ray_type

	// RayGen Table
	{
		struct GlobalArgs
		{
			void* pHeap;
		} globalArgs{};

		globalArgs.pHeap = &heapPointer;

		m_RayGenTable.Create(m_DeviceCtx->GetDevice(), 1, shaderIdentifierSize, L"RayGen Shader Table");
		TableRecord record(rayGenIdentifier, shaderIdentifierSize, &heapPointer, sizeof(heapPointer));

		m_RayGenTable.AddRecord(record);
	}

	// Miss Table
	{
		const uint32_t shaderRecordSize{ shaderIdentifierSize + static_cast<uint32_t>(sizeof(heapPointer)) };

		m_MissTable.Create(m_DeviceCtx->GetDevice(), 2, shaderRecordSize, L"Miss Shader Table");
		m_MissTable.SetStride(shaderIdentifierSize);
		
		TableRecord missRecord(missIdentifier, shaderIdentifierSize, &heapPointer, sizeof(heapPointer));
		m_MissTable.AddRecord(missRecord);

		TableRecord shadowMissRecord(shadowMissIdentifier, shaderIdentifierSize, &heapPointer, sizeof(heapPointer));
		m_MissTable.AddRecord(shadowMissRecord);

		m_MissTable.CheckAlignment();
	}

	// Hit Table
	{
		// ClosestHitGroup
		struct LocalArgs
		{
			XMVECTOR Albedo;
			uint64_t* pHeap;
			uint64_t* pTexture;
		} localArgs{};

		localArgs.Albedo = m_CubeData.CubeColor;
		localArgs.pHeap		= heapPointer;
		localArgs.pTexture	= reinterpret_cast<uint64_t*>(m_Textures.at(0).m_Descriptor.GetGPU().ptr);

		const uint32_t shaderRecordSize{ shaderIdentifierSize + static_cast<uint32_t>(sizeof(localArgs)) };
		//
		m_HitTable.Create(m_DeviceCtx->GetDevice(), 3, shaderRecordSize, L"Hit Shader Table");

		TableRecord hitRecord(hitIdentifier, shaderIdentifierSize, &localArgs, sizeof(localArgs));
		m_HitTable.AddRecord(hitRecord);

		TableRecord shadowHitRecord(shadowHitIdentifier, shaderIdentifierSize);
		m_HitTable.AddRecord(shadowHitRecord);

		TableRecord planeHitRecord(planeHitIdentifier, shaderIdentifierSize);
		m_HitTable.AddRecord(planeHitRecord);

		m_HitTable.CheckAlignment();
	}

}
 
//void RaytracingContext::SerializeAndCreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3D12RootSignature** ppRootSignature, LPCWSTR DebugName) const
void RaytracingContext::SerializeAndCreateRootSignature(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC& Desc, ID3D12RootSignature** ppRootSignature, LPCWSTR DebugName) const
{
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	//D3D12SerializeVersionedRootSignature()
	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&Desc, signature.GetAddressOf(), error.GetAddressOf()));
	//ThrowIfFailed(D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf()));
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
		0, 
		signature->GetBufferPointer(), 
		signature->GetBufferSize(), 
		IID_PPV_ARGS(&(*(ppRootSignature)))));
	
	if (DebugName)
		(*(ppRootSignature))->SetName(DebugName);

	SAFE_RELEASE(signature);
	SAFE_RELEASE(error);
}

void RaytracingContext::SetConstBufferData()
{
	m_SceneData.LightDiffuse = { 0.960f, 0.416f, 0.416f, 1.0f };
	m_SceneData.ViewProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, m_Camera->GetViewProjection()));
	m_SceneData.CameraPosition = m_Camera->GetPosition();

	m_SceneBuffer.Update(m_SceneData, m_DeviceCtx->FRAME_INDEX);
		
	//m_CubeData = { XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f) };
	//m_CubeBuffer.Update(m_CubeData, m_DeviceCtx->FRAME_INDEX);
}

void RaytracingContext::DrawGUI()
{
	ImGui::Begin("Raytrace Lighting");

	if (ImGui::DragFloat3("Position", m_LightPosition.data()))
	{
		m_SceneData.LightPosition = XMVectorSet(m_LightPosition.at(0), m_LightPosition.at(1), m_LightPosition.at(2), 1.0f);
	}

	if (ImGui::ColorEdit4("Ambient", m_LightAmbient.data()), ImGuiColorEditFlags_Float)
	{
		m_SceneData.LightAmbient = XMVectorSet(m_LightAmbient.at(0), m_LightAmbient.at(1), m_LightAmbient.at(2), m_LightAmbient.at(3));
	}

	if (ImGui::ColorEdit4("Diffuse", m_LightDiffuse.data()), ImGuiColorEditFlags_Float)
	{
		m_SceneData.LightDiffuse = XMVectorSet(m_LightDiffuse.at(0), m_LightDiffuse.at(1), m_LightDiffuse.at(2), m_LightDiffuse.at(3));
	}

	ImGui::End();
}

void RaytracingContext::OnResize()
{
	const auto device{ m_DeviceCtx->GetDevice() };

	const auto uavDesc{ CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		static_cast<uint64_t>(m_DeviceCtx->GetViewport().Width),
		static_cast<uint32_t>(m_DeviceCtx->GetViewport().Height),
		1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) };

	const auto defaultHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_RaytracingOutput.ReleaseAndGetAddressOf())));
	m_RaytracingOutput.Get()->SetName(L"Raytracing Output");

	//m_DeviceCtx->GetMainHeap()->Allocate(m_OutputUAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(m_RaytracingOutput.Get(), nullptr, &UAVDesc, m_OutputUAV.GetCPU());
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

	m_SceneData.ViewProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, m_Camera->GetViewProjection()));
	m_SceneData.CameraPosition = m_Camera->GetPosition();
	m_SceneBuffer.Update(m_SceneData, m_DeviceCtx->FRAME_INDEX);
}
