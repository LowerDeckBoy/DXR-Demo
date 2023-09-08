#include "../Core/DeviceContext.hpp"
#include "Texture.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/FileUtils.hpp"
#include <DirectXTex.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/WICTextureLoader.h>
//#include <directxtk12/DDSTextureLoader.h>
//#include "TextureUtils.hpp"


Texture::Texture()
{

}

Texture::Texture(DeviceContext* pDevice, const std::string_view& TexturePath)
{
	Create(pDevice, TexturePath);
}

Texture::~Texture()
{
	Release();
}

void Texture::Create(DeviceContext* pDevice, const std::string_view& TexturePath)
{
	CreateFromWIC(pDevice, TexturePath);
}

void Texture::CreateFromWIC(DeviceContext* pDevice, const std::string_view& TexturePath)
{
	//assert(m_Device = pDevice);

	std::wstring wpath = std::wstring(TexturePath.begin(), TexturePath.end());
	const wchar_t* path = wpath.c_str();

	DirectX::ScratchImage* scratchImage{ new DirectX::ScratchImage() };
	DirectX::LoadFromWICFile(path,
		DirectX::WIC_FLAGS_FORCE_RGB,
		nullptr,
		*scratchImage);

	// https://github.com/microsoft/DirectXTK12/wiki/ResourceUploadBatch
	DirectX::ResourceUploadBatch upload(pDevice->GetDevice());
	upload.Begin();

	std::unique_ptr<uint8_t[]> decodedData;
	D3D12_SUBRESOURCE_DATA subresource{};

	DirectX::LoadWICTextureFromFileEx(pDevice->GetDevice(), path, 0, D3D12_RESOURCE_FLAG_NONE, DirectX::DX12::WIC_LOADER_DEFAULT, m_Texture.ReleaseAndGetAddressOf(), decodedData, subresource);

	const auto desc{ m_Texture->GetDesc() };

	//desc.MipLevels
	const uint32_t mips = (desc.MipLevels < m_MipLevels) ? desc.MipLevels : m_MipLevels;
	const auto uploadDesc = CD3DX12_RESOURCE_DESC(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		desc.Width, desc.Height, 1, 1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		1, 0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE);

	const auto heapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_Texture.ReleaseAndGetAddressOf())));

	upload.Upload(m_Texture.Get(), 0, &subresource, 1);
	upload.Transition(m_Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	upload.GenerateMips(m_Texture.Get());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = mips;
	srvDesc.Texture2D.MostDetailedMip = 0;

	pDevice->GetMainHeap()->Allocate(m_Descriptor);
	pDevice->GetDevice()->CreateShaderResourceView(m_Texture.Get(), &srvDesc, m_Descriptor.GetCPU());

	auto finish{ upload.End(pDevice->GetCommandQueue()) };
	finish.wait();

	m_Width = static_cast<uint32_t>(desc.Width);
	m_Height = desc.Height;
}

void Texture::Release()
{
	SAFE_RELEASE(m_TextureUploadHeap);
	SAFE_RELEASE(m_Texture);
}
