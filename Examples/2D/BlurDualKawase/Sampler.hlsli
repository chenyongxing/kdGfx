float4 linearSample(Texture2D tex, float2 uv)
{
    uint width, height;
    tex.GetDimensions(width, height);
    float2 size = float2(width, height);
    
    int2 texClampMin = int2(Param.texUVClamp.x * width, Param.texUVClamp.y * height);
    int2 texClampMax = int2(Param.texUVClamp.z * width - 1, Param.texUVClamp.w * height - 1);
    
    uv = clamp(uv, Param.texUVClamp.xy, Param.texUVClamp.zw);
    
    float2 texelCoord = uv * float2(width, height) - float2(0.5, 0.5);
    
    int2 uvInt = int2(texelCoord);
    float2 uvFrac = frac(texelCoord);
    
    int x0 = max(uvInt.x, texClampMin.x);
    int x1 = min(uvInt.x + 1, texClampMax.x);
    int y0 = max(uvInt.y, texClampMin.y);
    int y1 = min(uvInt.y + 1, texClampMax.y);
    
    float4 c00 = tex.Sample(mainSampler, (float2(x0, y0) + 0.5) / size);
    float4 c10 = tex.Sample(mainSampler, (float2(x1, y0) + 0.5) / size);
    float4 c01 = tex.Sample(mainSampler, (float2(x0, y1) + 0.5) / size);
    float4 c11 = tex.Sample(mainSampler, (float2(x1, y1) + 0.5) / size);
    
    float4 c0 = lerp(c00, c10, uvFrac.x);
    float4 c1 = lerp(c01, c11, uvFrac.x);
    return lerp(c0, c1, uvFrac.y);
}

//float4 linearSample(Texture2D tex, float2 uv)
//{
//    uint width, height;
//    tex.GetDimensions(width, height);
    
//    float2 clampMinUV = float2(Param.texClampMin) / float2(width, height);
//    float2 clampMaxUV = float2(Param.texClampMax - 1) / float2(width, height);
//    uv = clamp(uv, clampMinUV, clampMaxUV);
    
//    float2 texelCoord = uv * float2(width, height) - float2(0.5, 0.5);
    
//    int2 uvInt = int2(floor(texelCoord));
//    float2 uvFrac = frac(texelCoord);
    
//    int x0 = max(uvInt.x, Param.texClampMin.x);
//    int x1 = min(uvInt.x + 1, Param.texClampMax.x);
//    int y0 = max(uvInt.y, Param.texClampMin.y);
//    int y1 = min(uvInt.y + 1, Param.texClampMax.y);
    
//    float4 c00 = tex.Load(int3(x0, y0, 0));
//    float4 c10 = tex.Load(int3(x1, y0, 0));
//    float4 c01 = tex.Load(int3(x0, y1, 0));
//    float4 c11 = tex.Load(int3(x1, y1, 0));
    
//    float4 c0 = lerp(c00, c10, uvFrac.x);
//    float4 c1 = lerp(c01, c11, uvFrac.x);
//    return lerp(c0, c1, uvFrac.y);
//}
