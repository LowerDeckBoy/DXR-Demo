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

struct ModelVertex
{
    float3 Position;
    float2 TexCoord;
    float3 Normal;
    float3 Tangent;
    float3 Bitangent;
};

struct AlbedoResolution
{
    float4 TexResolution;
};

// Buffers
RWTexture2D<float4> gRaytraceScene : register(u0, space0);
RaytracingAccelerationStructure gSceneTopLevel : register(t0, space0);

ConstantBuffer<SceneBuffer> SceneData : register(b1, space0);

//StructuredBuffer<ModelVertex> Vertices : register(t0, space1);
StructuredBuffer<int> Indices : register(t1, space1);
//ByteAddressBuffer Indices  : register(t1, space1);
//ByteAddressBuffer Vertices : register(t0, space1);

//Texture2D<float4> BaseColorTexture : register(t2, space1);

//ConstantBuffer<AlbedoResolution> TextureResolution : register(b2, space0);

#endif // COMMON_HLSLI
