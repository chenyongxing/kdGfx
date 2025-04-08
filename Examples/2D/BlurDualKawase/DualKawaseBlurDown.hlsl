struct Vert2Pixel
{
    float4 position : SV_Position;
};

[shader("vertex")]
Vert2Pixel vertexMain(uint vertexID: SV_VertexID)
{
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);

    Vert2Pixel output;
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

struct PushConstant
{
    float2 iResolution;
    float2 texelSize;
    float offset;
};

[[vk::binding(0, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(1, 0)]] Texture2D mainTexture : register(t0, space0);
[[vk::push_constant]] ConstantBuffer<PushConstant> Param : register(b0, space0);

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float2 uv = input.position.xy / Param.iResolution;

    float4 color = mainTexture.Sample(mainSampler, uv) * 4.0;
    color += mainTexture.Sample(mainSampler, uv - Param.texelSize * Param.offset);
    color += mainTexture.Sample(mainSampler, uv + Param.texelSize * Param.offset);
    color += mainTexture.Sample(mainSampler, uv + float2(Param.texelSize.x, -Param.texelSize.y) * Param.offset);
    color += mainTexture.Sample(mainSampler, uv - float2(Param.texelSize.x, -Param.texelSize.y) * Param.offset);

    return color / 8.0;
}
