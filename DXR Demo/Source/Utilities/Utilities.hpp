#pragma once
#include <stdexcept>
#include <cassert>
#include <string>


// For ComPtr
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(_ComPtr) { if (_ComPtr) { _ComPtr.Reset(); _ComPtr = nullptr; } }
#endif // SAFE_RELEASE

// For non ComPtr
#ifndef SAFE_DELETE
#define SAFE_DELETE(_Ptr) { if (_Ptr) { _Ptr->Release(); _Ptr = nullptr; } }
#endif // SAFE_DELETE

// Helper macro to determinate where exactly HRESULT error occurs
#ifndef TraceError
#define TraceError(DebugMessage)											\
	{																		\
		std::string file{ "File: " + std::string(__FILE__) };				\
		std::string func{ "\nFunction: " + std::string(__func__) };			\
		std::string line{ "\nLine: " + std::to_string(__LINE__) + '\n' };	\
		std::string message{ file + func + line + DebugMessage };			\
		::OutputDebugStringA(message.c_str());								\
		::MessageBoxA(nullptr, message.data(), "Error", MB_OK);				\
	}																	
#endif // TraceError

#ifndef ThrowIfFailed
#define ThrowIfFailed(hResult, ...) \
	if (FAILED(hResult))\
	{\
		std::string msg{ std::string(__VA_ARGS__) };			\
		TraceError(msg);										\
		throw std::runtime_error("HRESULT exception thrown.");	\
	}
#endif // ThrowIfFailed
