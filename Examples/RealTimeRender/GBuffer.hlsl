#include "Math.hlsli"
#include "BaseTypes.hlsli"

[[vk::binding(0, 0)]] ConstantBuffer<Param> param : register(b0, space0);
[[vk::binding(1, 0)]] SamplerState mainSampler : register(s0, space0);
[[vk::binding(2, 0)]] StructuredBuffer<Instance> instances : register(t0, space0);

struct VertInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 texCoord : TEXCOORD;
    uint instanceID : SV_StartInstanceLocation;
};

struct Vert2Pixel
{
    float4 svPosition : SV_Position;
    float4 clipPosition : CLIPPOSITION;
    float4 preClipPosition : PRECLIPPOSITION;
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    nointerpolation uint materialIndex : INDEX0;
};

// todo: 怎么还原抖动到像素中心？
[shader("vertex")]
Vert2Pixel vertexMain(VertInput input)
{
    Instance instance = instances[NonUniformResourceIndex(input.instanceID)];
    float4x4 model = instance.transform;
    float4 position = mul(model, float4(input.position, 1.0));
    Vert2Pixel output;
    output.clipPosition = mul(param.projection, mul(param.view, position));
    output.preClipPosition = mul(param.preProjection, mul(param.preView, position));
    output.svPosition = mul(param.projection0, mul(param.view, position));
    output.position = position.xyz / position.w;
    output.normal = mul(transpose(inverse((float3x3)model)), input.normal);
    output.texCoord = input.texCoord;
    output.materialIndex = instance.materialIndex;
    return output;
}

[[vk::binding(3, 0)]] StructuredBuffer<Material> materials : register(t1, space0);
[[vk::binding(4, 0)]] Texture2D textures[] : register(t2, space0);

struct PixelOutput
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 baseColor : SV_Target2;
    float2 velocity : SV_Target3;
};

[shader("pixel")]
PixelOutput pixelMain(Vert2Pixel input)
{
    PixelOutput output;
    output.position = float4(input.position, 1.0);
    
    Material material = materials[NonUniformResourceIndex(input.materialIndex)];
    output.normal = float4(normalize(input.normal), 0.0);
    if (material.normalMapIndex < UINT_MAX)
    {
        uint normalMapIndex = NonUniformResourceIndex(material.normalMapIndex);
        float3 normal = textures[normalMapIndex].Sample(mainSampler, input.texCoord).xyz;
        normal = normalize(normal * 2.0 - float3(1.0, 1.0, 1.0));
        output.normal.xyz = tangentToWorld(normal, output.normal.xyz);
    }
    
    output.baseColor = material.baseColor;
    if (material.baseColorMapIndex < UINT_MAX)
    {
        uint baseColorMapIndex = NonUniformResourceIndex(material.baseColorMapIndex);
        output.baseColor *= textures[baseColorMapIndex].Sample(mainSampler, input.texCoord);
    }
    clip(output.baseColor.a - 0.2);
    
    float3 ndcPosition = input.clipPosition.xyz / input.clipPosition.w;
    float3 preNdcPosition = input.preClipPosition.xyz / input.preClipPosition.w;
    output.velocity = ndcPosition.xy - preNdcPosition.xy;
    
    //output.velocity = (ndcPosition.xy - param.jitter) - (preNdcPosition.xy - param.preJitter);
    
    //float2 jitterDiff = (param.jitter - param.preJitter) * 0.5f;
    //output.velocity -= jitterDiff;
    
    return output;
}
