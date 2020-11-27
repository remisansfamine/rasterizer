#pragma once

#include <cmath>

#include "types.hpp"

#include <string>

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

inline float2 operator*(const float2& v, float a)
{
    return { v.x * a, v.y * a };
}

inline float2 operator*(float a, const float2& v)
{
    return { v * a };
}

inline float2 operator+(const float2& v1, const float2& v2)
{
    return { v1.x + v2.x, v1.y + v2.y };
}

inline float2 operator-(const float2& f)
{
    return { -f.x, -f.y };
}

inline float2 operator-(const float2& v1, const float2& v2)
{
    return { v1.x + -v2.x, v1.y + -v2.y };
}

inline float2 operator/(const float2& v, float a)
{
    return { v.x / a, v.y / a };
}

inline float4 operator+(const float4& v1, const float4& v2)
{
    return { v1.x + v2.x, v1.y + v2.y , v1.z + v2.z ,v1.w + v2.w };
}

inline float4 operator+=(float4& v1, const float4& v2)
{
    return v1 = v1 + v2;
}

inline float4 operator-(const float4& v)
{
    return { -v.x, -v.y, -v.z, -v.w };
}

inline float4 operator-(const float4& v1, const float4& v2)
{
    return { v1.x + -v2.x, v1.y + -v2.y, v1.z + -v2.z, v1.w + -v2.w };
}

inline float4 operator*(const float4& v, float scale)
{
    return { v.x * scale, v.y * scale, v.z * scale, v.w * scale };
}

inline float4 operator*(float scale, const float4& v)
{
    return { v.x * scale, v.y * scale , v.z * scale , v.w * scale };
}

inline float4 operator/(const float4& v, float scale)
{
    return { v.x / scale, v.y / scale, v.z / scale, v.w / scale };
}

inline float4 operator/(float scale, const float4& v)
{
    return { v.x / scale, v.y /scale , v.z /scale , v.w / scale };
}

inline float4 operator*(const float4& v1, const float4& v2)
{
    return { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w };
}

inline float4 operator*=(float4& v1, const float4& v2)
{
    return v1 = v1 * v2;
}

inline float4 operator*(const mat4x4& m, const float4& v)
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

inline float3 operator*(const float3& v, float a)
{
    return { v.x * a, v.y * a, v.z * a };
}

inline float3 operator*(float a, const float3& v)
{
    return { v * a };
}

inline float3 operator*(const float3& v1, const float3& v2)
{
    return { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
}

inline float3 operator+(const float3& v1, const float3& v2)
{
    return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

inline float3 operator-(const float3& f)
{
    return { -f.x, -f.y, -f.z };
}

inline float3 operator-(const float3& v1, const float3& v2)
{
    return { v1.x + -v2.x, v1.y + -v2.y, v1.z + -v2.z };
}

inline float3 operator/(const float3& v, float a)
{
    return { v.x / a, v.y / a, v.z / a };
}

inline float3 operator^(const float3& v1, const float3& v2)
{
    return {
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    };
}

float3 getSphericalCoords(float r, float theta, float phi);

inline float magnitude(const float3& v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float magnitude(const float4& v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline float3 normalized(const float3& v)
{
    float magn = magnitude(v);
    return magn == 0 ? v : v / magn;
}

inline float4 normalized(const float4& v)
{
    float magn = magnitude(v);
    return magn == 0 ? v : v / magn;
}

inline float remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    return (value - oldMin) * (newMax - newMin) / (newMin - oldMin) + newMin;
}

inline float min(float value1, float value2)
{
    return value1 <= value2 ? value1 : value2;
}

inline float max(float value1, float value2)
{
    return value1 <= value2 ? value2 : value1;
}

inline float saturate(float value)
{
    return min(max(value, 0.f), 1.f);
}

inline float dot(const float3& v1, const float3& v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}