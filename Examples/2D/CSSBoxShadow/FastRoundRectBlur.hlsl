struct PushConstant
{
    float4 color;
    float2 topLeft;
    float2 bottomRight;
    float sigma;
    float radius;
    float2 _pad0;
    // mask
    float4 sdfRect;
    float aspectRatio;
    float sdfRadius;
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

// A standard gaussian function, used for weighting samples
float gaussian(float x, float sigma)
{
    const float pi = 3.141592653589793;
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (sqrt(2.0 * pi) * sigma);
}

// This approximates the error function, needed for the gaussian integral
float2 erf(float2 x)
{
    float2 s = sign(x), a = abs(x);
    x = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
    x *= x;
    return s - s / (x * x);
}

// Return the blurred mask along the x dimension
float roundedBoxShadowX(float x, float y, float sigma, float corner, float2 halfSize)
{
    float delta = min(halfSize.y - corner - abs(y), 0.0);
    float curved = halfSize.x - corner + sqrt(max(0.0, corner * corner - delta * delta));
    float2 integral = 0.5 + 0.5 * erf((x + float2(-curved, curved)) * (sqrt(0.5) / sigma));
    return integral.y - integral.x;
}

#define MULTI_SAMPLE_COUNT 3

// Return the mask for the shadow of a box from lower to upper
float roundedBoxShadow(float2 lower, float2 upper, float2 pos, float sigma, float corner)
{
  // Center everything to make the math easier
    float2 center = (lower + upper) * 0.5;
    float2 halfSize = (upper - lower) * 0.5;
    pos -= center;

  // The signal is only non-zero in a limited range, so don't waste samples
    float low = pos.y - halfSize.y;
    float high = pos.y + halfSize.y;
    float start = clamp(-3.0 * sigma, low, high);
    float end = clamp(3.0 * sigma, low, high);

  // Accumulate samples
    float step = (end - start) / MULTI_SAMPLE_COUNT;
    float y = start + step * 0.5;
    float value = 0.0;
    for (int i = 0; i < MULTI_SAMPLE_COUNT; i++)
    {
        value += roundedBoxShadowX(pos.x, pos.y - y, sigma, corner, halfSize) * gaussian(y, sigma) * step;
        y += step;
    }

    return value;
}

float roundRectSDF(float2 center, float2 size, float radius)
{
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float2 uv = input.uv * 2.0;
    uv.x *= Param.aspectRatio;
    
    float2 rectSize = Param.sdfRect.zw * 0.5;
    float d = roundRectSDF(uv - Param.sdfRect.xy - rectSize, rectSize, Param.sdfRadius);
    float aa = fwidth(d) * 0.5;
    float mask = smoothstep(-aa, aa, d);
    
    float shadow = roundedBoxShadow(Param.topLeft, Param.bottomRight, input.position.xy, Param.sigma, Param.radius);
    return float4(Param.color.rgb, Param.color.a * shadow * mask);
}
