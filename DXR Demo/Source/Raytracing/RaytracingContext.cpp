#include "RaytracingContext.hpp"
#include <array>
#include "../Utilities/Utilities.hpp"

const wchar_t* RaytracingContext::c_HitGroupName			= L"HitGroup";
const wchar_t* RaytracingContext::c_RaygenShaderName		= L"Raygen";
const wchar_t* RaytracingContext::c_ClosestHitShaderName	= L"ClosestHit";
const wchar_t* RaytracingContext::c_MissShaderName			= L"Miss";

RaytracingContext::RaytracingContext(DeviceContext* pDeviceCtx)
{
	m_DeviceCtx = pDeviceCtx;
	assert(m_DeviceCtx);
	//assert((m_dxrDevice = pDeviceCtx->GetDevice()) && "Failed to get DXR Device!");
	//assert((m_dxrCommandList = pDeviceCtx->GetCommandList()) && "Failed to get DXR Graphics Command List!");

	m_RaytracingBackbuffer.BackbufferFormat = pDeviceCtx->GetRenderTargetFormat();
	m_RaytracingBackbuffer.Width = static_cast<uint64_t>(pDeviceCtx->GetViewport().Width);
	m_RaytracingBackbuffer.Height = static_cast<uint32_t>(pDeviceCtx->GetViewport().Height);

	Create();
}

RaytracingContext::~RaytracingContext()
{
}

void RaytracingContext::Create()
{
	//Consts
	const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Allocate one constant buffer per frame, since it gets updated every frame.
	size_t cbSize = 2 * sizeof(AlignedSceneConstantBuffer);
	const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_perFrameConstants)));

	// Map the constant buffer and cache its heap pointers.
	// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_perFrameConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantData)));


	BuildGeometry();

	CreateRootSignatures();

	CreateStateObject();

	BuildAccelerationStructures();
	
	BuildShaderTables();

	CreateOutputResource();
}

void RaytracingContext::OnRaytrace()
{
	DoRaytrace();
	OutputToBackbuffer();
}

void RaytracingContext::DoRaytrace()
{
	auto commandList = m_DeviceCtx->GetCommandList();
	auto frameIndex = m_DeviceCtx->FRAME_INDEX;

	auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
	{
		// Since each shader table has only one shader record, the stride is same as the size.
		dispatchDesc->HitGroupTable.StartAddress = m_HitGroupShaderTable->GetGPUVirtualAddress();
		dispatchDesc->HitGroupTable.SizeInBytes = m_HitGroupShaderTable->GetDesc().Width;
		dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
		dispatchDesc->MissShaderTable.StartAddress = m_MissShaderTable->GetGPUVirtualAddress();
		dispatchDesc->MissShaderTable.SizeInBytes = m_MissShaderTable->GetDesc().Width;
		dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
		dispatchDesc->RayGenerationShaderRecord.StartAddress = m_RayGenShaderTable->GetGPUVirtualAddress();
		dispatchDesc->RayGenerationShaderRecord.SizeInBytes = m_RayGenShaderTable->GetDesc().Width;
		dispatchDesc->Width  = m_RaytracingBackbuffer.Width;
		dispatchDesc->Height = m_RaytracingBackbuffer.Height;
		dispatchDesc->Depth = 1;
		commandList->SetPipelineState1(stateObject);
		commandList->DispatchRays(dispatchDesc);
	};

	auto SetCommonPipelineState = [&](auto* descriptorSetCommandList)
	{
		//descriptorSetCommandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
		descriptorSetCommandList->SetDescriptorHeaps(1, m_DeviceCtx->GetMainHeap()->GetHeapAddressOf());
		// Set index and successive vertex buffer decriptor tables
		//commandList->SetComputeRootDescriptorTable(3, m_RTCube.Index.Buffer.m_Descriptor.GetGPU());
		commandList->SetComputeRootDescriptorTable(3, m_RTCube.Index.Buffer.DescriptorSRV.GetGPU());
		commandList->SetComputeRootDescriptorTable(0, m_DescriptorUAV.GetGPU());
	};

	commandList->SetComputeRootSignature(m_GlobalRootSignature.Get());

	// Copy the updated scene constant buffer to GPU.
	//memcpy(&m_mappedConstantData[frameIndex].constants, &m_SceneCB[frameIndex], sizeof(m_SceneCB[frameIndex]));
	//std::memcpy(&m_mappedConstantData[frameIndex].constants, m_SceneCB.GetData(frameIndex), sizeof(m_SceneCB.GetData(frameIndex)));
	std::memcpy(&m_mappedConstantData[frameIndex].constants, &m_SceneCB[frameIndex], sizeof(m_SceneCB[frameIndex]));
	auto cbGpuAddress = m_perFrameConstants->GetGPUVirtualAddress() + frameIndex * sizeof(m_mappedConstantData[0]);
	commandList->SetComputeRootConstantBufferView(2, cbGpuAddress);

	// Bind the heaps, acceleration structure and dispatch rays.
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	SetCommonPipelineState(commandList);
	commandList->SetComputeRootShaderResourceView(1, m_TopLevelAccelerationStructure->GetGPUVirtualAddress());
	DispatchRays(commandList, m_StateObject.Get(), &dispatchDesc);
}

void RaytracingContext::CreateRootSignatures()
{
	// Global Root Signature
	{
		std::array<CD3DX12_DESCRIPTOR_RANGE, 2> ranges{};
		ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		ranges.at(1).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);

		std::array<CD3DX12_ROOT_PARAMETER, 4> parameters{};
		parameters.at(0).InitAsDescriptorTable(1, &ranges.at(0));
		parameters.at(1).InitAsShaderResourceView(0);
		parameters.at(2).InitAsConstantBufferView(0);
		parameters.at(3).InitAsDescriptorTable(1, &ranges.at(1));

		CD3DX12_ROOT_SIGNATURE_DESC desc(static_cast<uint32_t>(parameters.size()), parameters.data());
		SerializeAndCreateRootSignature(desc, &m_GlobalRootSignature);
	}
	// Local Root Signature
	{
		//SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)
		//auto size{ static_cast<uint32_t>(((sizeof(m_CubeCB) - 1) / sizeof(uint32_t) + 1)) };
		auto size{ static_cast<uint32_t>(((sizeof(m_RTCube.m_cubeConst) - 1) / sizeof(uint32_t) + 1)) };
		std::array<CD3DX12_ROOT_PARAMETER, 1> parameters{};
		parameters.at(0).InitAsConstants(size, 1);

		CD3DX12_ROOT_SIGNATURE_DESC desc(static_cast<uint32_t>(parameters.size()), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
		SerializeAndCreateRootSignature(desc, &m_LocalRootSignature);
	}
}

void RaytracingContext::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* pRaytracingPipeline)
{
	auto localRootSignature{ pRaytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>() };
	localRootSignature->SetRootSignature(m_LocalRootSignature.Get());

	auto localRootAssociation{ pRaytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>() };
	localRootAssociation->SetSubobjectToAssociate(*localRootSignature);
	localRootAssociation->AddExport(c_HitGroupName);

}

void RaytracingContext::CreateStateObject()
{
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


	m_RaytracingBlob = Shader::CreateDXIL("Assets/Raytracing/RayTracing.hlsl");

	auto library{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	auto libdxil{ CD3DX12_SHADER_BYTECODE(m_RaytracingBlob->GetBufferPointer(), m_RaytracingBlob->GetBufferSize()) };
	library->SetDXILLibrary(&libdxil);
	{
		library->DefineExport(c_RaygenShaderName);
		library->DefineExport(c_ClosestHitShaderName);
		library->DefineExport(c_MissShaderName);
	}

	auto hitGroup{ raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>() };
	hitGroup->SetClosestHitShaderImport(c_ClosestHitShaderName);
	hitGroup->SetHitGroupExport(c_HitGroupName);
	hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	auto shaderConfig{ raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>() };
	m_PayloadSize	= static_cast<uint32_t>(sizeof(XMFLOAT4));
	m_AttributeSize = static_cast<uint32_t>(sizeof(XMFLOAT2));
	shaderConfig->Config(m_PayloadSize, m_AttributeSize);

	CreateLocalRootSignatureSubobjects(&raytracingPipeline);

	auto globalRootSignature{ raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>() };
	globalRootSignature->SetRootSignature(m_GlobalRootSignature.Get());

	auto pipelineConfig{ raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>() };
	pipelineConfig->Config(m_MaxRecursiveDepth);


	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(m_StateObject.GetAddressOf())));
	m_StateObject.Get()->SetName(L"Raytracing State Object");
}

void RaytracingContext::CreateOutputResource()
{
	auto device{ m_DeviceCtx->GetDevice() };

	auto uavDesc{ CD3DX12_RESOURCE_DESC::Tex2D(m_RaytracingBackbuffer.BackbufferFormat, m_RaytracingBackbuffer.Width, m_RaytracingBackbuffer.Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) };

	auto defaultHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_RaytracingOutput.GetAddressOf())));
	m_RaytracingOutput.Get()->SetName(L"Raytracing Output");

	m_DeviceCtx->GetMainHeap()->Allocate(m_DescriptorUAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(m_RaytracingOutput.Get(), nullptr, &UAVDesc, m_DescriptorUAV.GetCPU());

}

void RaytracingContext::BuildGeometry()
{
	std::vector<uint32_t> indices =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};

	// Cube vertices positions and corresponding triangle normals.
	std::vector<CubeNormal> vertices =
	{
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
	};

	//m_RTCube.Vertex.Create(m_DeviceCtx, vertices, true);
	//m_RTCube.Index.Create(m_DeviceCtx->GetDevice(), indices);

	{
		m_RTCube.Vertex.Buffer.Create(m_DeviceCtx, {}, { vertices.data(), static_cast<uint32_t>(vertices.size()), sizeof(vertices.at(0)) * vertices.size() });
		m_RTCube.Vertex.BufferView.Set(&m_RTCube.Vertex.Buffer);
	}
	{
		m_RTCube.Index.Buffer.Create(m_DeviceCtx, {}, { indices.data(), static_cast<uint32_t>(indices.size()), indices.size() * sizeof(uint32_t) });
		m_RTCube.Index.BufferView.Set(&m_RTCube.Index.Buffer);
	}

	// SRV here
	{
		BufferUtils::CreateSRV(m_DeviceCtx, &m_RTCube.Vertex.Buffer);
		BufferUtils::CreateSRV(m_DeviceCtx, &m_RTCube.Index.Buffer);
	}
}

void RaytracingContext::BuildAccelerationStructures()
{
	m_DeviceCtx->GetCommandList()->Reset(m_DeviceCtx->GetCommandAllocator(0), nullptr);

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

	geometryDesc.Triangles.IndexBuffer	= m_RTCube.Index.BufferView.BufferView.BufferLocation;
	geometryDesc.Triangles.IndexCount	= m_RTCube.Index.BufferView.Count;
	geometryDesc.Triangles.IndexFormat	= m_RTCube.Index.BufferView.BufferView.Format;

	geometryDesc.Triangles.VertexBuffer.StartAddress	= m_RTCube.Vertex.BufferView.BufferView.BufferLocation;
	geometryDesc.Triangles.VertexBuffer.StrideInBytes	= m_RTCube.Vertex.BufferView.BufferView.StrideInBytes;
	geometryDesc.Triangles.VertexCount					= m_RTCube.Vertex.Buffer.GetData().ElementsCount;
	geometryDesc.Triangles.VertexFormat					= DXGI_FORMAT_R32G32B32_FLOAT;

	geometryDesc.Triangles.Transform3x4 = 0;

	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags{ D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE };

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc{};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs{ bottomLevelBuildDesc.Inputs };
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.Flags = buildFlags;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geometryDesc;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc{};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs{ topLevelBuildDesc.Inputs }; 
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.pGeometryDescs = nullptr;


	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo{};
	m_DeviceCtx->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	assert(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo{};
	m_DeviceCtx->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
	assert(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

	ComPtr<ID3D12Resource> scratchResource;
	BufferUtils::CreateUAV(m_DeviceCtx, scratchResource.GetAddressOf(), std::max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes));

	{
		D3D12_RESOURCE_STATES initialResourceState{ D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE };

		BufferUtils::CreateUAV(m_DeviceCtx, m_BottomLevelAccelerationStructure.GetAddressOf(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
		BufferUtils::CreateUAV(m_DeviceCtx, m_TopLevelAccelerationStructure.GetAddressOf(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	}

	ComPtr<ID3D12Resource> instanceDescs;
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	instanceDesc.AccelerationStructure = m_BottomLevelAccelerationStructure->GetGPUVirtualAddress();

	BufferUtils::UploadBuffer(m_DeviceCtx, instanceDescs.GetAddressOf(), &instanceDesc, sizeof(instanceDesc));

	// Bottom Level Acceleration Structure desc
	{
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = m_BottomLevelAccelerationStructure.Get()->GetGPUVirtualAddress();
	}

	// Top Level Acceleration Structure desc
	{
		topLevelBuildDesc.DestAccelerationStructureData = m_TopLevelAccelerationStructure.Get()->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
	}

	m_DeviceCtx->GetCommandList()->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
	auto barrier{ CD3DX12_RESOURCE_BARRIER::UAV(m_BottomLevelAccelerationStructure.Get()) };
	m_DeviceCtx->GetCommandList()->ResourceBarrier(1, &barrier);
	m_DeviceCtx->GetCommandList()->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	m_DeviceCtx->ExecuteCommandLists();

	// Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
	m_DeviceCtx->WaitForGPU();

}

void RaytracingContext::BuildShaderTables()
{
	void* rayGenIdentifier	{ nullptr };
	void* missIdentifier	{ nullptr };
	void* hitGroupIdentifier{ nullptr };

	auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
	{
		rayGenIdentifier = stateObjectProperties->GetShaderIdentifier(c_RaygenShaderName);
		missIdentifier = stateObjectProperties->GetShaderIdentifier(c_MissShaderName);
		hitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(c_HitGroupName);
	};

	uint32_t shaderIdentifierSize{ };
	{
		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		ThrowIfFailed(m_StateObject.As(&stateObjectProperties));

		GetShaderIdentifiers(stateObjectProperties.Get());
		shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}

	// Ray Gen Table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;

		ShaderTable rayGenShaderTable(m_DeviceCtx->GetDevice(), numShaderRecords, shaderRecordSize);

		ShaderRecord newRecord(rayGenIdentifier, shaderIdentifierSize);
		rayGenShaderTable.AddRecord(newRecord);
		m_RayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss Table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(m_DeviceCtx->GetDevice(), numShaderRecords, shaderRecordSize);

		ShaderRecord newRecord(rayGenIdentifier, shaderIdentifierSize);
		missShaderTable.AddRecord(newRecord);

		m_MissShaderTable = missShaderTable.GetResource();
	}

	// Hit Group Table
	{ 
		struct RootArguments {
			CubeConstant cb;
		} rootArguments;
		rootArguments.cb = m_RTCube.m_cubeConst;

		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable hitGroupShaderTable(m_DeviceCtx->GetDevice(), numShaderRecords, shaderRecordSize);

		ShaderRecord shaderRecord(rayGenIdentifier, shaderIdentifierSize);
		hitGroupShaderTable.AddRecord(shaderRecord);

		m_HitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
}

void RaytracingContext::OutputToBackbuffer()
{
	auto renderTarget = m_DeviceCtx->GetRenderTarget(m_DeviceCtx->FRAME_INDEX);

	D3D12_RESOURCE_BARRIER preCopyBarriers[2]{};
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_RaytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_DeviceCtx->GetCommandList()->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

	m_DeviceCtx->GetCommandList()->CopyResource(renderTarget, m_RaytracingOutput.Get());
	
	D3D12_RESOURCE_BARRIER postCopyBarriers[2]{};
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_RaytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_DeviceCtx->GetCommandList()->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
}

void RaytracingContext::SerializeAndCreateRootSignature(D3D12_ROOT_SIGNATURE_DESC& Desc, ComPtr<ID3D12RootSignature>* ppRootSignature)
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> error;

	ThrowIfFailed(D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, blob.GetAddressOf(), error.GetAddressOf()));
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*(ppRootSignature)))));
}
