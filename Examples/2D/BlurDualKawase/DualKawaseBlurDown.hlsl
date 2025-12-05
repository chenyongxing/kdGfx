struct PushConstant
{
    float2 iResolution;
    float2 texelSize;
    float4 texUVClamp;
    float offset;
};

[[vk::binding(0, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(1, 0)]] Texture2D mainTexture : register(t0, space0);
[[vk::push_constant]] ConstantBuffer<PushConstant> Param : register(b0, space0);

struct VertInput
{
    [[vk::location(0)]] float2 position : POSITION;
};

struct Vert2Pixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

[shader("vertex")]
Vert2Pixel vertexMain(VertInput input)
{
    float2 uv = input.position / Param.iResolution;

    Vert2Pixel output;
    output.uv = uv;
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    output.position.y = -output.position.y;
    return output;
}

#include "sampler.hlsli"

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float2 uv = input.uv;

    float4 color = linearSample(mainTexture, uv) * 4.0;
    color += linearSample(mainTexture, uv - Param.texelSize * Param.offset);
    color += linearSample(mainTexture, uv + Param.texelSize * Param.offset);
    color += linearSample(mainTexture, uv + float2(Param.texelSize.x, -Param.texelSize.y) * Param.offset);
    color += linearSample(mainTexture, uv - float2(Param.texelSize.x, -Param.texelSize.y) * Param.offset);

    return color / 8.0;
}
