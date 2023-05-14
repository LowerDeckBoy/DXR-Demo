#include "DeviceContext.hpp"

#include "Window.hpp"
#include "../Utilities/Utilities.hpp"

#if defined (_DEBUG) || (DEBUG)
#include <dxgidebug.h>
#endif

DeviceContext::DeviceContext()
{
    Create();
}

DeviceContext::~DeviceContext()
{
    Destroy();
}

void DeviceContext::Create()
{
    CreateDevice();
    CreateCommandQueue();
    CreateCommandAllocators();
    CreateCommandList();

    CreateSwapChain();
    CreateBackbuffers();

    CreateFence();

    CreateDescriptorHeaps();
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

    ComPtr<ID3D12Device> device;
    ThrowIfFailed(D3D12CreateDevice(m_Adapter.Get(), m_FeatureLevel, IID_PPV_ARGS(device.ReleaseAndGetAddressOf())), "Failed to create ID3D12Device!\n");

    ThrowIfFailed(device.As(&m_Device), "Failed to cast ID3D12Device to ID3D12Device5!\n");
    m_Device.Get()->SetName(L"D3DDevice");

#if defined (_DEBUG) || (DEBUG)
    ComPtr<IDXGIDebug> dxgiDebug;
    ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));

#endif

    // Check for Raytracing here
    bRaytracingSupport = CheckRaytracingSupport(m_Adapter.Get());
}

void DeviceContext::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NumDescriptors = 4096;

    m_MainHeap = std::make_unique<DescriptorHeap>();
    ThrowIfFailed(m_Device.Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_MainHeap->GetHeapAddressOf())));

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
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = FRAME_COUNT;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    ThrowIfFailed(m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_RenderTargetHeap.ReleaseAndGetAddressOf())));

    m_RenderTargetHeapDescriptorSize = m_Device.Get()->GetDescriptorHandleIncrementSize(desc.Type);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RenderTargetHeap.Get()->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < FRAME_COUNT; i++)
    {
        ThrowIfFailed(m_SwapChain.Get()->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i])));
        m_Device.Get()->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, rtvHandle);

        rtvHandle.Offset(1, m_RenderTargetHeapDescriptorSize);
    }
}

void DeviceContext::CreateFence()
{
    ThrowIfFailed(m_Device.Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetAddressOf())), "Failed to create ID3D12Fence!\n");
    m_FenceValues[FRAME_INDEX]++;

    m_FenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(m_FenceEvent != nullptr);
}

void DeviceContext::SetViewport()
{
    m_ViewportRect.left     = 0;
    m_ViewportRect.top      = 0;
    m_ViewportRect.right    = static_cast<LONG>(Window::Resolution().Width);
    m_ViewportRect.bottom   = static_cast<LONG>(Window::Resolution().Height);

    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width    = static_cast<float>(Window::Resolution().Width);
    m_Viewport.Height   = static_cast<float>(Window::Resolution().Height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
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
                return true;
        }
    }

    return false;
}

void DeviceContext::Destroy()
{
}

ID3D12CommandAllocator* DeviceContext::GetCommandAllocator(uint32_t Index) const
{
    if (Index >= 0 && Index < FRAME_COUNT)
        return m_CommandAllocators[Index].Get();

    return nullptr;
}

ID3D12Resource* DeviceContext::GetRenderTarget(uint32_t Index) const
{
    if (Index >= 0 && Index < FRAME_COUNT)
        return m_RenderTargets[Index].Get();

    return nullptr;
}
