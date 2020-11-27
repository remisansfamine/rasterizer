
#include <cstdio>
#include <cstring>
#include <cassert>

#include <imgui.h>

#include <common/maths.hpp>

#include "renderer_impl.hpp"

#include <iostream>

#define PIXELLIGHT 0

rdrImpl* rdrInit(float* colorBuffer32Bits, float* depthBuffer, int width, int height)
{
    rdrImpl* renderer = new rdrImpl();

    renderer->fb.colorBuffer = reinterpret_cast<float4*>(colorBuffer32Bits);
    renderer->fb.depthBuffer = depthBuffer;
    renderer->fb.width = width;
    renderer->fb.height = height;

    renderer->viewport = Viewport{ 0, 0, width, height };

    return renderer;
}

void rdrShutdown(rdrImpl* renderer)
{
    delete renderer;
}

void rdrSetUniformFloatV(rdrImpl* renderer, rdrUniformType type, float* value)
{
    switch (type)
    {
        case UT_TIME:      renderer->uniform.time = value[0]; break;
        case UT_DELTATIME: renderer->uniform.deltaTime = value[0]; break;
        default:;
    }
}

void rdrSetUniformLight(rdrImpl* renderer, int index, rdrLight* light)
{
    if (index < 0 || index >= IM_ARRAYSIZE(renderer->uniform.lights))
        return;

    memcpy(&renderer->uniform.lights[index], light, sizeof(rdrLight));
}


void rdrSetProjection(rdrImpl* renderer, float* projectionMatrix)
{
    memcpy(renderer->uniform.projection.e, projectionMatrix, 16 * sizeof(float));
}

void rdrSetView(rdrImpl* renderer, float* viewMatrix)
{
    memcpy(renderer->uniform.view.e, viewMatrix, 16 * sizeof(float));
}

void rdrSetModel(rdrImpl* renderer, float* modelMatrix)
{
    memcpy(renderer->uniform.model.e, modelMatrix, 16 * sizeof(float));
}

void rdrSetViewport(rdrImpl* renderer, int x, int y, int width, int height)
{
    renderer->viewport.x = x;
    renderer->viewport.y = y;
    renderer->viewport.width = width;
    renderer->viewport.height = height;
}

void rdrSetTexture(rdrImpl* renderer, float* colors32Bits, int width, int height)
{
    renderer->uniform.texture =
    {
        width,
        height,
        colors32Bits
    };
}

void drawPixel(float4* colorBuffer, int width, int height, int x, int y, const float4& color)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    colorBuffer[y * width + x] = color;
}

void drawLine(float4* colorBuffer, int width, int height, int x0, int y0, int x1, int y1, const float4& color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    for (;;) {
        drawPixel(colorBuffer, width, height, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

void drawLine(const Framebuffer& fb, const float3& p0, const float3& p1, const float4& color)
{
    drawLine(fb.colorBuffer, fb.width, fb.height, (int)roundf(p0.x), (int)roundf(p0.y), (int)roundf(p1.x), (int)roundf(p1.y), color);
}

bool isInside(const float4& clip)
{
    return (clip.x > -clip.w && clip.x < clip.w)
        && (clip.y > -clip.w && clip.y < clip.w)
        && (clip.z > -clip.w && clip.z < clip.w)
        && 0 < clip.w;
}

float3 ndcToScreenCoords(const float3& ndc, const Viewport& viewport)
{
    return
    {
        remap( ndc.x, -1.f, 1.f, viewport.x, viewport.width  * 0.5f),
        remap(-ndc.y, -1.f, 1.f, viewport.y, viewport.height * 0.5f),
        remap(-ndc.z, -1.f, 1.f, 0.f, 1.f)
    };
}

float getWeight(const float2& a, const float2& b, const float2& c)
{
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

float4 getLightColor(const Uniform& uniform)
{
    float4 lightColor = {0.f, 0.f, 0.f, 0.f};
    for (int i = 0; i < IM_ARRAYSIZE(uniform.lights); i++)
    {
        if (uniform.lights[i].isEnable)
            lightColor += uniform.lights[i].lightColor * uniform.lights[i].lightPower;
    }

    return lightColor;
}

float4 fragmentShader(const Varying& fragVars, const Uniform& uniform)
{
#if PIXELLIGHT
    if (!uniform.texture.data)
        return fragVars.light * fragVars.color * getLightColor(uniform);
#else
    if (!uniform.texture.data)
        return fragVars.light * fragVars.color;
#endif

    const Texture& texture = uniform.texture;

    int s = (int(texture.width  * fragVars.uv.u) % texture.width  + texture.width)  % texture.width;
    int t = (int(texture.height * fragVars.uv.v) % texture.height + texture.height) % texture.height;

    int index = ((texture.width * t) + s) * 4;
    float4 texColor =
    {
        texture.data[index + 0],
        texture.data[index + 1],
        texture.data[index + 2],
        texture.data[index + 3]
    };

#if PIXELLIGHT
    return (texColor * fragVars.color) * fragVars.light * getLightColor(uniform);
#else
    return (texColor * fragVars.color);
#endif

}

float interpolateFloat(const float3& value, const float3& weight)
{
    return value.e[0] * weight.e[0] + value.e[1] * weight.e[1] + value.e[2] * weight.e[2];
}

Varying interpolateVarying(const Varying* varyings, const float3& weight)
{
    Varying result;

    float* vr = (float*)&result;
    float* v0 = (float*)&varyings[0];
    float* v1 = (float*)&varyings[1];
    float* v2 = (float*)&varyings[2];

    for (int i = 0; i < sizeof(Varying) / sizeof(float); i++)
        vr[i] = interpolateFloat(float3(v0[i], v1[i], v2[i]), weight);

    return result;
}

void rasterTriangle(const Framebuffer& fb, const float4* screenCoords, const Varying* varying, const Uniform& uniform)
{
    // Get the bounding box
    int xMin = min(screenCoords[0].x, min(screenCoords[1].x, screenCoords[2].x));
    int yMin = min(screenCoords[0].y, min(screenCoords[1].y, screenCoords[2].y));
    int xMax = max(screenCoords[0].x, max(screenCoords[1].x, screenCoords[2].x));
    int yMax = max(screenCoords[0].y, max(screenCoords[1].y, screenCoords[2].y));

    float triangleArea = getWeight(screenCoords[0].xy, screenCoords[1].xy, screenCoords[2].xy);

    for (int i = xMin; i <= xMax; i++)
    {
        for (int j = yMin; j <= yMax; j++)
        {
            // Check if the pixel is in the triangle foreach segment
            float2 pixel(i + 0.5f, j + 0.5f);
            
            float weight0 = getWeight(screenCoords[1].xy, screenCoords[2].xy, pixel);
            if (weight0 < 0.f)
                continue;
            
            float weight1 = getWeight(screenCoords[2].xy, screenCoords[0].xy, pixel);
            if (weight1 < 0.f)
                continue;

            if (triangleArea - weight0 - weight1 < 0.f)
                continue;

            weight0 /= triangleArea;
            weight1 /= triangleArea;

            float weight2 = 1.f - weight0 - weight1;

            float3 weight(weight0, weight1, weight2);

            // Depth test / z-buffer
            float z = interpolateFloat(float3(screenCoords[0].z, screenCoords[1].z, screenCoords[2].z), weight);
            {
                float* zBuffer = &fb.depthBuffer[j * fb.width + i];
                if (*zBuffer >= z && uniform.depthTest)
                    continue;

                *zBuffer = z;
            }

            float3 wVector(screenCoords[0].w, screenCoords[1].w, screenCoords[2].w);

            float interpolatedW = 1.f / interpolateFloat(wVector, weight);
            float3 correctedWeight = wVector * weight * interpolatedW;

            Varying fragVarying = interpolateVarying(varying, correctedWeight);
            
            float4 color = fragmentShader(fragVarying, uniform);
            color.a = 1.f;
            drawPixel(fb.colorBuffer, fb.width, fb.height, i, j, color);
        }
    }
}

float4 vertexShader(const rdrVertex& vertex, const Uniform& uniform, Varying& varying)
{
    // Store triangle vertices positions
    float3 localCoords(vertex.x, vertex.y, vertex.z);

    varying.normal = (uniform.model * float4{ vertex.nx, vertex.ny, vertex.nz, 0.f }).xyz;

    varying.light = saturate(-dot(varying.normal, normalized(localCoords - uniform.lights[0].lightPos)));

#if !PIXELLIGHT
    varying.color = float4(vertex.r, vertex.g, vertex.b, vertex.a) * getLightColor(uniform);

    /*varying.color = float4(vertex.r, vertex.g, vertex.b, vertex.a);
    for (int i = 0; i < IM_ARRAYSIZE(uniform.lights); i++)
    {
        if (uniform.lights[i].isEnable)
            varying.color *= uniform.lights[i].lightColor *
                             uniform.lights[i].lightPower *
                             saturate(-dot(varying.normal, normalized(localCoords - uniform.lights[i].lightPos)));
    }*/
#else
    varying.color = float4(vertex.r, vertex.g, vertex.b, vertex.a);
#endif

    varying.uv = { vertex.u, vertex.v };

    return uniform.mvp * float4{ localCoords, 1.f };
}

void drawTriangle(rdrImpl* renderer, const Uniform& uniform, rdrVertex* vertices)
{
    Varying varying[3];

    // Local space (v3) -> Clip space (v4)
    float4 clipCoords[3]
    {
        vertexShader(vertices[0], uniform, varying[0]),
        vertexShader(vertices[1], uniform, varying[1]),
        vertexShader(vertices[2], uniform, varying[2]),
    };

    // TODO: Subdivide in others triangles
    if (!isInside(clipCoords[0]) || !isInside(clipCoords[1]) || !isInside(clipCoords[2]))
        return;

    // Clip space (v4) to NDC (v3)
    float3 ndcCoords[3] = {
        { clipCoords[0].xyz / clipCoords[0].w },
        { clipCoords[1].xyz / clipCoords[1].w },
        { clipCoords[2].xyz / clipCoords[2].w },
    };

    if (((ndcCoords[2] - ndcCoords[0]) ^ (ndcCoords[1] - ndcCoords[0])).z > 0 && renderer->uniform.backFaceCulling)
        return;

    // NDC (v3) to screen coords (v2)
    float4 screenCoords[3] = {
        { ndcToScreenCoords(ndcCoords[0], renderer->viewport), 1.f / clipCoords[0].w },
        { ndcToScreenCoords(ndcCoords[1], renderer->viewport), 1.f / clipCoords[1].w },
        { ndcToScreenCoords(ndcCoords[2], renderer->viewport), 1.f / clipCoords[2].w },
    };

    if (renderer->uniform.wireframeMode)
    {
        // Draw triangle wireframe
        drawLine(renderer->fb, screenCoords[0].xyz, screenCoords[1].xyz, renderer->lineColor);
        drawLine(renderer->fb, screenCoords[1].xyz, screenCoords[2].xyz, renderer->lineColor);
        drawLine(renderer->fb, screenCoords[2].xyz, screenCoords[0].xyz, renderer->lineColor);
    }
    else
        // Rasterize triangle
        rasterTriangle(renderer->fb, screenCoords, varying, uniform);
}

void rdrDrawTriangles(rdrImpl* renderer, rdrVertex* vertices, int count)
{
    renderer->uniform.mvp = renderer->uniform.projection * renderer->uniform.view * renderer->uniform.model;

    // Transform vertex list to triangles into colorBuffer
    for (int i = 0; i < count; i += 3)
        drawTriangle(renderer, renderer->uniform, &vertices[i]);
}

void rdrSetImGuiContext(rdrImpl* renderer, struct ImGuiContext* context)
{
    ImGui::SetCurrentContext(context);
}

void rdrShowImGuiControls(rdrImpl* renderer)
{
    ImGui::Checkbox("wireframe", &renderer->uniform.wireframeMode);
    ImGui::Checkbox("depthtest", &renderer->uniform.depthTest);
    ImGui::Checkbox("backFaceCulling", &renderer->uniform.backFaceCulling);
    ImGui::ColorEdit4("lineColor", renderer->lineColor.e);
}