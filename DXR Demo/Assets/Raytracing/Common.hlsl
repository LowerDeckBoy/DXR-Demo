#ifndef COMMON_HLSL
#define COMMON_HLSL

struct HitInfo
{
	float4 ColorAndDistance;
};

struct Attributes
{
	float2 Barycentric;
};

#endif //COMMON_HLSL
