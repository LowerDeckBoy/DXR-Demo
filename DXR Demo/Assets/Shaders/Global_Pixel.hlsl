#ifndef GLOBAL_PIXEL_HLSL
#define GLOBAL_PIXEL_HLSL

struct PS_INPUT
{
    float4 Position         : SV_POSITION;
    float4 WorldPosition    : WORLD_POSITION;
    //float4 Color            : COLOR;
    float3 Normal           : NORMAL;
};

float4 main(PS_INPUT pin) : SV_TARGET
{
	//return pin.Color;
    //return float4(0.5f, 0.5f, 0.5f, 1.0f);
    return float4(pin.Normal, 1.0f);
}

#endif // GLOBAL_PIXEL_HLSL
