#pragma once

#include <cmath>

#include "types.hpp"

#include <iostream>

// Constant and common maths functions
namespace maths
{
    const float TAU = 6.283185307179586476925f;

    inline float cos(float x) { return cosf(x); }
    inline float sin(float x) { return sinf(x); }
    inline float tan(float x) { return tanf(x); }
}

namespace mat4
{
    inline mat4x4 identity()
    {
        return {
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f,
        };
    }

    mat4x4 translate(const float3& value);

    mat4x4 scale(const float3& value);

    mat4x4 rotateX(float angleRadians);

    mat4x4 rotateY(float angleRadians);

    mat4x4 rotateZ(float angleRadians);

    mat4x4 perspective(float fovY, float aspect, float near, float far);

    mat4x4 frustum(float left, float right, float bottom, float top, float near, float far);
}

inline float4 operator*(float scale, float4 v)
{
    return { v.x * scale, v.y * scale , v.z * scale , v.w * scale };
}

inline float4 operator+(float4 v1, float4 v2)
{
    return { v1.x + v2.x, v1.y + v2.y , v1.z + v2.z ,v1.w + v2.w };
}

inline float4 operator*(const mat4x4& m, float4 v)
{
    float4 result;

    for (int i = 0; i < 4; i++)
    {
        float sum = 0.f;
        for (int j = 0; j < 4; j++)
            sum += m.c[i].e[j] * v.e[j];

        result.e[i] = sum;
    }

    return result;
}

inline mat4x4 operator*(const mat4x4& a, const mat4x4& b)
{
    mat4x4 result;

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            float sum = 0.f;
            for (int k = 0; k < 4; k++)
                sum += a.c[i].e[k] * b.c[k].e[j];

            result.c[i].e[j] = sum;
        }
    }

    return result;
}

inline float3 operator*(float3 v, float a)
{
    return { v.x * a, v.y * a, v.z * a };
}

inline float3 operator*(float a, float3 v)
{
    return { v * a };
}

inline float3 operator+(float3 v1, float3 v2)
{
    return { v1.x + v1.x, v1.y + v1.y, v1.z + v1.z };
}

inline float4 operator*(float4 v, float a)
{
    return { v.x * a, v.y * a, v.z * a, v.w * a };
}

inline float3 operator/(float3 v, float a)
{
    return { v.x / a, v.y / a, v.z / a };
}

inline float4 operator*(float4 v1, float4 v2)
{
    return { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w };
}

float3 getSphericalCoords(float r, float theta, float phi);

float min(float value1, float value2);

float max(float value1, float value2);