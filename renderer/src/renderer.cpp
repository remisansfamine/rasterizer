
#include <cstdio>
#include <cstring>
#include <cassert>

#include <imgui.h>

#include <common/maths.hpp>

#include "renderer_impl.hpp"

#include <algorithm>

#include <cmath>

rdrImpl* rdrInit(float* colorBuffer32Bits, float* depthBuffer, int* stencilBuffer, int width, int height)
{
    rdrImpl* renderer = new rdrImpl();

    renderer->fb.colorBuffer = reinterpret_cast<float4*>(colorBuffer32Bits);
    renderer->fb.depthBuffer = depthBuffer;
    renderer->fb.stencilBuffer = stencilBuffer;
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
    case UT_CAMERAPOS: renderer->uniform.cameraPos = float3{ value[0], value[1], value[2] }; break;
    case UT_GLOBALAMBIENT:  renderer->uniform.globalAmbient = float4{ value[0], value[1], value[2], value[3] }; break;
    case UT_GLOBALCOLOR:    renderer->uniform.globalColor = float4{ value[0], value[1], value[2], value[3] }; break;
    default:;
    }
}

void rdrSetUniformBool(rdrImpl* renderer, rdrUniformType type, bool value)
{
    switch (type)
    {
    case UT_DEPTHTEST:      renderer->uniform.depthTest = value; break;
    case UT_STENCTILTEST:   renderer->uniform.stencilTest = value; break;
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
        "",
        width,
        height,
        (float4*)colors32Bits
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
        remap(ndc.x, -1.f, 1.f, viewport.x, viewport.width * 0.5f),
        remap(-ndc.y, -1.f, 1.f, viewport.y, viewport.height * 0.5f),
        remap(-ndc.z, -1.f, 1.f, 0.f, 1.f)
    };
}

float getWeight(const float2& a, const float2& b, const float2& c)
{
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

void getLightColor(const Uniform& uniform, Varying& varying)
{
    for (int i = 0; i < IM_ARRAYSIZE(uniform.lights); i++)
    {
        if (!uniform.lights[i].isEnable)
            continue;

        const Light& currLight = uniform.lights[i];

        float3 normal = normalized(varying.normal);
        float3 lightDir = (currLight.lightPos).xyz / (currLight.lightPos).w - currLight.lightPos.w * varying.coords;

        float  distance = magnitude(lightDir);

        lightDir /= distance;

        if (!currLight.lightPos.w)
            lightDir *= -1.f;

        float NdotL = dot(lightDir, normal);

        float attenuation = currLight.constantAttenuation +
            currLight.linearAttenuation * distance +
            currLight.quadraticAttenuation * distance * distance;

        // Ambient
        varying.ambientColor += currLight.ambient;

        // Diffuse
        varying.diffuseColor += std::max(0.f, NdotL) * currLight.diffuse / attenuation;

        // Specular
        float3 R = normalized(2.f * NdotL * normal - lightDir);
        float3 V = normalized(uniform.cameraPos - varying.coords);

        varying.specularColor += powf(std::max(0.f, dot(R, V)), uniform.shiness) * currLight.specular / attenuation;
    }

    varying.diffuseColor.a = varying.specularColor.a = 0.f;
}

float4 getTextureColor(const Varying& fragVars, const Uniform& uniform)
{
    if (!uniform.texture.data || uniform.texture.height <= 0 || uniform.texture.width <= 0)
        return { 1.f, 1.f, 1.f, 1.f };

    const Texture& texture = uniform.texture;

    // Get tex coords with UVs
    float s = (texture.width - 1.f)  * mod(fragVars.uv.u, 1.f);
    float t = (texture.height - 1.f) * mod(fragVars.uv.v, 1.f);

    int si = int(s), ti = int(t);

    int tindex = ti * texture.width;

    float4 texColor;
    if (uniform.textureFilter == FilterType::BILINEAR)
    {
        const float4 colors[4] =
        {
            texture.data[ti * texture.width + si],
            texture.data[ti * texture.width + si + 1],
            texture.data[(ti + 1) * texture.width + si],
            texture.data[(ti + 1) * texture.width + si + 1],
        };
        texColor = bilinear(s - si, t - ti, colors);
    }
    else
        texColor = texture.data[ti * texture.width + si];

    return texColor;
}

float4 fragmentShader(Varying& fragVars, const Uniform& uniform)
{
    if (!uniform.lighting)
        return getTextureColor(fragVars, uniform) * fragVars.color;

    if (uniform.lightPerPixel)
        getLightColor(uniform, fragVars);

    return getTextureColor(fragVars, uniform) * fragVars.color * (uniform.globalAmbient + fragVars.ambientColor + fragVars.diffuseColor) + fragVars.specularColor;
}

float interpolateFloat(const float3& value, const float3& weight)
{
    return dot(value, weight);
}

Varying interpolateVarying(const Varying varyings[3], const float3& weight)
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

void rasterTriangle(const Framebuffer& fb, const float4 screenCoords[3], const Varying varying[3], const Uniform& uniform)
{
    // Get the bounding box
    int xMin = std::min(screenCoords[0].x, std::min(screenCoords[1].x, screenCoords[2].x));
    int yMin = std::min(screenCoords[0].y, std::min(screenCoords[1].y, screenCoords[2].y));
    int xMax = std::max(screenCoords[0].x, std::max(screenCoords[1].x, screenCoords[2].x));
    int yMax = std::max(screenCoords[0].y, std::max(screenCoords[1].y, screenCoords[2].y));

    float area = getWeight(screenCoords[0].xy, screenCoords[1].xy, screenCoords[2].xy);

    if (area == 0.f)
        return;

    float inversedArea = 1.f / area;

    float2 fragment;

    for (int i = xMin; i <= xMax; i++)
    {
        fragment.x = i + 0.5f;
        for (int j = yMin; j <= yMax; j++)
        {
            fragment.y = j + 0.5f;

            // Check if the pixel is in the triangle foreach segment
            float weight0 = getWeight(screenCoords[1].xy, screenCoords[2].xy, fragment) * inversedArea;
            if (weight0 < 0.f)
                continue;

            float weight1 = getWeight(screenCoords[2].xy, screenCoords[0].xy, fragment) * inversedArea;
            if (weight1 < 0.f)
                continue;

            float weight2 = 1.f - weight0 - weight1;
            if (weight2 < 0.f)
                continue;

            float3 weight(weight0, weight1, weight2);

            // Perspective correction
            if (uniform.perspectiveCorrection)
            {
                float3 correctionFloats(screenCoords[0].w, screenCoords[1].w, screenCoords[2].w);
                weight *= correctionFloats / interpolateFloat(correctionFloats, weight);
            }

            int fbIndex = j * fb.width + i;

            float z, * zBuffer = nullptr;
            if (uniform.depthTest)
            {
                float3 zFloats(screenCoords[0].z, screenCoords[1].z, screenCoords[2].z);

                z = interpolateFloat(zFloats, weight);

                zBuffer = &fb.depthBuffer[fbIndex];

                if (*zBuffer >= z)
                    continue;
            }

            Varying fragVarying = interpolateVarying(varying, weight);

            float4 fragColor = fragmentShader(fragVarying, uniform);

            if (zBuffer && fragColor.a > 0.75f)
                *zBuffer = z;

            fb.colorBuffer[fbIndex] = fragColor * fragColor.a + fb.colorBuffer[fbIndex] * (1.f - fragColor.a);
        }
    }
}

float4 vertexShader(const rdrVertex& vertex, const Uniform& uniform, Varying& varying)
{
    // Store triangle vertices positions
    float4 localCoords(vertex.x, vertex.y, vertex.z, 1.f);

    varying.coords = (uniform.model * localCoords).xyz;

    varying.normal = (uniform.model * float4(vertex.nx, vertex.ny, vertex.nz, 0.f)).xyz;

    varying.color = float4(vertex.r, vertex.g, vertex.b, vertex.a) * uniform.globalColor;

    if (!uniform.lightPerPixel && uniform.lighting)
        getLightColor(uniform, varying);

    varying.uv = { vertex.u, vertex.v };

    return uniform.mvp * localCoords;
}

bool faceCulling(const float3 ndcCoords[3], FaceType type)
{
    switch (type)
    {
    case FaceType::BACK:
        return ((ndcCoords[2] - ndcCoords[0]) ^ (ndcCoords[1] - ndcCoords[0])).z > 0.f;

    case FaceType::FRONT:
        return ((ndcCoords[2] - ndcCoords[0]) ^ (ndcCoords[1] - ndcCoords[0])).z < 0.f;

    case FaceType::FRONT_AND_BACK:
        return true;

    case FaceType::NONE:
        return false;

    default: return false;
    }
}

void drawTriangle(rdrImpl* renderer, const Uniform& uniform, const rdrVertex vertices[3])
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

    float3 invertedW(1.f / clipCoords[0].w, 1.f / clipCoords[1].w, 1.f / clipCoords[2].w);

    // Clip space (v4) to NDC (v3)
    float3 ndcCoords[3] = {
        { clipCoords[0].xyz * invertedW.e[0] },
        { clipCoords[1].xyz * invertedW.e[1] },
        { clipCoords[2].xyz * invertedW.e[2] },
    };

    if (faceCulling(ndcCoords, renderer->uniform.faceToCull))
        return;

    // NDC (v3) to screen coords (v2)
    float4 screenCoords[3] = {
        { ndcToScreenCoords(ndcCoords[0], renderer->viewport), invertedW.e[0] },
        { ndcToScreenCoords(ndcCoords[1], renderer->viewport), invertedW.e[1] },
        { ndcToScreenCoords(ndcCoords[2], renderer->viewport), invertedW.e[2] },
    };

    // Rasterize triangle
    if (renderer->uniform.fillTriangle)
        rasterTriangle(renderer->fb, screenCoords, varying, uniform);

    // Draw triangle wireframe
    if (renderer->uniform.wireframeMode)
    {
        drawLine(renderer->fb, screenCoords[0].xyz, screenCoords[1].xyz, renderer->lineColor);
        drawLine(renderer->fb, screenCoords[1].xyz, screenCoords[2].xyz, renderer->lineColor);
        drawLine(renderer->fb, screenCoords[2].xyz, screenCoords[0].xyz, renderer->lineColor);
    }
}

void rdrDrawTriangles(rdrImpl* renderer, const rdrVertex* vertices, int count)
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
    ImGui::Checkbox("lighting", &renderer->uniform.lighting);
    ImGui::Checkbox("light per pixel", &renderer->uniform.lightPerPixel);

    const char* filterTypeStr[] = { "NEAREST", "BILINEAR" };
    int filterTypeIndex = (int)renderer->uniform.textureFilter;
    if (ImGui::Combo("Texture filter", &filterTypeIndex, filterTypeStr, IM_ARRAYSIZE(filterTypeStr)))
        renderer->uniform.textureFilter = FilterType(filterTypeIndex);

    const char* faceTypeStr[] = { "NONE", "BACK", "FRONT", "FRONT_AND_BACK" };
    int faceTypeIndex = (int)renderer->uniform.faceToCull;
    if (ImGui::Combo("Face to cull", &faceTypeIndex, faceTypeStr, IM_ARRAYSIZE(faceTypeStr)))
        renderer->uniform.faceToCull = FaceType(faceTypeIndex);

    ImGui::Checkbox("perspectiveCorrection", &renderer->uniform.perspectiveCorrection);
    ImGui::Checkbox("fillTriange", &renderer->uniform.fillTriangle);
    ImGui::ColorEdit4("lineColor", renderer->lineColor.e);

    ImGui::DragFloat("shiness", &renderer->uniform.shiness, 0.f);
}