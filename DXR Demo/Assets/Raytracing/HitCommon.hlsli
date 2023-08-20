#ifndef HITCOMMON_HLSL
#define HITCOMMON_HLSL

struct HitPayload
{
    float4 ColorAndT;
};

struct HitAttributes
{
    float2 UV;
};

float3 WorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}


#endif // HITCOMMON_HLSL
