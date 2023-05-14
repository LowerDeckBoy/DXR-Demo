
struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    output.Position = float4(vin.Position, 1.0f);
    output.Color = vin.Color;
    
    return output;
}
