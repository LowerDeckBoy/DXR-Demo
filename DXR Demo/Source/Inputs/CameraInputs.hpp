#pragma once
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <array>

class Camera;

class CameraInputs
{
private:
	inline static IDirectInputDevice8* DxKeyboard{};
	inline static IDirectInputDevice8* DxMouse{};
	inline static LPDIRECTINPUT8 DxInput{};
	inline static DIMOUSESTATE DxLastMouseState{};

public:
	static void Init();
	static void ReadInputs(Camera* pCamera, float DeltaTime);
	static void Release();

};

