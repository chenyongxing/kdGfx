struct VertInput
{
    [[vk::location(0)]] float2 pos : POSITION;
    [[vk::location(1)]] float2 uv : TEXCOORD;
};

struct Vert2Pixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

[[vk::binding(0, 0)]] cbuffer Constant : register(b0, space0)
{
    float2 screenSize;
};

float4 screenToNdc(float2 position)
{
    float2 pos = position / screenSize * 2.0;
    pos = pos - float2(1.0, 1.0);
    return float4(pos.x, -pos.y, 0.0, 1.0);
}

[shader("vertex")]
Vert2Pixel vertexMain(VertInput input)
{
    Vert2Pixel output;
    output.uv = input.uv;
    output.position = screenToNdc(input.pos);
    return output;
}

[[vk::binding(1, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(2, 0)]] Texture2D mainTexture : register(t0, space0);

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    return mainTexture.Sample(mainSampler, input.uv);
}
