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

struct PushConstant
{
    int ignoreTemporaly;
    int simpleMode;
    float2 screenSize;
};
[[vk::push_constant]] ConstantBuffer<PushConstant> param : register(b0, space0);
[[vk::binding(0, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(1, 0)]] Texture2D currentTexture : register(t0, space0);
[[vk::binding(2, 0)]] Texture2D temporalTexture : register(t1, space0);
[[vk::binding(3, 0)]] Texture2D velocityTexture : register(t2, space0);
[[vk::binding(4, 0)]] Texture2D depthTexture : register(t3, space0);

float3 RGB2YCoCgR(float3 RGB)
{
    float Y = dot(RGB, float3(1, 2, 1));
    float Co = dot(RGB, float3(2, 0, -2));
    float Cg = dot(RGB, float3(-1, 2, -1));
	                
    float3 YCoCg = float3(Y, Co, Cg);
    return YCoCg;
}

float3 YCoCgR2RGB(float3 YCoCg)
{
    float Y = YCoCg.x * 0.25;
    float Co = YCoCg.y * 0.25;
    float Cg = YCoCg.z * 0.25;

    float R = Y + Co - Cg;
    float G = Y + Cg;
    float B = Y - Co - Cg;

    float3 RGB = float3(R, G, B);
    return RGB;
}

float Luminance(float3 color)
{
    return 0.25 * color.r + 0.5 * color.g + 0.25 * color.b;
}

float3 ToneMap(float3 color)
{
    return color / (1 + Luminance(color));
}

float3 UnToneMap(float3 color)
{
    return color / (1 - Luminance(color));
}

float2 getClosestUV(float2 uv)
{
    float2 deltaRes =  1.0 / param.screenSize;
    float closestDepth = 1.0f;
    float2 closestUV = uv;
    
    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            float2 newUV = uv + deltaRes * float2(i, j);

            float depth = currentTexture.Sample(mainSampler, newUV).x;

            if (depth < closestDepth)
            {
                closestDepth = depth;
                closestUV = newUV;
            }
        }
    }
    
    return closestUV;
}

float3 clipHistory(float2 uv, float3 nowColor, float3 preColor)
{
    float3 aabbMin = nowColor, aabbMax = nowColor;
    float2 deltaRes = 1.0 / param.screenSize;
    float3 m1 = float3(0.0, 0.0, 0.0);
    float3 m2 = float3(0.0, 0.0, 0.0);
    
    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            float2 newUV = uv + deltaRes * float2(i, j);
            float3 C = RGB2YCoCgR(ToneMap(currentTexture.Sample(mainSampler, newUV).rgb));
            m1 += C;
            m2 += C * C;
        }
    }

    // Variance clip
    const int N = 9;
    const float VarianceClipGamma = 1.0f;
    float3 mu = m1 / N;
    float3 sigma = sqrt(abs(m2 / N - mu * mu));
    aabbMin = mu - VarianceClipGamma * sigma;
    aabbMax = mu + VarianceClipGamma * sigma;

    // clip to center
    float3 p_clip = 0.5 * (aabbMax + aabbMin);
    float3 e_clip = 0.5 * (aabbMax - aabbMin);

    float3 v_clip = preColor - p_clip;
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0)
        return p_clip + v_clip / ma_unit;
    else
        return preColor;
}


float getAdaptiveWeight(float2 velocity)
{
    float speed = length(velocity);
    return clamp(speed * 0.2 + 0.02, 0.01, 0.15);
}

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    if (param.ignoreTemporaly)
    {
        return currentTexture.Sample(mainSampler, input.uv);
    }
    if (param.simpleMode)
    {
        float4 currentColor = currentTexture.Sample(mainSampler, input.uv);
        float4 historyColor = temporalTexture.Sample(mainSampler, input.uv);
        return lerp(historyColor, currentColor, 0.05);
    }
    
    float2 velocity = velocityTexture.Sample(mainSampler, getClosestUV(input.uv)).xy;
    float2 prevUV = input.uv - velocity;
    
    float3 nowColor = currentTexture.Sample(mainSampler, input.uv).rgb;
    float3 preColor = temporalTexture.Sample(mainSampler, prevUV).rgb;
    
    nowColor = RGB2YCoCgR(ToneMap(nowColor));
    preColor = RGB2YCoCgR(ToneMap(preColor));

    preColor = clipHistory(input.uv, nowColor, preColor);

    preColor = UnToneMap(YCoCgR2RGB(preColor));
    nowColor = UnToneMap(YCoCgR2RGB(nowColor));
    
    float c = 0.05;
    return float4(c * nowColor + (1 - c) * preColor, 1.0);
}
