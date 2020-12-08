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
    float2 xy;
};


union mat4x4
{
    float  e[16];
    float4 c[4];
};

struct Texture
{
    std::string fileName;
    int width = 0, height = 0;
    float4* data = nullptr;
};

struct Light
{
    bool    isEnable = false;
    float4  lightPos = { 0.f, 0.f, 0.f, 1.f };
    float4  ambient = { 0.f, 0.f, 0.f, 0.f };
    float4  diffuse  = { 1.f, 1.f, 1.f, 1.f };
    float4  specular = { 0.f, 0.f, 0.f, 0.f };
    float   constantAttenuation = 1.f;
    float   linearAttenuation = 0.f;
    float   quadraticAttenuation = 0.f;
};