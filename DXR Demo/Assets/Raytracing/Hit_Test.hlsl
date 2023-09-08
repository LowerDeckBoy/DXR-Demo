#ifndef HIT_TEST_HLSL
#define HIT_TEST_HLSL

#include "Common.hlsli"
#include "HitCommon.hlsli"

cbuffer cbCube : register(b0, space2)
{
    float4 Albedo;
}
//ObjectVertex
StructuredBuffer<ModelVertex> Vertex : register(t0, space1);
StructuredBuffer<uint> Indices : register(t1, space1);
Texture2D<float4> AlbedoTexture : register(t1, space2);

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] + attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) + attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

float2 HitUVAttribute(float2 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] + attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) + attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, BuiltInTriangleIntersectionAttributes attrib)
{
    uint primitiveIndex = PrimitiveIndex() * 3;
    float3 vertices[3] =
    {
        Vertex[Indices[primitiveIndex + 0]].Normal,
        Vertex[Indices[primitiveIndex + 1]].Normal,
        Vertex[Indices[primitiveIndex + 2]].Normal,
    };
    
    float2 texCoords[3] =
    {
        Vertex[Indices[primitiveIndex + 0]].TexCoord,
        Vertex[Indices[primitiveIndex + 1]].TexCoord,
        Vertex[Indices[primitiveIndex + 2]].TexCoord,
    };

    float3 triangleNormal = HitAttribute(vertices, attrib);
    float3 pixelToLight = normalize(gSceneData.LightPosition.xyz - WorldPosition());
    //float NdotL = max(0.0f, dot(pixelToLight, texCoords));
    
    float2 uv = HitUVAttribute(texCoords, attrib);
    
    uint width;
    uint height;
    AlbedoTexture.GetDimensions(width, height);

    float3 tex;
    if (width == 1024)
    {
        tex = AlbedoTexture.Load(int3((uv.xy * width), 0.0f)).rgb;
    }
    else
    {
        uv.x *= (width  / 2);
        uv.y *= (height / 2);
        tex = AlbedoTexture.Load(int3((uv.xy), 0.0f)).rgb;
    }
    
    payload.Color = float4(tex, 1.0f);
    //payload.Color = float4(0.5f, 0.5f, 1.0f, RayTCurrent());

}

[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, BuiltInTriangleIntersectionAttributes attrib)
{
    payload.Color = float4(1.f, 0.5f, 1.0f, RayTCurrent());
}

#endif // HIT_TEST_HLSL
