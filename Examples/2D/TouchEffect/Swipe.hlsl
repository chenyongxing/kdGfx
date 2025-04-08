struct PushConstant
{
    float2 screenSize;
    float2 _pad0;
    float4 color;
};
[[vk::push_constant]] ConstantBuffer<PushConstant> Param : register(b0, space0);

struct VertInput
{
    [[vk::location(0)]] float2 pos : POSITION;
};

float4 screenToNdc(float2 position)
{
    float2 pos = position / Param.screenSize * 2.0;
    pos = pos - float2(1.0, 1.0);
    return float4(pos.x, -pos.y, 0.0, 1.0);
}

[shader("vertex")]
float4 vertexMain(VertInput input) : SV_Position
{
    return screenToNdc(input.pos);
}

[[vk::binding(0, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(1, 0)]] Texture2D mainTexture : register(t0, space0);

[shader("pixel")]
float4 pixelMain() : SV_Target
{
    return Param.color;
}
