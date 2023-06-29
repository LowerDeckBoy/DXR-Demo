#pragma once
#include <wrl.h>
#include <d3dcompiler.h>
#include <vector>
#include <dxcapi.h>
#include <sstream>
#include <fstream>
#include "../Utilities/Utilities.hpp"

// TODO:
// DXIL Libraries require to create Compiler and Library
// so Manager role is to create them once
// instead of per shader basis
class ShaderManager
{
public:


};

class Shader
{
public:
	// For Shader model below 6.0
	void Create(const std::string_view& Filepath, const std::string_view& Target, const std::string_view& Entrypoint = "main", ID3DInclude* pInclude = nullptr)
	{
		if (bIsInitialized)
			return;

		uint32_t compileFlags{ 0 };
#if defined (_DEBUG) || DEBUG
		compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ID3DBlob* Error{ nullptr };
		std::wstring path{ std::wstring(Filepath.begin(), Filepath.end()) };
		HRESULT hResult{ D3DCompileFromFile(path.c_str(), nullptr, pInclude, Entrypoint.data(), Target.data(), compileFlags, 0, Blob.GetAddressOf(), &Error) };

		if (Error != nullptr)
		{
			::OutputDebugStringA(static_cast<char*>(Error->GetBufferPointer()));
			return;
		}

		if (FAILED(hResult) || Blob == nullptr)
		{
			::OutputDebugStringA("Failed to compile shader!\n");
			return;
		}

		bIsInitialized = true;
	}

	void Create6(const std::string_view& Filepath, LPCWSTR Target, LPCWSTR Entrypoint = L"main", ID3DInclude* pInclude = nullptr)
	{
		IDxcLibrary* pLibrary{ nullptr };
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary)));

		IDxcCompiler* pCompiler{ nullptr };
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

		uint32_t codePage = CP_UTF8;
		IDxcBlobEncoding* pBlobEncoding{ nullptr };
		std::wstring wstr{ std::wstring(Filepath.begin(), Filepath.end()) };
		LPCWSTR path{ wstr.c_str() };
		ThrowIfFailed(pLibrary->CreateBlobFromFile(path, &codePage, &pBlobEncoding));

		IDxcOperationResult* result{ nullptr };
		HRESULT hResult{ pCompiler->Compile(pBlobEncoding, path, Entrypoint, Target, nullptr, 0, nullptr, 0, nullptr, &result) };

		if (SUCCEEDED(hResult))
			result->GetStatus(&hResult);

		if (FAILED(hResult))
		{
			if (result)
			{
				IDxcBlobEncoding* errorsBlob;
				hResult = result->GetErrorBuffer(&errorsBlob);
				if (SUCCEEDED(hResult) && errorsBlob)
				{
					wprintf(L"Compilation failed with errors:\n%hs\n",
						(const char*)errorsBlob->GetBufferPointer());
				}
			}
			// Handle compilation error...
		}

		IDxcBlob* data{};
		result->GetResult(Blob6.GetAddressOf());
	}

	// DXR USAGE ONLY
	static IDxcBlob* CreateDXIL(const std::string_view& Filepath)
	{
		static IDxcCompiler* compiler{ nullptr };
		static IDxcLibrary* library{ nullptr };
		static IDxcIncludeHandler* includeHeader{ nullptr };

		if (!compiler)
		{
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
			ThrowIfFailed(library->CreateIncludeHandler(&includeHeader));
		}

		std::ifstream shaderFile(Filepath.data());
		if (!shaderFile.good())
		{
			throw std::logic_error("Failed to read shader file!");
		}

		std::stringstream strStream;
		strStream << shaderFile.rdbuf();
		std::string shaderStr{ strStream.str() };

		static IDxcBlobEncoding* textBlob{};
		ThrowIfFailed(library->CreateBlobWithEncodingFromPinned(LPBYTE(shaderStr.c_str()), static_cast<uint32_t>(shaderStr.size()), 0, &textBlob));

		static IDxcOperationResult* result{};

		std::wstring wstr{ std::wstring(Filepath.begin(), Filepath.end()) };
		LPCWSTR filepath{ wstr.c_str() };
		ThrowIfFailed(compiler->Compile(textBlob, filepath, L"", L"lib_6_3", nullptr, 0, nullptr, 0, includeHeader, &result));

		HRESULT resultCode{};
		ThrowIfFailed(result->GetStatus(&resultCode));
		if (FAILED(resultCode))
		{
			static IDxcBlobEncoding* error{};
			static HRESULT hResult{ result->GetErrorBuffer(&error) };
			if (FAILED(hResult))
			{
				throw std::logic_error("Failed to get shader error code!");
			}

			std::vector<char> infoLog(error->GetBufferSize() + 1);
			std::memcpy(infoLog.data(), error->GetBufferPointer(), error->GetBufferSize());
			infoLog[error->GetBufferSize()] = 0;

			std::string errorMsg{ "Shader Compiler error:\n" };
			errorMsg.append(infoLog.data());

			::OutputDebugStringA(errorMsg.c_str());
			throw std::exception();
		}

		static IDxcBlob* blob{ nullptr };
		ThrowIfFailed(result->GetResult(&blob));
		return blob;
	}

	[[nodiscard]]
	inline ID3DBlob* GetData() const { return Blob.Get(); }
	// For Shader model of 6.0+
	inline IDxcBlob* GetData6() const { return Blob6.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3DBlob> Blob;
	Microsoft::WRL::ComPtr<IDxcBlob> Blob6;
	bool bIsInitialized{ false };
};

