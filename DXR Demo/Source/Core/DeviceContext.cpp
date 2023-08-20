#include "DeviceContext.hpp"
#include "Window.hpp"
#include <algorithm>
#include "../Utilities/Utilities.hpp"

#if defined (_DEBUG) || (DEBUG)
#include <dxgidebug.h>
#endif

ComPtr<IDXGIAdapter3> AdapterInfo::Adapter = nullptr;
std::string AdapterInfo::AdapterName = "";

DeviceContext::DeviceContext()
{
    Create();
}

DeviceContext::~DeviceContext()
{
    Release();
}

void DeviceContext::Create()
{
    CreateDevice();
    CreateCommandQueue();
    CreateCommandAllocators();
    CreateCommandList();
    CreateFence();

    CreateSwapChain();
    CreateDescriptorHeaps();
    CreateBackbuffers();

    //CreateDepthStencil();
}

void DeviceContext::CreateDevice()
{
    uint32_t dxgiFactoryFlags{ 0 };
#if defined (_DEBUG) || (DEBUG)
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

    ComPtr<ID3D12Debug1> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));

    debugController.Get()->EnableDebugLayer();

    SAFE_RELEASE(debugController);
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_Factory.ReleaseAndGetAddressOf())));

    ComPtr<IDXGIAdapter1> adapter;
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_Factory.Get()->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf())); i++)
    {
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        // No need to check for D3D_FEATURE_LEVEL_12_1
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), m_FeatureLevel, __uuidof(ID3D12Device), nullptr)))
        {
            // Suitable device found
            SAFE_RELEASE(adapter);
            break;
        }
    }

    if (!adapter.Get())
        throw std::runtime_error("Lost IDXGIAdapter1 before ID3D12Device creation!\n");

    ThrowIfFailed(adapter.As(&m_Adapter));

    ThrowIfFailed(adapter.As(&AdapterInfo::Adapter));

    // Getting GPU name
    {
        DXGI_ADAPTER_DESC1 desc1{};
        m_Adapter.Get()->GetDesc1(&desc1);
        std::wstring wname{ desc1.Description };
        std::string name(wname.length(), 0);
        std::transform(wname.begin(), wname.end(), name.begin(), [](wchar_t c) { return (char)c; });
        AdapterInfo::AdapterName = name;
    }

    ComPtr<ID3D12Device> device;
    ThrowIfFailed(D3D12CreateDevice(m_Adapter.Get(), m_FeatureLevel, IID_PPV_ARGS(device.ReleaseAndGetAddressOf())), "Failed to create ID3D12Device!\n");

    ThrowIfFailed(device.As(&m_Device), "Failed to cast ID3D12Device to ID3D12Device5!\n");
    m_Device.Get()->SetName(L"D3D12 Device");

#if defined (_DEBUG) || (DEBUG)
    ComPtr<IDXGIDebug1> dxgiDebug;
    ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));
    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_DX, DXGI_DEBUG_RLO_SUMMARY);
    dxgiDebug.Get()->DisableLeakTrackingForThread();

    SAFE_RELEASE(dxgiDebug);
#endif

    SAFE_RELEASE(adapter);

    // Check for Raytracing support
    // Mendatory
    bRaytracingSupport = CheckRaytracingSupport(m_Adapter.Get());
    assert(bRaytracingSupport);

}

void DeviceContext::CreateDescriptorHeaps()
{
    // Render Target
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = FRAME_COUNT;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;
    ThrowIfFailed(m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_RenderTargetHeap.ReleaseAndGetAddressOf())));

    m_RenderTargetHeapDescriptorSize = m_Device.Get()->GetDescriptorHandleIncrementSize(desc.Type);

    // SRV
    //D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NumDescriptors = 4096;

    m_MainHeap = std::make_unique<DescriptorHeap>();
    ThrowIfFailed(m_Device.Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_MainHeap->GetHeapAddressOf())));

    // D3D12MA
    D3D12MA::ALLOCATOR_DESC allocatorDesc{};
    allocatorDesc.pDevice = m_Device.Get();
    allocatorDesc.pAdapter = m_Adapter.Get();
    D3D12MA::CreateAllocator(&allocatorDesc, m_Allocator.ReleaseAndGetAddressOf());
}

void DeviceContext::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE Type)
{
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = Type;
    desc.NodeMask = 0;
    desc.Priority = 0;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(m_Device.Get()->CreateCommandQueue(&desc, IID_PPV_ARGS(m_CommandQueue.ReleaseAndGetAddressOf())), "Failed to create ID3D12CommandQueue!\n");
    m_CommandQueue.Get()->SetName(L"Command Queue");
}

void DeviceContext::CreateCommandAllocators(D3D12_COMMAND_LIST_TYPE Type)
{
    for (uint32_t i = 0; i < FRAME_COUNT; i++)
    {
        ThrowIfFailed(m_Device.Get()->CreateCommandAllocator(Type, IID_PPV_ARGS(m_CommandAllocators[i].ReleaseAndGetAddressOf())));
    }
}

void DeviceContext::CreateCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12PipelineState* pInitialState)
{
    ThrowIfFailed(m_Device.Get()->CreateCommandList(0, Type, m_CommandAllocators[0].Get(), pInitialState, IID_PPV_ARGS(m_CommandList.ReleaseAndGetAddressOf())), "Failed to create ID3D12CommandList!\n");
    m_CommandList.Get()->SetName(L"Command List");
}

void DeviceContext::CreateSwapChain()
{
    SetViewport();

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = FRAME_COUNT;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Width  = Window::Resolution().Width;
    desc.Height = Window::Resolution().Height;
    desc.SampleDesc = { 1, 0 };

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc{};
    fullScreenDesc.Windowed = TRUE;
    fullScreenDesc.RefreshRate = { 0, 0 };
    fullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    fullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

    ComPtr<IDXGISwapChain1> swapchain;
    ThrowIfFailed(m_Factory.Get()->CreateSwapChainForHwnd(m_CommandQueue.Get(), Window::GetHWND(), &desc, &fullScreenDesc, nullptr, swapchain.ReleaseAndGetAddressOf()), "Failed to create IDXGISwapChain1!\n");
    ThrowIfFailed(swapchain.As(&m_SwapChain));

    ThrowIfFailed(m_Factory.Get()->MakeWindowAssociation(Window::GetHWND(), DXGI_MWA_NO_ALT_ENTER));

    FRAME_INDEX = m_SwapChain.Get()->GetCurrentBackBufferIndex();
}

void DeviceContext::CreateBackbuffers()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RenderTargetHeap.Get()->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < FRAME_COUNT; i++)
    {
        ThrowIfFailed(m_SwapChain.Get()->GetBuffer(i, IID_PPV_ARGS(m_RenderTargets.at(i).ReleaseAndGetAddressOf())));
        m_Device.Get()->CreateRenderTargetView(m_RenderTargets.at(i).Get(), nullptr, rtvHandle);

        rtvHandle.Offset(1, m_RenderTargetHeapDescriptorSize);
    }
}

void DeviceContext::CreateFence()
{
    ThrowIfFailed(m_Device.Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetAddressOf())), "Failed to create ID3D12Fence!\n");
    m_FenceValues.at(FRAME_INDEX)++;

    m_FenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(m_FenceEvent != nullptr);
}

void DeviceContext::CreateDepthStencil()
{
    D3D12_DESCRIPTOR_HEAP_DESC dsHeap{};
    dsHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsHeap.NumDescriptors = 1;
    ThrowIfFailed(m_Device.Get()->CreateDescriptorHeap(&dsHeap, IID_PPV_ARGS(m_DepthHeap.GetAddressOf())));

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
    auto heapDesc{ CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
                                                static_cast<uint64_t>(m_Viewport.Width),
                                                static_cast<uint32_t>(m_Viewport.Height),
                                                1, 1) };
    heapDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    ThrowIfFailed(m_Device.Get()->CreateCommittedResource(&heapProperties,
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

    m_Device.Get()->CreateDepthStencilView(m_DepthStencil.Get(), &dsView, m_DepthHeap.Get()->GetCPUDescriptorHandleForHeapStart());
    m_DepthStencil.Get()->SetName(L"Depth Stencil");
}

void DeviceContext::SetViewport()
{
    m_Viewport.TopLeftX     = 0.0f;
    m_Viewport.TopLeftY     = 0.0f;
    m_Viewport.Width        = static_cast<float>(Window::Resolution().Width);
    m_Viewport.Height       = static_cast<float>(Window::Resolution().Height);
    m_Viewport.MinDepth     = D3D12_MIN_DEPTH;
    m_Viewport.MaxDepth     = D3D12_MAX_DEPTH;

    m_ViewportRect.left     = 0;
    m_ViewportRect.top      = 0;
    m_ViewportRect.right    = static_cast<LONG>(Window::Resolution().Width);
    m_ViewportRect.bottom   = static_cast<LONG>(Window::Resolution().Height);
}

bool DeviceContext::CheckRaytracingSupport(IDXGIAdapter1* pAdapter)
{
    ComPtr<ID3D12Device> device;
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 features{};

    if (SUCCEEDED(D3D12CreateDevice(pAdapter, m_FeatureLevel, IID_PPV_ARGS(device.ReleaseAndGetAddressOf()))))
    {
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features))))
        {
            if (features.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
                return false;
        }
    }

    return true;
}

void DeviceContext::ExecuteCommandLists()
{
    ThrowIfFailed(m_CommandList.Get()->Close());

    ID3D12CommandList* ppCommandLists[] = { m_CommandList.Get() };
    m_CommandQueue.Get()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    WaitForGPU();
}

void DeviceContext::ExecuteCommandLists(bool bResetAllocator)
{
    ThrowIfFailed(m_CommandList.Get()->Close(), "Failed to close ID3D12GraphicsCommandList!");
    std::array<ID3D12CommandList*, 1> ppCommandLists{ m_CommandList.Get() };
    GetCommandQueue()->ExecuteCommandLists(static_cast<uint32_t>(ppCommandLists.size()), ppCommandLists.data());

    if (bResetAllocator)
        ThrowIfFailed(GetCommandList()->Reset(GetCommandAllocator(), nullptr));

    WaitForGPU();
}

void DeviceContext::FlushGPU()
{
    for (uint32_t i = 0; i < FRAME_COUNT; i++)
    {
        const uint64_t currentValue{ FRAME_INDEX };

        ThrowIfFailed(m_CommandQueue.Get()->Signal(m_Fence.Get(), currentValue));
        m_FenceValues.at(i)++;

        if (m_Fence.Get()->GetCompletedValue() < currentValue)
        {
            ThrowIfFailed(m_Fence.Get()->SetEventOnCompletion(currentValue, m_FenceEvent));

            WaitForSingleObject(m_FenceEvent, INFINITE);
        }
    }

    FRAME_INDEX = 0;
}

void DeviceContext::MoveToNextFrame()
{
    const UINT64 currentFenceValue = m_FenceValues.at(FRAME_INDEX);
    ThrowIfFailed(GetCommandQueue()->Signal(GetFence(), currentFenceValue));

    // Update the frame index.
    FRAME_INDEX = m_SwapChain.Get()->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_Fence.Get()->GetCompletedValue() < m_FenceValues.at(FRAME_INDEX))
    {
        ThrowIfFailed(m_Fence.Get()->SetEventOnCompletion(m_FenceValues.at(FRAME_INDEX), m_FenceEvent));
        WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
    }

    m_FenceValues.at(FRAME_INDEX) = currentFenceValue + 1;
}

void DeviceContext::WaitForGPU()
{
    ThrowIfFailed(m_CommandQueue.Get()->Signal(m_Fence.Get(), m_FenceValues.at(FRAME_INDEX)));

    ThrowIfFailed(m_Fence.Get()->SetEventOnCompletion(m_FenceValues.at(FRAME_INDEX), m_FenceEvent));
    ::WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

    m_FenceValues.at(FRAME_INDEX)++;
}

void DeviceContext::SetRenderTarget()
{
    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RenderTargetHeap.Get()->GetCPUDescriptorHandleForHeapStart(), FRAME_INDEX, m_RenderTargetHeapDescriptorSize);
    const CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle(m_DepthHeap.Get()->GetCPUDescriptorHandleForHeapStart());
    m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &depthHandle);

}

void DeviceContext::ClearRenderTarget()
{
    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RenderTargetHeap.Get()->GetCPUDescriptorHandleForHeapStart(), FRAME_INDEX, m_RenderTargetHeapDescriptorSize);
    const CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle(m_DepthHeap.Get()->GetCPUDescriptorHandleForHeapStart());
    m_CommandList.Get()->ClearRenderTargetView(rtvHandle, m_ClearColor.data(), 0, nullptr);
    m_CommandList.Get()->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void DeviceContext::ReleaseRenderTargets()
{
    for (size_t i = 0; i < FRAME_COUNT; i++)
    {
        m_RenderTargets.at(i).Reset();
        m_FenceValues.at(i) = m_FenceValues.at(FRAME_INDEX);
    }
}

void DeviceContext::OnResize()
{
    WaitForGPU();

    if (!m_Device.Get() || !m_SwapChain.Get() || !GetCommandAllocator())
        throw std::exception();

    GetCommandAllocator()->Reset();
    GetCommandList()->Reset(GetCommandAllocator(), nullptr);

    ReleaseRenderTargets();

    if (m_DepthStencil.Get())
        m_DepthStencil.Reset();

    const HRESULT hResult{ m_SwapChain.Get()->ResizeBuffers(DeviceContext::FRAME_COUNT,
                                                static_cast<uint32_t>(Window::Resolution().Width),
                                                static_cast<uint32_t>(Window::Resolution().Height),
                                                DXGI_FORMAT_R8G8B8A8_UNORM, 0) };

    if (hResult == DXGI_ERROR_DEVICE_REMOVED || hResult == DXGI_ERROR_DEVICE_RESET || FAILED(hResult))
    {
        ::OutputDebugStringA("Device removed!\n");
        throw std::exception();
    }

    FRAME_INDEX = 0;

    SetViewport();
    CreateBackbuffers();
    CreateDepthStencil();

}

void DeviceContext::Release()
{
    SAFE_RELEASE(m_DepthStencil);
    SAFE_RELEASE(m_DepthHeap);
    SAFE_RELEASE(m_RenderTargetHeap);

    for (auto& target : m_RenderTargets)
        SAFE_RELEASE(target);
    for (auto& allocator : m_CommandAllocators)
        SAFE_RELEASE(allocator);

    SAFE_RELEASE(m_Allocator);

    SAFE_RELEASE(m_Fence);
    SAFE_RELEASE(m_CommandQueue);
    SAFE_RELEASE(m_CommandList);
    SAFE_RELEASE(m_SwapChain);
    SAFE_RELEASE(m_Device);
    SAFE_RELEASE(m_Adapter);
    SAFE_RELEASE(m_Factory);
}

uint32_t DeviceContext::QueryAdapterMemory()
{
    DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo{};
    AdapterInfo::Adapter.Get()->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo);

    return static_cast<uint32_t>(memoryInfo.CurrentUsage / 1024 / 1024);
}

ID3D12CommandAllocator* DeviceContext::GetCommandAllocator(uint32_t Index) const
{
    if (Index >= 0 && Index < FRAME_COUNT)
        return m_CommandAllocators.at(Index).Get();

    return nullptr;
}

ID3D12CommandAllocator* DeviceContext::GetCommandAllocator() const
{
    return m_CommandAllocators.at(FRAME_INDEX).Get();
}

ID3D12Resource* DeviceContext::GetRenderTarget() const
{
    return m_RenderTargets.at(FRAME_INDEX).Get();
}

ID3D12Resource* DeviceContext::GetRenderTarget(uint32_t Index) const
{
    if (Index >= 0 && Index < FRAME_COUNT)
        return m_RenderTargets.at(Index).Get();

    return nullptr;
}
