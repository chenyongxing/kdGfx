#include "BaseTypes.hlsli"

struct Vert2Pixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

[shader("vertex")]
Vert2Pixel vertexMain(uint vertexID : SV_VertexID)
{
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);

    Vert2Pixel output;
    output.uv = float2(uv.x, 1.0 - uv.y);
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

[[vk::binding(0, 0)]] ConstantBuffer<Param> param : register(b0, space0);
[[vk::binding(1, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(2, 0)]] Texture2D mainTexture : register(t0, space0);

float3 linearToSrgb(float3 v, float gamma)
{
    float invGamma = 1.0 / gamma;
    return pow(v, float3(invGamma, invGamma, invGamma));
}

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float4 color = mainTexture.Sample(mainSampler, input.uv);
    color.rgb = linearToSrgb(color.rgb, param.gamma);
    return color;
}
