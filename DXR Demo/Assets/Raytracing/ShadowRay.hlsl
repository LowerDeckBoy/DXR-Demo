#ifndef SHADOWRAY_HLSL
#define SHADOWRAY_HLSL

#include "HitCommon.hlsli"

// Note:
// Miss shader on it's own will suffice to cast rays
// just as it does along ClosestHit shader
//
//[shader("closesthit")]
//void ShadowClosestHit(inout ShadowHitInfo HitInfo, BuiltInTriangleIntersectionAttributes attrib)
//{
//    //HitInfo.bHit = true;
//}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo HitInfo : SV_RayPayload)
{
    HitInfo.bHit = false;
}

#endif // SHADOWRAY_HLSL
