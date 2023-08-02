#include "../Core/DeviceContext.hpp"
#include "../Rendering/Camera.hpp"
#include "Editor.hpp"
#include "../Core/Window.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/Timer.hpp"
#include "../Utilities/MemoryUsage.hpp"


Editor::Editor()
{
}

Editor::~Editor()
{
	if (m_Font) m_Font = nullptr;

	SAFE_RELEASE(m_Heap);
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Editor::Initialize(DeviceContext* pDeviceCtx, Camera* pCamera, Timer* pTimer)
{
	assert(m_DeviceCtx = pDeviceCtx);
	assert(m_Timer = pTimer);

	CreateHeap();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& IO{ ImGui::GetIO() };
	IO.FontGlobalScale = 1.0f;
	IO.ConfigFlags |= ImGuiWindowFlags_AlwaysAutoResize;

	ImGuiStyle& Style{ ImGui::GetStyle() };
	ImGui::StyleColorsDark();
	Style.WindowRounding = 5.0f;
	Style.WindowBorderSize = 0.0f;

	ImGui_ImplWin32_Init(Window::GetHWND());
	ImGui_ImplDX12_Init(m_DeviceCtx->GetDevice(),
		DeviceContext::FRAME_COUNT,
		m_DeviceCtx->GetRenderTargetFormat(),
		m_Heap.Get(),
		m_Heap.Get()->GetCPUDescriptorHandleForHeapStart(),
		m_Heap.Get()->GetGPUDescriptorHandleForHeapStart());

	constexpr float fontSize{ 16.0f };
	//m_Font = IO.Fonts->AddFontFromFileTTF("Assets/Fonts/Roboto-Medium.ttf", fontSize);
	//m_Font = IO.Fonts->AddFontFromFileTTF("Assets/Fonts/JetBrainsMonoNerdFontMono-Medium.ttf", fontSize);
	//m_Font = IO.Fonts->AddFontFromFileTTF("Assets/Fonts/CaskaydiaCoveNerdFontMono-Bold.ttf", fontSize);
	m_Font = IO.Fonts->AddFontFromFileTTF("Assets/Fonts/CaskaydiaCoveNerdFontMono-SemiBold.ttf", fontSize);

}

void Editor::BeginFrame()
{
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
	
	ImGui::PushFont(m_Font);
}

void Editor::EndFrame()
{
	// Main Menu Bar
	{
		ImGui::BeginMainMenuBar();

		MemoryUsage::ReadRAM();
		const uint32_t VRAM{ DeviceContext::QueryAdapterMemory() };
		ImGui::Text("GPU: %s | ", AdapterInfo::AdapterName.data());
		ImGui::Text("FPS: %d ms: %.2f | Memory: %.2f MB | VRAM: %u MB", m_Timer->m_FPS, m_Timer->m_Miliseconds, MemoryUsage::MemoryInUse, VRAM);
		ImGui::EndMainMenuBar();
	}

	ImGui::PopFont();
	ImGui::EndFrame();
	ImGui::Render();

	m_DeviceCtx->GetCommandList()->SetDescriptorHeaps(1, m_Heap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_DeviceCtx->GetCommandList());
}

void Editor::OnResize()
{
	//IO.DisplaySize = ImVec2(Window::Resolution().Width, Window::Resolution().Height);
}

void Editor::CreateHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NumDescriptors = 1;
	ThrowIfFailed(m_DeviceCtx->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf())));
	m_Heap.Get()->SetName(L"GUI Heap");
}
