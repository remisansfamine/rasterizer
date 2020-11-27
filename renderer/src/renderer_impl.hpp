#pragma once

#include <rdr/renderer.h>

#include <common/types.hpp>

struct Uniform
{
    float time;
    float deltaTime;

    Light lights[8];

    Texture texture;

    mat4x4 mvp;
    mat4x4 model;
    mat4x4 view;
    mat4x4 projection;

    bool depthTest = true;
    bool wireframeMode = false;
    bool backFaceCulling = true;
};

struct Varying
{
    float light;
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