#ifndef HIT_HLSL
#define HIT_HLSL

#include "Common.hlsl"

struct TriangleVertex {
    float3 Vertex;
    float4 Color;
};

StructuredBuffer<TriangleVertex> TriVertex : register(t0, space1);

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.Barycentric.x - attrib.Barycentric.y, attrib.Barycentric.x, attrib.Barycentric.y);

    uint vertId = 3 * PrimitiveIndex();
    float3 hitColor = TriVertex[vertId + 0].Color * barycentrics.x + TriVertex[vertId + 1].Color * barycentrics.y + TriVertex[vertId + 2].Color * barycentrics.z;

    payload.Color = float4(hitColor, RayTCurrent());
   
}

#endif //HIT_HLSL
