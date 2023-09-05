#ifndef COMMON_HLSLI
#define COMMON_HLSLI

struct SceneBuffer
{
	matrix ViewProjection;
	float4 CameraPosition;
	float4 LightPosition;
	float4 LightAmbient;
	float4 LightDiffuse;
};

struct HitInfo
{
	float4 Color;
};

struct CubeVertex
{
	float4 CubeColor;
};

struct ObjectVertex
{
	float3 Vertex;
	float3 Normal;
};

/*
cbuffer cbCamera : register(b0, space0)
{
	float4x4 View;
	float4x4 Projection;
	float4x4 InversedView;
	float4x4 InversedProjection;
}
*/

struct ModelVertex
{
	float3 Position;
	float2 TexCoord;
	float3 Normal;
	float3 Tangent;
	float3 Bitangent;
};

// Buffers
RWTexture2D<float4>             gRaytraceScene  : register(u0, space0);
RaytracingAccelerationStructure gSceneTopLevel  : register(t0, space0);
ConstantBuffer<SceneBuffer>     gSceneData      : register(b1, space0);

#endif // COMMON_HLSLI
