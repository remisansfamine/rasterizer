#pragma once

#include <rdr/renderer.h>

#include <common/types.hpp>

struct Varying
{
    float light;
    float3 normale;
    float4 color;
    float u, v;
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

    mat4x4 model;
    mat4x4 view;
    mat4x4 projection;

    Texture texture;

    bool depthTest = true;
    bool wireframeMode = false;
};