#pragma once

#define UINT_MAX 0xFFFFFFFF

inline float3x3 inverse(float3x3 m)
{
    float det = m._11 * m._22 * m._33 
              + m._12 * m._23 * m._31 
              + m._13 * m._21 * m._32 
              - m._13 * m._22 * m._31 
              - m._12 * m._21 * m._33 
              - m._11 * m._23 * m._32;

    float invDet = 1.0 / det;

    float3x3 adjugate;
    adjugate._11 = +(m._22 * m._33 - m._23 * m._32);
    adjugate._12 = -(m._12 * m._33 - m._13 * m._32);
    adjugate._13 = +(m._12 * m._23 - m._13 * m._22);
    
    adjugate._21 = -(m._21 * m._33 - m._23 * m._31);
    adjugate._22 = +(m._11 * m._33 - m._13 * m._31);
    adjugate._23 = -(m._11 * m._23 - m._13 * m._21);
    
    adjugate._31 = +(m._21 * m._32 - m._22 * m._31);
    adjugate._32 = -(m._11 * m._32 - m._12 * m._31);
    adjugate._33 = +(m._11 * m._22 - m._12 * m._21);

    float3x3 invM = adjugate * invDet;
    return invM;
}

float3 faceForward(float3 normal, float3 dir)
{
    float sign = dot(dir, normal) >= 0.0 ? 1.0 : -1.0;
    return sign * normal;
}

float3 tangentToWorld(float3 dir, float3 T, float3 B, float3 N)
{
    return dir.x * T + dir.y * B + dir.z * N;
}

float3 worldToTangent(float3 dir, float3 T, float3 B, float3 N)
{
    return float3(dot(dir, T), dot(dir, B), dot(dir, N));
}

void genTangentBasis(in float3 N, out float3 T, out float3 B)
{
    float sign = N.z >= 0.0 ? 1.0 : -1.0;
    float a = -1.0 / (sign + N.z);
    float b = N.x * N.y * a;
    T = float3(1 + sign * a * N.x * N.x, sign * b, -sign * N.x);
    B = float3(b, sign + a * N.y * N.y, -N.y);
}

float3 tangentToWorld(float3 dir, float3 N)
{
    float3 T, B;
    genTangentBasis(N, T, B);
    return tangentToWorld(dir, T, B, N);
}
