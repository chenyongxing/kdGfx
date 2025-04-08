struct PushConstant
{
    float2 screenSize;
    float2 leftTopPos;
    float2 rectSize;
    float radius;
    float attenuation;
};
[[vk::push_constant]] ConstantBuffer<PushConstant> Param : register(b0, space0);

struct VertInput
{
    [[vk::location(0)]] float2 pos : POSITION;
};

struct Vert2Pixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

float4 screenToNdc(float2 position)
{
    float2 pos = position / Param.screenSize * 2.0;
    pos = pos - float2(1.0, 1.0);
    return float4(pos.x, -pos.y, 0.0, 1.0);
}

[shader("vertex")]
Vert2Pixel vertexMain(VertInput input)
{
    Vert2Pixel output;
    output.uv = (input.pos - Param.leftTopPos) / Param.rectSize;
    output.position = screenToNdc(input.pos);
    return output;
}

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float dist = distance(input.uv, float2(0.5, 0.5));
    float aa = fwidth(dist);
    float dist2 = smoothstep(-aa, aa, Param.radius - dist);

    float alpha = Param.radius - dist;
    alpha = 1.0 - alpha - Param.attenuation;
    alpha *= dist2;
    return float4(float3(dist2, dist2, dist2), alpha);
}
