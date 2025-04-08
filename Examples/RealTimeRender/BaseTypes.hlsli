#pragma once

struct Param
{
    // GBuffer
    float4x4 projection0; //use jitter
    float4x4 view;
    float4x4 projection;
    float4x4 preView;
    float4x4 preProjection;
    float2 jitter;
    float2 preJitter;
    // Lighting
    int haveDirLight;
    float3 dirLightDir;
    float3 viewPos;
    // ToneMapping
    float gamma;
};

struct Instance
{
    float4x4 transform;
    uint index;
    uint materialIndex;
    uint selected;
    float _pad0;
};

struct Material
{
    float3 emissive;
    int emissiveMapIndex;
    float4 baseColor;
    int alphaMode;
    float alphaCutoff;
    uint baseColorMapIndex;
    float metallic;
    float roughness;
    uint occlusionRoughnessMetallicMapIndex;
    uint normalMapIndex;
    float ior;
    float sheenRoughness;
    float clearcoat;
    float clearcoatRoughness;
    float transmission;
    float3 attenuationColor;
    float attenuationDistance;
    float3 sheenColor;
    float _pad0;
};
