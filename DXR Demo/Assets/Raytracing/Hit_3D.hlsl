#ifndef HIT_3D_HLSL
#define HIT_3D_HLSL

#include "Common.hlsl"

cbuffer cbCube : register(b0, space2)
{
    float4 Albedo;
}

StructuredBuffer<ObjectVertex> Vertex : register(t0, space1);
StructuredBuffer<int> Indices : register(t1, space1);

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] + attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) + attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

float3 WorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, BuiltInTriangleIntersectionAttributes attrib)
{ 
    //float3 barycentrics = float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);
    
    uint primitiveIndex = PrimitiveIndex() * 3;
    float3 vertices[3] =
    {
        Vertex[Indices[primitiveIndex + 0]].Normal,
        Vertex[Indices[primitiveIndex + 1]].Normal,
        Vertex[Indices[primitiveIndex + 2]].Normal,
    };
    
    float3 triangleNormal = HitAttribute(vertices, attrib);
    
    float3 pixelToLight = normalize(SceneData.LightPosition.xyz - WorldPosition());
    //float3 pixelToLight = normalize(SceneData.CameraPosition.xyz - WorldPosition());
    float NdotL = max(0.0f, dot(pixelToLight, triangleNormal));
    float4 color = Albedo * SceneData.LightDiffuse * NdotL;
    payload.Color = SceneData.LightAmbient + color;
}

#endif //HIT_3D_HLSL
