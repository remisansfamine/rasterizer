#pragma once

union float3
{
    float e[3];
    struct { float x; float y; float z; };
};

float3 operator-(const float3&);

union float4
{
    float4() = default;
    float4(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {}

    float4 (float3 xyz, float w)
        : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {}

    float e[4];
    struct { float x; float y; float z; float w; };
    struct { float r; float g; float b; float a; };
    float3 xyz;
};


union mat4x4
{
    float e[16];
    float4 c[4];
};

struct Texture
{
    int width, height;
    float* data;
};