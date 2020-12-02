#pragma once

#include <rdr/renderer.h>

#include <common/types.hpp>

struct Uniform
{
    float time;
    float deltaTime;

    float4 ambient = {0.2f, 0.2f, 0.2f, 1.f};
    float shiness = 20.f;

    Light lights[8];

    Texture texture;

    mat4x4 mvp;
    mat4x4 model;
    mat4x4 view;
    mat4x4 projection;

    bool depthTest = true;
    bool wireframeMode = false;
    bool backFaceCulling = true;
    bool lightPerPixel = false;
};

struct Varying
{
    float3 coords;
    float3 normal;
    float4 color;
    float2 uv;
};

struct Viewport
{
    int x;
    int y;
    int width;
    int height;
};

struct Framebuffer
{
    int width;
    int height;
    float4* colorBuffer;
    float* depthBuffer;
};

struct rdrImpl
{
    Framebuffer fb;
    Viewport viewport;

    float4 lineColor = { 1.f, 1.f, 1.f, 1.f };

    Uniform uniform;
};