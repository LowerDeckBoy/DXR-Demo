#pragma once
#include <Windows.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cassert>

#define SAFE_RELEASE(x) { if (x) { x.Reset(); x = nullptr; } }

inline void ThrowIfFailed(HRESULT hResult)
{
	if (FAILED(hResult))
		throw std::runtime_error("");
}

inline void ThrowIfFailed(HRESULT hResult, const std::string_view& Message)
{
	if (FAILED(hResult))
		throw std::runtime_error(Message.data());
}
