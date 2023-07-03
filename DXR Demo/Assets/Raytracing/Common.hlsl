#ifndef COMMON_HLSL
#define COMMON_HLSL

struct HitInfo
{
	float4 Color;
};

//test
struct SceneBuffer
{
	matrix ViewProjection;
	float4 CameraPosition;
	float4 LightPosition;
	float4 LightAmbient;
	float4 LightDiffuse;
};

struct CubeVertex
{
	float4 CubeColor;
};

struct ObjectVertex
{
    float3 Vertex;
    //float4 Color;
	float3 Normal;
};

cbuffer cbCamera : register(b0, space0)
{
    float4x4 View;
    float4x4 Projection;
    float4x4 InversedView;
    float4x4 InversedProjection;
}

ConstantBuffer<SceneBuffer> SceneData : register(b1, space0);

#endif //COMMON_HLSL
