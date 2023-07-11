#pragma once
#include <wrl.h>
#include <d3dcompiler.h>
#include <vector>
#include <dxcapi.h>
#include <sstream>
#include <fstream>
#include "../Utilities/Utilities.hpp"


class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	void Initialize();

	IDxcBlob* CreateDXIL(const std::string_view& Filepath, LPCWSTR Target, LPCWSTR EntryPoint = L"main");

	void Release();

private:
	Microsoft::WRL::ComPtr<IDxcCompiler>		m_Compiler;
	Microsoft::WRL::ComPtr<IDxcLibrary>			m_Library;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler>	m_IncludeHandler;

};

class Shader
{
public:
	// For Shader model below 6.0
	void Create(const std::string_view& Filepath, const std::string_view& Target, const std::string_view& Entrypoint = "main", ID3DInclude* pInclude = nullptr);

	// Temporal
	[[deprecated]]
	void Create6(const std::string_view& Filepath, LPCWSTR Target, LPCWSTR Entrypoint = L"main", ID3DInclude* pInclude = nullptr);

	// DXR USAGE ONLY
	static IDxcBlob* CreateDXIL(const std::string_view& Filepath);

	[[nodiscard]]
	inline ID3DBlob* GetData() const noexcept { return Blob.Get(); }
	// For Shader model of 6.x
	[[nodiscard]]
	inline IDxcBlob* GetData6() const noexcept { return Blob6.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3DBlob> Blob;
	Microsoft::WRL::ComPtr<IDxcBlob> Blob6;
	bool bIsInitialized{ false };
};
