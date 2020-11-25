
#include <cstdio>
#include <cstring>
#include <cassert>

#include <imgui.h>

#include <common/maths.hpp>

#include "renderer_impl.hpp"

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

void rdrSetProjection(rdrImpl* renderer, float* projectionMatrix)
{
    memcpy(renderer->projection.e, projectionMatrix, 16 * sizeof(float));
}

void rdrSetView(rdrImpl* renderer, float* viewMatrix)
{
    memcpy(renderer->view.e, viewMatrix, 16 * sizeof(float));
}

void rdrSetModel(rdrImpl* renderer, float* modelMatrix)
{
    memcpy(renderer->model.e, modelMatrix, 16 * sizeof(float));
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
    renderer->texture.width = width;
    renderer->texture.height = height;

    renderer->texture.data = colors32Bits;
}

void drawPixel(float4* colorBuffer, int width, int height, int x, int y, float4 color)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    colorBuffer[y * width + x] = color;
}

void drawLine(float4* colorBuffer, int width, int height, int x0, int y0, int x1, int y1, float4 color)
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

void drawLine(const Framebuffer& fb, float3 p0, float3 p1, float4 color)
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

float remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    return (value - oldMin) * (newMax - newMin) / (newMin - oldMin) + newMin;
}

float3 ndcToScreenCoords(const float3& ndc, const Viewport& viewport)
{
    return
    {
        (ndc.x + 1.f) * viewport.width / 2 + viewport.x,
        (1.f - ndc.y) * viewport.height * 0.5f + viewport.y,
        remap(-ndc.z, -1.f, 1.f, 0.f, 1.f)
    };
}

float getWeight(const float3& a, const float3& b, const float3& c)
{
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

void rasterTriangle(rdrImpl* renderer, const Framebuffer& fb, float3* screenCoords, const rdrVertex* vertices)
{
    int xMin = screenCoords[0].x, yMin = screenCoords[0].y, xMax = screenCoords[0].x, yMax = screenCoords[0].y;

    for (int i = 1; i < 3; i++)
    {
        xMin = min(xMin, screenCoords[i].x);
        yMin = min(yMin, screenCoords[i].y);
        xMax = max(xMax, screenCoords[i].x);
        yMax = max(yMax, screenCoords[i].y);
    }

    float triangleArea = getWeight(screenCoords[0], screenCoords[1], screenCoords[2]);

    for (int i = xMin; i <= xMax; i++)
    {
        for (int j = yMin; j <= yMax; j++)
        {
            float3 pixel = { i + 0.5f, j + 0.5f, 0.f };

            float weight0 = getWeight(screenCoords[1], screenCoords[2], pixel);
            if (weight0 < 0.f)
                continue;
            
            float weight1 = getWeight(screenCoords[2], screenCoords[0], pixel);
            if (weight1 < 0.f)
                continue;

            float weight2 = triangleArea - weight0 - weight1;
            if (weight2 < 0.f)
                continue;

            weight0 /= triangleArea;
            weight1 /= triangleArea;
            weight2 = 1.f - weight0 - weight1;

            float z = weight0 * screenCoords[0].z + screenCoords[1].z * weight1 + screenCoords[2].z * weight2;

            if (fb.depthBuffer[j * fb.width + i] >= z && renderer->depthTest)
                continue;

            fb.depthBuffer[j * fb.width + i] = z;

            float u = weight0 * vertices[0].u + vertices[1].u * weight1 + vertices[2].u * weight2;
            float v = weight0 * vertices[0].v + vertices[1].v * weight1 + vertices[2].v * weight2;

            int x = (renderer->texture.width ) * u;
            int y = (renderer->texture.height) * v;

            x %= renderer->texture.width;
            y %= renderer->texture.height;

            if (x < 0)
                x += renderer->texture.width;

            if (y < 0)
                y += renderer->texture.height;

            int index = (((renderer->texture.width) * y) + x) * 4;
            float4 color =
            {
                renderer->texture.data[index + 0],
                renderer->texture.data[index + 1],
                renderer->texture.data[index + 2],
                renderer->texture.data[index + 3]
            };

            drawPixel(fb.colorBuffer, fb.width, fb.height, i, j, color);
        }
    }
}

void drawTriangle(rdrImpl* renderer, const mat4x4& mvp, rdrVertex* vertices)
{
    // Store triangle vertices positions
    float3 localCoords[3] = {
        { vertices[0].x, vertices[0].y, vertices[0].z },
        { vertices[1].x, vertices[1].y, vertices[1].z },
        { vertices[2].x, vertices[2].y, vertices[2].z },
    };

    // Local space (v3) -> Clip space (v4)
    float4 clipCoords[3] = {
        { mvp * float4{ localCoords[0], 1.f } },
        { mvp * float4{ localCoords[1], 1.f } },
        { mvp * float4{ localCoords[2], 1.f } },
    };

    // TODO: Subdivide in others triangles
    if (!isInside(clipCoords[0]) || !isInside(clipCoords[1]) || !isInside(clipCoords[2]))
        return;

    // Clip space (v4) to NDC (v3)
    // TODO
    float3 ndcCoords[3] = {
        { clipCoords[0].xyz / clipCoords[0].w },
        { clipCoords[1].xyz / clipCoords[1].w },
        { clipCoords[2].xyz / clipCoords[2].w },
    };

    // NDC (v3) to screen coords (v2)
    // TODO
    float3 screenCoords[3] = {
        { ndcToScreenCoords(ndcCoords[0], renderer->viewport) },
        { ndcToScreenCoords(ndcCoords[1], renderer->viewport) },
        { ndcToScreenCoords(ndcCoords[2], renderer->viewport) },
    };

    if (renderer->wireframeMode)
    {
        // Draw triangle wireframe
        drawLine(renderer->fb, screenCoords[0], screenCoords[1], renderer->lineColor);
        drawLine(renderer->fb, screenCoords[1], screenCoords[2], renderer->lineColor);
        drawLine(renderer->fb, screenCoords[2], screenCoords[0], renderer->lineColor);
    }
    else
        // Rasterize triangle
        rasterTriangle(renderer, renderer->fb, screenCoords, vertices);
}

void rdrDrawTriangles(rdrImpl* renderer, rdrVertex* vertices, int count)
{
    mat4x4 mvp = renderer->projection * renderer->view * renderer->model;

    // Transform vertex list to triangles into colorBuffer
    for (int i = 0; i < count; i += 3)
    {
        drawTriangle(renderer, mvp, &vertices[i]);
    }
}

void rdrSetImGuiContext(rdrImpl* renderer, struct ImGuiContext* context)
{
    ImGui::SetCurrentContext(context);
}

void rdrShowImGuiControls(rdrImpl* renderer)
{
    ImGui::Checkbox("wireframe", &renderer->wireframeMode);
    ImGui::ColorEdit4("lineColor", renderer->lineColor.e);
}