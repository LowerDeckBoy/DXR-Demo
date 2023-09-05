#ifndef SHADOWRAY_HLSL
#define SHADOWRAY_HLSL

#include "Common.hlsli"
#include "HitCommon.hlsli"

//https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial/extra/dxr_tutorial_extra_another_ray_type

[shader("closesthit")]
void ShadowClosestHit(inout ShadowHitInfo HitInfo, BuiltInTriangleIntersectionAttributes attrib)
{
    HitInfo.bHit = true;
}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo HitInfo : SV_RayPayload)
{
    HitInfo.bHit = false;
}

#endif // SHADOWRAY_HLSL
