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

[[vk::binding(0, 0)]] cbuffer Constant : register(b0, space0)
{
    float4 shadowColor;
    float4 windowRoundRadius;
    float4 windowTitleColor;
    float2 shadowTopLeft;
    float2 shadowBottomRight;
    float2 windowOffset;
    float2 windowSize;
    float aspect;
    float shadowRadius;
    float shadowSigma;
    float windowTitleHeight;
    float3 windowBorderColor;
    float windowBorderThickness;
};

// https://madebyevan.com/shaders/fast-rounded-rectangle-shadows/

// This approximates the error function, needed for the gaussian integral
float4 erf(float4 x)
{
    float4 s = sign(x), a = abs(x);
    x = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
    x *= x;
    return s - s / (x * x);
}

// Return the mask for the shadow of a box from lower to upper
float boxShadow(float2 lower, float2 upper, float2 pos, float sigma)
{
    float4 query = float4(pos - lower, pos - upper);
    float4 integral = 0.5 + 0.5 * erf(query * (sqrt(0.5) / sigma));
    return (integral.z - integral.x) * (integral.w - integral.y);
}

float sdRoundedBox(in float2 p, in float2 b, in float4 r)
{
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x = (p.y > 0.0) ? r.x : r.y;
    float2 q = abs(p) - b + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float4 outColor;

    // shadow
    float shadow = boxShadow(shadowTopLeft, shadowBottomRight, input.position.xy, shadowSigma);

    // window
    float2 st = input.uv * 2.0;
    st.x *= aspect;
    float2 offset = -windowOffset * 2.0;

    float windowDist = sdRoundedBox(st + offset, windowSize, windowRoundRadius);
    float aa = fwidth(windowDist);
    float shapeWindow = smoothstep(0.0, aa, windowDist);

    outColor = lerp(float4(0.0, 0.0, 0.0, 0.0), shadowColor, shapeWindow * shadow);

    // title
    if (windowTitleHeight > 0.001)
    {
        float titleDist = sdRoundedBox
        (
            st + offset + float2(0.0, windowSize.y - windowTitleHeight - windowBorderThickness),
            float2(windowSize.x - windowBorderThickness, windowTitleHeight),
            float4(0.0, windowRoundRadius.y - windowBorderThickness, 0.0, windowRoundRadius.w - windowBorderThickness)
        );
        float shapeTitle = 1.0 - smoothstep(0.0, aa, titleDist);
        outColor += shapeTitle * windowTitleColor;
    }

    // border
    float shapeWindowInner = 1.0 - smoothstep(0.0, aa, windowDist + windowBorderThickness);
    outColor += (1.0 - shapeWindow - shapeWindowInner) * float4(windowBorderColor, 1.0);

    return outColor;
}
