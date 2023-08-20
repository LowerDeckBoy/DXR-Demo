#ifndef HITRAY_HLSL
#define HITRAY_HLSL

#include "Common.hlsli"
#include "HitCommon.hlsli"

cbuffer cbCube : register(b0, space2)
{
    float4 Albedo;
}

StructuredBuffer<ObjectVertex> Vertex : register(t0, space1);
//StructuredBuffer<uint> Indices : register(t1, space1);

//RaytracingAccelerationStructure TopLevel : register(t2, space1);

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
    //float3 barycentrics = float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);
    //uint id = InstanceIndex();
    
    //const uint3 indices = Load3x32BitIndices(primitiveIndex, ) 
    uint primitiveIndex = PrimitiveIndex() * 3;
    float3 vertices[3] =
    {
        Vertex[Indices[primitiveIndex + 0]].Normal,
        Vertex[Indices[primitiveIndex + 1]].Normal,
        Vertex[Indices[primitiveIndex + 2]].Normal,
    };
    
    float3 triangleNormal = HitAttribute(vertices, attrib);
    
    //float3 pixelToLight = normalize(SceneData.CameraPosition.xyz - WorldPosition());
    float3 pixelToLight = normalize(SceneData.LightPosition.xyz - WorldPosition());
    float NdotL = max(0.0f, dot(pixelToLight, triangleNormal));
    float4 color = saturate(Albedo * SceneData.LightDiffuse * NdotL);
    
    payload.Color = SceneData.LightAmbient + color;
}

/*
[shader("closesthit")] 
void PlaneClosestHit(inout HitInfo payload, BuiltInTriangleIntersectionAttributes attrib) 
{ 
    //float3 lightPos = float3(2, 2, -2); 
    float3 lightPos = SceneData.LightPosition;
    // Find the world - space hit position 
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float3 lightDir = normalize(lightPos - worldOrigin);
    // Fire a shadow ray. The direction is hard-coded here, but can be fetched
    // from a constant-buffer
    RayDesc ray;
    ray.Origin = worldOrigin;
    ray.Direction = lightDir;
    ray.TMin = 0.01;
    ray.TMax = 100000;
    bool hit = true; 
    // Initialize the ray payload 
    ShadowHitInfo shadowPayload;
    shadowPayload.bHit = false;
    // Trace the ray 
    TraceRay(
        SceneTopLevel, 
        RAY_FLAG_NONE, 
        0xFF, 
        1,
        0, 
        1, 
        ray, 
        shadowPayload); 
    
    float factor = shadowPayload.bHit ? 0.3f : 1.0f;
    float3 barycentrics = float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y); 
    float4 hitColor = float4(float3(0.7, 0.7, 0.3) * factor, RayTCurrent()); 
    payload.Color = float4(hitColor) + float4(1.0f, 1.0f, 1.0f, 1.0f);
}
*/

/*
 // #DXR Extra - Another ray type 
[shader("closesthit")] 
void PlaneClosestHit(inout HitInfo payload, BuiltInTriangleIntersectionAttributes attrib) 
{ 
    //float3 lightPos = float3(2, 2, -2); 
    //float3 lightPos = float3(2, 2, -10); 
    float3 lightPos = SceneData.LightPosition;
    // Find the world - space hit position 
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection(); 
    float3 lightDir = normalize(lightPos - worldOrigin); 
    // Fire a shadow ray. The direction is hard-coded here, but can be fetched 
    // from a constant-buffer 
    RayDesc ray; 
    ray.Origin = worldOrigin;
    ray.Direction = lightDir; 
    ray.TMin = 0.01; 
    ray.TMax = 100000; 
    bool hit = true; 
    // Initialize the ray payload 
    ShadowHitInfo shadowPayload; 
    shadowPayload.bHit = false; 
    // Trace the ray 
    TraceRay( 
        SceneTopLevel, 
        RAY_FLAG_NONE,  
        0xFF, 
    // Depending on the type of ray, a given object can have several hit 
    // groups attached (ie. what to do when hitting to compute regular 
    // shading, and what to do when hitting to compute shadows). Those hit 
    // groups are specified sequentially in the SBT, so the value below 
    // indicates which offset (on 4 bits) to apply to the hit groups for this 
    // ray. In this sample we only have one hit group per object, hence an 
    // offset of 0. 
        1, 
    // The offsets in the SBT can be computed from the object ID, its instance 
    // ID, but also simply by the order the objects have been pushed in the 
    // acceleration structure. This allows the application to group shaders in 
    // the SBT in the same order as they are added in the AS, in which case 
    // the value below represents the stride (4 bits representing the number // of hit groups) between two consecutive objects. 
        0, 
    // Index of the miss shader to use in case several consecutive miss 
    // shaders are present in the SBT. This allows to change the behavior of 
    // the program when no geometry have been hit, for example one to return a 
    // sky color for regular rendering, and another returning a full 
    // visibility value for shadow rays. This sample has only one miss shader, 
    // hence an index 0 
        1, // Ray information to trace 
        ray, 
    // Payload associated to the ray, which will be used to communicate 
    // between the hit/miss shaders and the raygen 
        shadowPayload); 
    
    float factor = shadowPayload.bHit ? 0.3 : 1.0; 
    float3 barycentrics = float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y); 
    float4 hitColor = float4(float3(0.7, 0.7, 0.3) * factor, RayTCurrent());
    //payload.Color = float4(hitColor);
    payload.Color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}
*/
#endif //HITRAY_HLSL
