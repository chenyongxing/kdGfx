struct PushConstant
{
    float2 screenSize;
    float2 leftTopPos;
    float2 rectSize;
    float endAngle;
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

// Inspired by https://www.shadertoy.com/view/Mtlyz7
#define PI 3.141592653589793

static const float innerRadius = 0.7;
static const float outerRadius = 1.0;
static const float3 fgColor = float3(0.0, 0.68, 1.0);
static const float3 bgColor = float3(0.6, 0.6, 0.6);

[shader("pixel")]
float4 pixelMain(Vert2Pixel input) : SV_Target
{
    float2 st = input.uv * 2.0 - 1.0;
    float dist = length(st);
    float aa = fwidth(dist);

    float outer = smoothstep(-aa, aa, outerRadius - dist);
    float inner = smoothstep(-aa, aa, dist - innerRadius);

    float angle = atan2(st.y, st.x) + PI;
    float mask = smoothstep(-aa, aa, Param.endAngle - angle);

    float bandAlpha = outer * inner;
    float4 bg = float4(bgColor, bandAlpha);
    float4 fg = float4(fgColor, bandAlpha * mask);
    float3 color = lerp(bg.rgb, fg.rgb, fg.a);
    return float4(color, bg.a);
}
