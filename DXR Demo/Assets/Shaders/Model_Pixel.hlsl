#ifndef MODEL_PIXEL_HLSL
#define MODEL_PIXEL_HLSL

struct PS_INPUT
{
    float4 Position         : SV_POSITION;
    float4 WorldPosition    : WORLD_POSITION;
    float2 TexCoord         : TEXCOORD;
    float3 Normal           : NORMAL;
    //float3 Tangent          : TANGENT;
    //float3 Bitangent        : BITANGENT;
};

cbuffer cbMaterial : register(b0, space1)
{
    float3 CameraPosition;
    float padding3;

    float4 BaseColorFactor;
    float4 EmissiveFactor;

    float MetallicFactor;
    float RoughnessFactor;
    float AlphaCutoff;
    bool bDoubleSided;
};

struct MaterialIndices
{
    int BaseColorIndex;
    int NormalIndex;
    int MetallicRoughnessIndex;
    int EmissiveIndex;
};

ConstantBuffer<MaterialIndices> Indices : register(b0, space2);
Texture2D<float4> TexturesTable[] : register(t0, space1);

SamplerState texSampler : register(s0);

float4 main(PS_INPUT pin) : SV_TARGET
{
    float4 baseColor = TexturesTable[Indices.BaseColorIndex].Sample(texSampler, pin.TexCoord);
	//return pin.Color;
    //return float4(0.5f, 0.5f, 0.5f, 1.0f);
    //return float4(pin.Position.xyz, 1.0f);
    return float4(baseColor.rgb, 1.0f);
}

#endif // MODEL_PIXEL_HLSL
