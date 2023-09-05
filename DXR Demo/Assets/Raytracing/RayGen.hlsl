#ifndef RAYGEN_HLSL
#define RAYGEN_HLSL

#include "Common.hlsli"

[shader("raygeneration")]
void RayGen()
{
    HitInfo payload = { float4(.0f, .0f, .0f, .0f) };

    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = ((launchIndex.xy + 0.5f) / dims.xy) * 2.0f - 1.0f;
    
    float2 xy = launchIndex + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Y inversion
    screenPos.y = -screenPos.y;
    float aspectRatio = dims.x / dims.y;
    float4x4 viewProj = gSceneData.ViewProjection;
    float4 world = mul(float4(screenPos, 0, 1), viewProj);
    world.xyz /= world.w;
    float3 origin = gSceneData.CameraPosition.xyz;
    float3 direction = normalize(world.xyz - origin);
 
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.01f;       //-> zNear
    ray.TMax = 100000.0f;   //-> zFar
    //RAY_FLAG_CULL_BACK_FACING_TRIANGLES
    //RAY_FLAG_FORCE_NON_OPAQUE 
    TraceRay(gSceneTopLevel, 
        RAY_FLAG_FORCE_NON_OPAQUE,
        0xFF, 
        0, 
        2, 
        0, 
        ray, 
        payload);
    
    gRaytraceScene[launchIndex] = payload.Color;
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
