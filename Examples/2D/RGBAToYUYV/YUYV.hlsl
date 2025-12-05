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

[[vk::binding(0, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(1, 0)]] Texture2D mainTexture : register(t0, space0);

float3 RGBtoYUV(float3 rgb)
{
    return float3(
        dot(rgb, float3(0.299, 0.587, 0.114)),
        dot(rgb, float3(-0.169, -0.331, 0.5)) + 0.5,
        dot(rgb, float3(0.5, -0.419, -0.081)) + 0.5
    );
}

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    uint width, height;
    mainTexture.GetDimensions(width, height);
    float dx = 1.0 / width;
    
    float4 color0 = mainTexture.Sample(mainSampler, input.uv + float2(-0.5 * dx, 0));
    float4 color1 = mainTexture.Sample(mainSampler, input.uv + float2(+0.5 * dx, 0));

    float3 yuv0 = RGBtoYUV(color0.rgb);
    float3 yuv1 = RGBtoYUV(color1.rgb);
    
    return float4(
        yuv0.x, // Y0
        (yuv0.y + yuv1.y) * 0.5, // U avg
        yuv1.x, // Y1
        (yuv0.z + yuv1.z) * 0.5 // V avg
    );
}
