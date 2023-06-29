#include "RaytracingContext.hpp"
#include <array>
#include "../Utilities/Utilities.hpp"

const wchar_t* RaytracingContext::m_HitGroupName			= L"HitGroup";
const wchar_t* RaytracingContext::m_RayGenShaderName		= L"RayGen";
const wchar_t* RaytracingContext::m_MissShaderName			= L"Miss";
const wchar_t* RaytracingContext::m_ClosestHitShaderName	= L"ClosestHit";

RaytracingContext::RaytracingContext(DeviceContext* pDeviceCtx, VertexBuffer& Vertex, IndexBuffer& Index)
{
	{
		m_DeviceCtx = pDeviceCtx;
		assert(m_DeviceCtx);

		m_VertexBuffer = Vertex;
		m_IndexBuffer = Index;
	}

	Create();
}

RaytracingContext::~RaytracingContext()
{
	SAFE_RELEASE(m_RayGenSignature);
	SAFE_RELEASE(m_MissSignature);
	SAFE_RELEASE(m_HitSignature);
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

	
	CreateRootSignatures();

	CreateStateObject();

	BuildAccelerationStructures();

	BuildShaderTables();

	CreateOutputResource();

}

void RaytracingContext::OnRaytrace()
{
	DispatchRaytrace();
	OutputToBackbuffer();
}

void RaytracingContext::DispatchRaytrace()
{
	auto commandList = m_DeviceCtx->GetCommandList();

	commandList->SetDescriptorHeaps(1, m_RaytracingHeap->GetHeapAddressOf());

	commandList->SetComputeRootSignature(m_RayGenSignature.Get());
	commandList->SetComputeRootDescriptorTable(0, m_OutputUAV.GetGPU());
	commandList->SetComputeRootShaderResourceView(1, m_AS.m_TopLevel.m_ResultBuffer->GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
	// RayGen
	dispatchDesc.RayGenerationShaderRecord.StartAddress = m_RayGenTable.GetStorage()->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes	= m_RayGenTable.GetShaderRecordSize();
	// Miss
	dispatchDesc.MissShaderTable.StartAddress	= m_MissTable.GetStorage()->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes	= m_MissTable.GetShaderRecordSize();
	dispatchDesc.MissShaderTable.StrideInBytes	= m_MissTable.GetShaderRecordSize();
	// Hit
	dispatchDesc.HitGroupTable.StartAddress  = m_HitTable.GetStorage()->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes	 = m_HitTable.GetShaderRecordSize();
	dispatchDesc.HitGroupTable.StrideInBytes = m_HitTable.GetShaderRecordSize();
	// Output dimensions
	dispatchDesc.Width  = static_cast<uint32_t>(m_DeviceCtx->GetViewport().Width);
	dispatchDesc.Height = static_cast<uint32_t>(m_DeviceCtx->GetViewport().Height);
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
	/*
	// Local and Global Root Signatures
	{
		D3D12_ROOT_SIGNATURE_DESC desc{};
		desc.NumParameters = 0;
		desc.pParameters = nullptr;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		ComPtr<ID3DBlob> rootSignature;
		ComPtr<ID3DBlob> error;

		ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, rootSignature.GetAddressOf(), error.GetAddressOf()));

		ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(0, rootSignature->GetBufferPointer(), rootSignature->GetBufferSize(), IID_PPV_ARGS(m_LocalRootSignature.GetAddressOf())));
		m_LocalRootSignature.Get()->SetName(L"Raytracing Local Root Signature");

		rootSignature->Release();
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		//test
		std::array<CD3DX12_DESCRIPTOR_RANGE, 1> ranges{};
		ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		std::array<CD3DX12_ROOT_PARAMETER, 1> params{};
		params.at(0).InitAsDescriptorTable(1, &ranges.at(0), D3D12_SHADER_VISIBILITY_ALL);

		desc.NumParameters = static_cast<uint32_t>(params.size());
		desc.pParameters = params.data();

		ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, rootSignature.GetAddressOf(), error.GetAddressOf()));

		ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(0, rootSignature->GetBufferPointer(), rootSignature->GetBufferSize(), IID_PPV_ARGS(m_GlobalRootSignature.GetAddressOf())));
		m_GlobalRootSignature.Get()->SetName(L"Raytracing Global Root Signature");

		rootSignature->Release();
	}
	*/
	
	// Shader Root Signatures
	
	// RayGen
	{
		D3D12_ROOT_SIGNATURE_DESC desc{};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		std::array<CD3DX12_DESCRIPTOR_RANGE, 1> ranges{};
		ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		std::array<CD3DX12_ROOT_PARAMETER, 2> params{};
		// Output buffer
		params.at(0).InitAsDescriptorTable(1, &ranges.at(0), D3D12_SHADER_VISIBILITY_ALL);
		// TLAS buffer
		params.at(1).InitAsShaderResourceView(0);

		desc.NumParameters = static_cast<uint32_t>(params.size());
		desc.pParameters = params.data();

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(
			&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(m_RayGenSignature.GetAddressOf())));
		m_RayGenSignature.Get()->SetName(L"RayGen Root Signature");
	}
	// Miss
	{
		D3D12_ROOT_SIGNATURE_DESC desc{};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		desc.NumParameters = 0;
		desc.pParameters = nullptr;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(
			&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(m_MissSignature.GetAddressOf())));
		m_MissSignature.Get()->SetName(L"Miss Root Signature");
	}
	// Hit
	{
		D3D12_ROOT_SIGNATURE_DESC desc{};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		std::array<CD3DX12_DESCRIPTOR_RANGE, 1> ranges{};
		ranges.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		std::array<CD3DX12_ROOT_PARAMETER, 1> params{};
		params.at(0).InitAsShaderResourceView(0, 1);

		desc.NumParameters = static_cast<uint32_t>(params.size());
		desc.pParameters = params.data();

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(
			&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(m_HitSignature.GetAddressOf())));
		m_HitSignature.Get()->SetName(L"Hit Root Signature");
	}

	/*
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
		std::array<CD3DX12_ROOT_PARAMETER, 1> parameters{};
		//parameters.at(0).initas
		//parameters.at(0).InitAsConstants(size, 1);

		CD3DX12_ROOT_SIGNATURE_DESC desc(static_cast<uint32_t>(parameters.size()), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
		SerializeAndCreateRootSignature(desc, &m_LocalRootSignature);
	}
	*/
}

void RaytracingContext::CreateStateObject()
{
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	// DXIL
	m_RayGenShader	= Shader::CreateDXIL("Assets/Raytracing/RayGen.hlsl");
	m_MissShader	= Shader::CreateDXIL("Assets/Raytracing/Miss.hlsl");
	m_HitShader		= Shader::CreateDXIL("Assets/Raytracing/Hit.hlsl");

	// RayGen
	auto raygenLib{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	{
		auto raygenBytecode{ CD3DX12_SHADER_BYTECODE(m_RayGenShader->GetBufferPointer(), m_RayGenShader->GetBufferSize()) };
		raygenLib->SetDXILLibrary(&raygenBytecode);
		raygenLib->DefineExport(m_RayGenShaderName);
	}

	// Miss
	auto missLib{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	{
		auto missBytecode{ CD3DX12_SHADER_BYTECODE(m_MissShader->GetBufferPointer(), m_MissShader->GetBufferSize()) };
		missLib->SetDXILLibrary(&missBytecode);
		missLib->DefineExport(m_MissShaderName);
	}

	// Closest Hit
	auto hitLib{ raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>() };
	{
		auto hitBytecode{ CD3DX12_SHADER_BYTECODE(m_HitShader->GetBufferPointer(), m_HitShader->GetBufferSize()) };
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

	auto hitSignature{ raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>() };
	hitSignature->SetRootSignature(m_HitSignature.Get());

	auto hitAssociation{ raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>() };
	hitAssociation->SetSubobjectToAssociate(*hitSignature);
	hitAssociation->AddExport(m_ClosestHitShaderName);

	auto rayGenSignature{ raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>() };
	rayGenSignature->SetRootSignature(m_RayGenSignature.Get());

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

	m_RaytracingHeap->Allocate(m_OutputUAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(m_RaytracingOutput.Get(), nullptr, &UAVDesc, m_OutputUAV.GetCPU());

	m_RaytracingHeap->Allocate(m_TopLevelView);
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

	m_AS.CreateBottomLevel(m_VertexBuffer, m_IndexBuffer, true);
	DirectX::XMMATRIX matrix{ DirectX::XMMatrixIdentity() };
	m_AS.CreateTopLevel(m_AS.m_BottomLevel.m_ResultBuffer.Get(), matrix);

	// Perhaps unnecessary for now
	//m_DeviceCtx->WaitForGPU();
	//m_DeviceCtx->FlushGPU();

}

void RaytracingContext::BuildShaderTables()
{
	uint32_t shaderIdentifierSize{ D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };
	
	const D3D12_GPU_DESCRIPTOR_HANDLE rtvHandle{ m_RaytracingHeap->GetHeap()->GetGPUDescriptorHandleForHeapStart() };
	uint64_t* heapPointer{ reinterpret_cast<uint64_t*>(rtvHandle.ptr) };
	
	void* rayGenIdentifier	{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_RayGenShaderName) };
	void* missIdentifier	{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_MissShaderName) };
	void* hitIdentifier		{ m_StateObjectProperties.Get()->GetShaderIdentifier(m_HitGroupName) };

	// RayGen Table
	{
		uint32_t argsSize{ shaderIdentifierSize + sizeof(heapPointer) };
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
		//test
		void* pVertex{ reinterpret_cast<void*>(m_VertexBuffer.Buffer.GetBuffer()->GetGPUVirtualAddress()) };

		m_HitTable.Create(m_DeviceCtx->GetDevice(), 1, shaderIdentifierSize);
		TableRecord record(hitIdentifier, shaderIdentifierSize, &pVertex, sizeof(pVertex));
		m_HitTable.AddRecord(record);
	}

}

void RaytracingContext::SerializeAndCreateRootSignature(D3D12_ROOT_SIGNATURE_DESC& Desc, ComPtr<ID3D12RootSignature>* ppRootSignature)
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> error;

	ThrowIfFailed(D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, blob.GetAddressOf(), error.GetAddressOf()));
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*(ppRootSignature)))));
}
