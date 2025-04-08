struct Vert2Pixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

[shader("vertex")]
Vert2Pixel vertexMain(uint vertexID: SV_VertexID)
{
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    
    Vert2Pixel output;
    output.uv = float2(uv.x, 1.0 - uv.y);
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

struct PushConstant
{
    float2 iResolution;
    float2 renderSize;
};

[[vk::binding(0, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(1, 0)]] Texture2D mainTexture : register(t0, space0);
[[vk::push_constant]] ConstantBuffer<PushConstant> Param : register(b0, space0);

#define FIX(c) max(abs(c), 1e-5);

const static float PI = 3.1415926535897932384626433832795;

float3 weight3(float x)
{
    const float radius = 3.0;
    float3 texel = FIX(2.0 * PI * float3(x - 1.5, x - 0.5, x + 0.5));

    // Lanczos3. Note: we normalize outside this function, so no point in multiplying by radius.
    return /*radius **/ sin(texel) * sin(texel / radius) / (texel * texel);
}

float3 pixel(float xpos, float ypos)
{
    return mainTexture.Sample(mainSampler, float2(xpos, ypos)).rgb;
}

float3 lineaps(float ypos, float3 xpos1, float3 xpos2, float3 linetaps1, float3 linetaps2)
{
    return
        pixel(xpos1.r, ypos) * linetaps1.r +
        pixel(xpos1.g, ypos) * linetaps2.r +
           pixel(xpos1.b, ypos) * linetaps1.g +
        pixel(xpos2.r, ypos) * linetaps2.g +
           pixel(xpos2.g, ypos) * linetaps1.b +
        pixel(xpos2.b, ypos) * linetaps2.b;
}

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float2 stepxy = 1.0 / Param.iResolution;
    float2 texCoord = input.uv;

    float2 pos = texCoord.xy + stepxy * 0.5;
    float2 f = frac(pos / stepxy);

    float3 linetaps1 = weight3(0.5 - f.x * 0.5);
    float3 linetaps2 = weight3(1.0 - f.x * 0.5);
    float3 columntaps1 = weight3(0.5 - f.y * 0.5);
    float3 columntaps2 = weight3(1.0 - f.y * 0.5);

    // make sure all taps added together is exactly 1.0, otherwise some
    // (very small) distortion can occur
    float suml = dot(linetaps1, float3(1.0, 1.0, 1.0)) + dot(linetaps2, float3(1.0, 1.0, 1.0));
    float sumc = dot(columntaps1, float3(1.0, 1.0, 1.0)) + dot(columntaps2, float3(1.0, 1.0, 1.0));
    linetaps1 /= suml;
    linetaps2 /= suml;
    columntaps1 /= sumc;
    columntaps2 /= sumc;

    float2 xystart = (-2.5 - f) * stepxy + pos;
    float3 xpos1 = float3(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0);
    float3 xpos2 = float3(xystart.x + stepxy.x * 3.0, xystart.x + stepxy.x * 4.0, xystart.x + stepxy.x * 5.0);

    return float4(
        lineaps(xystart.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r +
                        lineaps(xystart.y + stepxy.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r +
                        lineaps(xystart.y + stepxy.y * 2.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g +
                        lineaps(xystart.y + stepxy.y * 3.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g +
                        lineaps(xystart.y + stepxy.y * 4.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b +
                        lineaps(xystart.y + stepxy.y * 5.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b,
                    1.0);
}
