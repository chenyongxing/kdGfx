struct PushConstant
{
    float4 color;
    float4 rect;
    float aspectRatio;
    float radius;
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

float roundRectSDF(float2 center, float2 size, float radius)
{
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float2 uv = input.uv * 2.0;
    uv.x *= Param.aspectRatio;
    
    float2 rectSize = Param.rect.zw * 0.5;
    float d = roundRectSDF(uv - Param.rect.xy - rectSize, rectSize, Param.radius);
    float aa = fwidth(d) * 0.5;
    d = smoothstep(-aa, aa, d);
    float a = 1.0 - d;
    return float4(Param.color.rgb, Param.color.a * a);
}
