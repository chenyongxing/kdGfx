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
[[vk::binding(2, 0)]] Texture2D positionTexture : register(t0, space0);
[[vk::binding(3, 0)]] Texture2D normalTexture : register(t1, space0);
[[vk::binding(4, 0)]] Texture2D baseColorTexture : register(t2, space0);

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float3 baseColor = baseColorTexture.Sample(mainSampler, input.uv).rgb;
    float3 position = positionTexture.Sample(mainSampler, input.uv).xyz;
    float3 N = normalTexture.Sample(mainSampler, input.uv).xyz;
    float3 V = normalize(param.viewPos - position);
    float3 L = param.dirLightDir;
    float3 H = normalize(L + V);
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    
    float3 ambient = baseColor * 0.1;
    float3 diffuse = baseColor * NoL;
    float3 specular = float3(0.3, 0.3, 0.3) * pow(NoH, 16.0);
    return float4(ambient + diffuse + specular, 1.0);
}
