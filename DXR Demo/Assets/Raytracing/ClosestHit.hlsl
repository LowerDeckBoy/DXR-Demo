#ifndef CLOSESTHIT_HLSL
#define CLOSESTHIT_HLSL

#include "Common.hlsli"
#include "HitCommon.hlsli"

uint3 GetIndices(uint triangleIndex)
{
    uint baseIndex = (triangleIndex * 3);
    int address = (baseIndex * 4);
    return Indices.Load3(address);
}
/*
ModelVertex GetVertexAttributes(uint triangleIndex, float3 barycentrics)
{
    uint3 indices = GetIndices(triangleIndex);
    ModelVertex vertex = (ModelVertex)0;
    vertex.Position     = float3(0.0f, 0.0f, 0.0f);
    vertex.TexCoord     = float2(0.0f, 0.0f);
    vertex.Normal       = float3(0.0f, 0.0f, 0.0f);
    vertex.Tangent      = float3(0.0f, 0.0f, 0.0f);
    vertex.Bitangent    = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < 3; i++)
    {
        //uint address = (indices[i] * 14) * 4;
        uint address = indices[i] * 56;
        vertex.Position  += asfloat(Vertices.Load3(address)) * barycentrics[i];
        address          += 12;
        vertex.TexCoord  += asfloat(Vertices.Load2(address)) * barycentrics[i];
        address          += 8;
        vertex.Normal    += asfloat(Vertices.Load3(address)) * barycentrics[i];
        address          += 12;
        vertex.Tangent   += asfloat(Vertices.Load3(address)) * barycentrics[i];
        address          += 12;
        vertex.Bitangent += asfloat(Vertices.Load3(address)) * barycentrics[i];
        //address += 12;
    }
    
    return vertex;
}
*/
uint3 LoadIndices(uint offsetBytes, ByteAddressBuffer Indices)
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
    //const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);
    const uint3 four16BitIndices = Indices.Load3(dwordAlignedOffset);

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


ModelVertex GetVertex(uint TriangleIndices, float3 barycentrics)
{   
    uint3 indices = GetIndices(TriangleIndices);
    
    ModelVertex vertex = (ModelVertex)0;
    vertex.Position     = float3(0.0f, 0.0f, 0.0f);
    vertex.TexCoord     = float2(0.0f, 0.0f);
    vertex.Normal       = float3(0.0f, 0.0f, 0.0f);
    vertex.Tangent      = float3(0.0f, 0.0f, 0.0f);
    vertex.Bitangent    = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < 3; i++)
    {
        vertex.Position  += Vertices[indices[i]].Position * barycentrics[i];
        vertex.TexCoord  += Vertices[indices[i]].TexCoord * barycentrics[i];
        vertex.Normal    += Vertices[indices[i]].Normal * barycentrics[i];
        vertex.Tangent   += Vertices[indices[i]].Tangent * barycentrics[i];
        vertex.Bitangent += Vertices[indices[i]].Bitangent * barycentrics[i];
    }
    
    return vertex;
}
/**/
//https://link.springer.com/content/pdf/10.1007/978-1-4842-4427-2_3.pdf
[shader("closesthit")]
void ClosestHit(inout HitPayload Payload, BuiltInTriangleIntersectionAttributes Attribs)
{
    //uint triangleIndex = PrimitiveIndex();
    float3 barycentrics = float3((1.0f - Attribs.barycentrics.x - Attribs.barycentrics.y), Attribs.barycentrics.x, Attribs.barycentrics.y);
    //uint3 indices = LoadIndices(PrimitiveIndex() * 12, Indices);
    //uint3 indices = GetIndices(PrimitiveIndex());
    ModelVertex vertex = GetVertex(PrimitiveIndex(), barycentrics);
    //ModelVertex vertex = GetVertexAttributes(PrimitiveIndex(), barycentrics);

    uint2 idx = DispatchRaysIndex();
    
    //float2 texCoord = (TextureResolution.TexResolution.xy + 0.5f / TextureResolution.TexResolution.xy) * 2.0f - 1.0f;
    //int2 coords = floor(vertex.TexCoord * (TextureResolution.TexResolution.xy));
    int2 coords = floor(vertex.TexCoord );
    
    //float3 color = BaseColorTexture.Load(int3(coords, 0)).rgb;
    float3 color = BaseColorTexture.Load(int3(coords, 0)).rgb;

    Payload.ColorAndT = float4(color, RayTCurrent());
    
    //float3 pixelToLight = normalize(SceneData.LightPosition.xyz - WorldPosition());
    //float NdotL = max(0.0f, dot(pixelToLight, triangleNormal));
    //float4 color = saturate(Albedo * SceneData.LightDiffuse * NdotL);
    
}

#endif // CLOSESTHIT_HLSL
