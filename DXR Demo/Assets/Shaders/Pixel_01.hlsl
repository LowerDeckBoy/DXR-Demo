
struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(PS_INPUT pin) : SV_TARGET
{
    return pin.Color;
}
