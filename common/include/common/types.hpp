#pragma once

#include <string>

union float2
{
    float2() = default;
    float2(float x, float y)
        : x(x), y(y)
    {}

    float e[2];
    struct { float x; float y; };
    struct { float u; float v; };
};

union float3
{
    float3() = default;
    float3(float x, float y, float z)
        : x(x), y(y), z(z)
    {}

    float3(float2 xy, float z = 0.f)
        : x(xy.x), y(xy.y), z(z)
    {}

    float e[3];
    struct { float x; float y; float z; };
    struct { float r; float g; float b; };
    float2 xy;
};

union float4
{
    float4() = default;
    float4(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {}

    float4 (float3 xyz, float w = 0.f)
        : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {}

    float e[4];
    struct { float x; float y; float z; float w; };
    struct { float r; float g; float b; float a; };
    float3 xyz;
    float3 rgb;
    float2 xy;
};


union mat4x4
{
    float  e[16];
    float4 c[4];
};