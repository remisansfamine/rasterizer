#pragma once

#include <rdr/renderer.h>

#include <common/types.hpp>

enum class FaceType
{
    NONE,
    BACK,
    FRONT,
    FRONT_AND_BACK 
};

enum class FilterType
{
    NEAREST,
    BILINEAR
};

struct Uniform
{
    float time;
    float deltaTime;

    float4 globalColor = { 1.f, 1.f, 1.f, 1.f };
    float4 globalAmbient = {0.2f, 0.2f, 0.2f, 1.f};
    float shiness = 80.f;

    Light lights[8];

    Texture texture;

    float3 cameraPos;
    mat4x4 mvp;
    mat4x4 model;
    mat4x4 view;
    mat4x4 projection;

    bool depthTest = true;
    bool stencilTest = false;
    bool wireframeMode = false;
    FaceType faceToCull = FaceType::BACK;
    FilterType textureFilter = FilterType::NEAREST;
    bool lighting = true;
    bool lightPerPixel = false;
    bool perspectiveCorrection = true;
    bool wMode = true;
    bool fillTriangle = true;
};

struct Varying
{
    float3 coords;
    float3 normal;
    float4 color;
    float2 uv;

    float4 ambientColor  =  { 0.f, 0.f, 0.f, 0.f };
    float4 diffuseColor  = { 0.f, 0.f, 0.f, 0.f };
    float4 specularColor = { 0.f, 0.f, 0.f, 0.f };
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
    int* stencilBuffer;
};

struct rdrImpl
{
    Framebuffer fb;
    Viewport viewport;

    float4 lineColor = { 1.f, 1.f, 1.f, 1.f };

    Uniform uniform;
};