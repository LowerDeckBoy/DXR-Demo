#ifndef RAYGEN_HLSL
#define RAYGEN_HLSL

#include "Common.hlsl"

RWTexture2D<float4> RaytraceScene : register(u0);
RaytracingAccelerationStructure SceneTopLevel : register(t0, space0);

[shader("raygeneration")]
void RayGen()
{
    HitInfo payload = { float4(.0f, .0f, .0f, .0f) };

    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = ((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f;
    
    float2 xy = launchIndex + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Y inversion
    screenPos.y = -screenPos.y;
    float aspectRatio = dims.x / dims.y;
    float4x4 viewProj = SceneData.ViewProjection;
    float4 world = mul(float4(screenPos, 0, 1), viewProj);
    world.xyz /= world.w;
    float3 origin = SceneData.CameraPosition.xyz;
    float3 direction = normalize(world.xyz - origin);
   
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    // TMin -> zNear, TMax -> zFar
    ray.TMin = 0.01f;
    ray.TMax = 100000.0f;
    //RAY_FLAG_CULL_BACK_FACING_TRIANGLES
    TraceRay(SceneTopLevel, RAY_FLAG_NONE, ~0, 0, 1, 0, ray, payload);
    
    RaytraceScene[launchIndex] = payload.Color;
}

/*
[shader("raygeneration")]
void RayGen()
{
    HitInfo payload;
    payload.ColorAndDistance = float4(0.0f, 0.0f, 0.0f, 0.0f);

    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = ((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f;
    
    RayDesc ray;
    ray.Origin = float3(d.x, -d.y, 1.0f);
    ray.Direction = float3(0.0f, 0.0f, -1.0f);
    // TMin -> zNear, TMax -> zFar
    ray.TMin = 0.0f;
    ray.TMax = 100000.0f;
    
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    gOutput[launchIndex] = float4(payload.ColorAndDistance.rgb, 1.f);
}
*/
#endif //RAYGEN_HLSL
