struct PushConstant
{
    float4 rect;
};

[[vk::push_constant]] ConstantBuffer<PushConstant> Param : register(b0, space0);

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

[[vk::binding(0, 0)]] SamplerState MainSampler : register(s0, space0);
[[vk::binding(1, 0)]] Texture2D MainTexture : register(t0, space0);
[[vk::binding(2, 0)]] Texture2D MaskTexture : register(t1, space0);

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float2 uv = lerp(Param.rect.xy, Param.rect.zw, input.uv);
    float4 imagePixel = MainTexture.Sample(MainSampler, uv);
    float mask = MaskTexture.Sample(MainSampler, input.uv).a;
    return imagePixel * mask;
}
