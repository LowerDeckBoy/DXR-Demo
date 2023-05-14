#pragma once
#include <cstdint>
#include <memory>
#include "../Utilities/Timer.hpp"

// Base for app behaviour
class DXSample
{
public:
	DXSample() {};
	virtual ~DXSample();

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnResize() = 0;
	virtual void OnDestroy() = 0;

	// Window states for window moving and resizing
	inline static bool bAppPaused { false };
	inline static bool bMinimized { false };
	inline static bool bMaximized { false };
	inline static bool bIsResizing{ false };

	struct DisplayResolution
	{
		DisplayResolution() : Width(1280), Height(800), AspectRatio(static_cast<float>(Width) / static_cast<float>(Height)) { }
		DisplayResolution(uint32_t Width, uint32_t Height)
		{
			Set(Width, Height);
		}

		void Set(uint32_t Width, uint32_t Height)
		{
			this->Width = Width;
			this->Height = Height;
			this->AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
		}

		uint32_t Width{ 1280 };
		uint32_t Height{ 800 };
		float AspectRatio{ static_cast<float>(Width) / static_cast<float>(Height) };
	} m_Resolution;

	DisplayResolution Resolution() { return m_Resolution; }


};

