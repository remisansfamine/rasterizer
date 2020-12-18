#pragma once

#include <rdr/renderer.h>

#include <common/types.hpp>

enum class FaceOrientation
{
    CW,
    CCW
};

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

struct rdrTexture
{
    int width = 0, height = 0;
    float4* data = nullptr;
};

struct Light
{
    bool    isEnable = false;

    float4  lightPos = { 0.0f, 0.0f, 0.0f, 1.f };
    float4  ambient  = { 0.0f, 0.0f, 0.0f, 1.f };
    float4  diffuse  = { 1.0f, 1.0f, 1.0f, 1.f };
    float4  specular = { 1.0f, 1.0f, 1.0f, 1.f };

    float   constantAttenuation  = 1.f;
    float   linearAttenuation    = 0.f;
    float   quadraticAttenuation = 0.f;
};

struct Material
{
    float4 ambientColor  = { 0.2f, 0.2f, 0.2f, 1.0f };
    float4 diffuseColor  = { 0.8f, 0.8f, 0.8f, 1.0f };
    float4 specularColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    float4 emissionColor = { 0.0f, 0.0f, 0.0f, 0.0f };

    float shininess = 20.f;
};

struct Uniform
{
    float time;
    float deltaTime;

    float4 globalColor   = { 1.f, 1.f, 1.f, 1.f };
    float4 globalAmbient = {0.2f, 0.2f, 0.2f, 1.f};
    float shiness = 80.f;

    Light lights[8];

    rdrTexture texture;
    Material material;

    float3 cameraPos;

    mat4x4 viewProj;
    mat4x4 model;
    mat4x4 view;
    mat4x4 projection;

    bool msaa = true;

    bool depthTest = true;

    bool blending = true;
    float cutout = 0.5f;

    FaceOrientation faceOrientation = FaceOrientation::CW;
    FaceType faceToCull = FaceType::BACK;

    FilterType textureFilter = FilterType::NEAREST;

    bool lighting = true;
    bool phongModel = false;
    bool perspectiveCorrection = true;
};

struct Varying
{
    float3 coords;
    float3 normal;
    float4 color;
    float2 uv;

    float4 shadedColor = { 0.f, 0.f, 0.f, 0.f };
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
    float*  depthBuffer;
    float4* msaaColorBuffer;
    float*  msaaDepthBuffer;
};

struct rdrImpl
{
    Framebuffer fb;
    Viewport viewport;

    float4 lineColor = { 1.f, 1.f, 1.f, 1.f };

    bool fillTriangle = true;
    bool wireframeMode = false;
    bool boxBlur = false;
    bool gaussianBlur = false;
    bool lightBloom = false;

    float gamma = 2.2f;
    float iGamma = 1.f / 2.2f;

    Uniform uniform;
};