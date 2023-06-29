#pragma once
#include <Windows.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cassert>

#define SAFE_RELEASE(x) { if (x) { x.Reset(); x = nullptr; } }

/*
std::string ErrorCodeToMessage()
{
	auto lastError{ ::GetLastError() };

	if (lastError == 0)
		return std::string();

	LPSTR buffer{ nullptr };

	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(buffer, size);

	//Free the Win32's string's buffer.
	LocalFree(buffer);

	return message;	

}
*/
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
