#ifndef HITRAY_HLSL
#define HITRAY_HLSL

#include "Common.hlsli"
#include "HitCommon.hlsli"

cbuffer cbCube : register(b0, space2)
{
    float4 Albedo;
}

StructuredBuffer<ObjectVertex> Vertex : register(t0, space1);
StructuredBuffer<uint> Indices : register(t1, space1);

// unused
// lookup function
uint3 Load3x32BitIndices(uint offsetBytes, ByteAddressBuffer Indices)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;
    const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);

    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 32) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 32) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 32) & 0xffff;
    }

    return indices;
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
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
    
    float3 triangleNormal = HitAttribute(vertices, attrib);
    
    float3 pixelToLight = normalize(gSceneData.LightPosition.xyz - WorldPosition());
    float NdotL = max(0.0f, dot(pixelToLight, triangleNormal));
    float4 color = saturate(Albedo * gSceneData.LightDiffuse * NdotL);
    
    payload.Color = float4(gSceneData.LightAmbient.rgb + color.rgb, RayTCurrent());
}

[shader("closesthit")] 
void PlaneClosestHit(inout HitInfo payload, BuiltInTriangleIntersectionAttributes attrib) 
{ 
    float3 lightPos = gSceneData.LightPosition.xyz;

    float3 origin = WorldPosition();
    float3 direction = normalize(lightPos - origin);
    
    // Tracing the shadow ray
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.01f;
    ray.TMax = 100000.0f;
    bool hit = true;
    
    ShadowHitInfo shadowPayload;
    // Not using ShadowClosestHit shader
    // hit is assumed as true
    shadowPayload.bHit = true;
    
    TraceRay(
        gSceneTopLevel,
        RAY_FLAG_NONE,
        0xFF, 
        1,
        0, 
        1, 
        ray, 
        shadowPayload);
    
    float shadowFactor = shadowPayload.bHit ? 0.25f : 1.0f;
    float3 barycentrics = float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y); 

    float4 hitColor = float4(float3(0.5f, 0.5f, 0.5f) * shadowFactor, RayTCurrent());
    //payload.Color = hitColor;
    
    uint primitiveIndex = PrimitiveIndex() * 3;
    float3 vertices[3] =
    {
        Vertex[Indices[primitiveIndex + 0]].Normal,
        Vertex[Indices[primitiveIndex + 1]].Normal,
        Vertex[Indices[primitiveIndex + 2]].Normal,
    };
    
    float3 triangleNormal = HitAttribute(vertices, attrib);
    float3 pixelToLight = normalize(gSceneData.LightPosition.xyz - WorldPosition());
    float NdotL = max(0.0f, dot(pixelToLight, triangleNormal));
    float4 color = saturate(float4(0.5f, 0.5f, 0.5f, 1.0f) * NdotL);
    //float4 color = saturate(float4(0.5f, 0.5f, 0.5f, 1.0f) * gSceneData.LightDiffuse * NdotL);
    
    payload.Color = float4(hitColor.rgb + color.rgb, RayTCurrent());
    
}

#endif //HITRAY_HLSL
