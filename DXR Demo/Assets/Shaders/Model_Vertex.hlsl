#ifndef MODEL_VERTEX_HLSL
#define MODEL_VERTEX_HLSL

cbuffer cbPerObject : register(b0)
{
    row_major float4x4 WVP;
    row_major float4x4 World;
    float4 padding[8];
}

struct VS_INPUT
{
    float3 Position     : POSITION;
    float2 TexCoord     : TEXCOORD;
    float3 Normal       : NORMAL;
    float3 Tangent      : TANGENT;
    float3 Bitangent    : BITANGENT;
};

struct VS_OUTPUT
{
    float4 Position         : SV_POSITION;
    float4 WorldPosition    : WORLD_POSITION;
    float2 TexCoord         : TEXCOORD;
    float3 Normal           : NORMAL;
    float3 Tangent          : TANGENT;
    float3 Bitangent        : BITANGENT;
};


VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    
    output.Position = mul(WVP, float4(vin.Position, 1.0f));
    output.WorldPosition = mul(World, float4(vin.Position, 1.0f));
    output.TexCoord = vin.TexCoord;
    output.Normal = normalize(vin.Normal);
    output.Tangent = normalize(vin.Tangent);
    output.Bitangent = normalize(vin.Bitangent);
    
    return output;
}

#endif // MODEL_VERTEX_HLSL
