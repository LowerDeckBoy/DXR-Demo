#ifndef GLOBAL_VERTEX_HLSL
#define GLOBAL_VERTEX_HLSL

cbuffer cbPerObject : register(b0)
{
    row_major float4x4 WVP;
    row_major float4x4 World;
    float4 padding[8];
}

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    //float4 Color : COLOR;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : WORLD_POSITION;
    float3 Normal : NORMAL;
    //float4 Color : COLOR;
};


VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    
    output.Position      = mul(WVP, float4(vin.Position, 1.0f));
    output.WorldPosition = mul(World, float4(vin.Position, 1.0f));
    //output.Color         = vin.Color;
    output.Normal        = normalize(vin.Normal);
    
    return output;
}

#endif // GLOBAL_VERTEX_HLSL
