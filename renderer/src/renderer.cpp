
#include <cstdio>
#include <cstring>
#include <cassert>

#include <imgui.h>

#include <common/maths.hpp>

#include "renderer_impl.hpp"

#include <algorithm>

#define NB_SAMPLES 4

struct clipPoint
{
    float4 coords;
    float3 weights = { 0.f, 0.f, 0.f };
};

rdrImpl* rdrInit(float* colorBuffer32Bits, float* depthBuffer, int width, int height)
{
    rdrImpl* renderer = new rdrImpl();

    renderer->fb.colorBuffer = reinterpret_cast<float4*>(colorBuffer32Bits);
    renderer->fb.depthBuffer = depthBuffer;
    renderer->fb.msaaColorBuffer = new float4[width * height * NB_SAMPLES]();
    renderer->fb.msaaDepthBuffer = new float[width * height * NB_SAMPLES]();
    renderer->fb.width = width;
    renderer->fb.height = height;

    renderer->viewport = Viewport{ 0, 0, width, height };

    return renderer;
}

float4 gammaCorrection(const float4& color, float iGamma)
{
    // Return color gamma corrected
    return
    {
        powf(color.r, iGamma),
        powf(color.g, iGamma),
        powf(color.b, iGamma),
        1.f
    };
}

float4 boxBlur(Framebuffer fb, int index)
{
    // Return the 'normalized' sum of a 3x3 pixels grid
    float4 sum =
        fb.colorBuffer[index - 1 + fb.width] + // Top left
        fb.colorBuffer[index     + fb.width] + // Top center
        fb.colorBuffer[index + 1 + fb.width] + // Top right
        fb.colorBuffer[index - 1           ] + // Mid left
        fb.colorBuffer[index               ] + // Current pixel
        fb.colorBuffer[index + 1           ] + // Mid right
        fb.colorBuffer[index - 1 - fb.width] + // Low left
        fb.colorBuffer[index     - fb.width] + // Low center
        fb.colorBuffer[index + 1 - fb.width];  // Low right

    return sum / 9.f;
}

float4 gaussianBlur(Framebuffer fb, int index)
{
    // Return the 'normalized' sum of a 3x3 pixels grid applied with some coefficients
    float4 sum =
              fb.colorBuffer[index - 1 + fb.width] + // Low left
        2.f * fb.colorBuffer[index     + fb.width] + // Low center
              fb.colorBuffer[index + 1 + fb.width] + // Low right
        2.f * fb.colorBuffer[index - 1           ] + // Mid left
        4.f * fb.colorBuffer[index               ] + // Current pixel
        2.f * fb.colorBuffer[index + 1           ] + // Mid right
              fb.colorBuffer[index - 1 - fb.width] + // Top left
        2.f * fb.colorBuffer[index     - fb.width] + // Top center
              fb.colorBuffer[index + 1 - fb.width];  // Top right

    return sum / 16.f;
}

void rdrFinish(rdrImpl* renderer)
{ 
    float4* color = renderer->fb.colorBuffer;
    float4* msaaColor = renderer->fb.msaaColorBuffer;

    #pragma region Box blur, gaussian blur and light bloom post-process effects
    int offset = 1;

    for (int i = offset; i < renderer->fb.width - offset; i++)
    {
        for (int j = offset; j < renderer->fb.height - offset; j++)
        {
            int index = i + renderer->fb.width * j;

            float4& currentColor = color[index];

            if (renderer->boxBlur)
                currentColor = boxBlur(renderer->fb, index);

            // Gaussian blur and light bloom
            if (renderer->gaussianBlur || currentColor.a > 2.5f && renderer->lightBloom)
                currentColor = gaussianBlur(renderer->fb, index);
        }
    }
    #pragma endregion

    #pragma region Pixel per pixel post-process
    for (int i = 0; i < renderer->fb.width; i++)
    {
        for (int j = 0; j < renderer->fb.height; j++)
        {
            int index = renderer->fb.width * j + i;

            float4& currentColor = color[index];

            if (renderer->uniform.msaa)
            {
                float4 sum = { 0.f, 0.f, 0.f, 0.f };

                int msaaIndex = index * NB_SAMPLES;

                for (int k = 0; k < NB_SAMPLES; k++)
                    sum += renderer->fb.msaaColorBuffer[msaaIndex + k];

                currentColor = sum / NB_SAMPLES;
            }

            currentColor = gammaCorrection(currentColor, renderer->iGamma);
        }
        
    }
    #pragma endregion

    float4 clearColor = { 0.f, 0.f, 0.f, 1.f };

    for (size_t i = 0; i < renderer->fb.width * renderer->fb.height * 4; ++i)
        memcpy(&msaaColor[i], &clearColor, sizeof(float4));
}

void rdrShutdown(rdrImpl* renderer)
{
    delete[] renderer->fb.msaaColorBuffer;
    delete[] renderer->fb.msaaDepthBuffer;
    delete renderer;
}

void rdrSetUniformFloatV(rdrImpl* renderer, rdrUniformType type, float* value)
{
    // Set uniform float in function of the input type
    switch (type)
    {
        case UT_TIME:           renderer->uniform.time = value[0]; break;
        case UT_DELTATIME:      renderer->uniform.deltaTime = value[0]; break;
        case UT_CAMERA_POS:     renderer->uniform.cameraPos = float3{ value[0], value[1], value[2] }; break;
        case UT_GLOBALAMBIENT:  renderer->uniform.globalAmbient = float4{ value[0], value[1], value[2], value[3] }; break;
        case UT_GLOBALCOLOR:    renderer->uniform.globalColor = float4{ value[0], value[1], value[2], value[3] }; break;
        default:;
    }
}

void rdrSetUniformBool(rdrImpl* renderer, rdrUniformType type, bool value)
{
    // Set uniform bool in function of the input type
    switch (type)
    {
        case UT_DEPTH_TEST:      renderer->uniform.depthTest = value; break;
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
        (float4*)colors32Bits
    };
}

void rdrSetUniformMaterial(rdrImpl* renderer, rdrMaterial* material)
{
    memcpy(&renderer->uniform.material, material, sizeof(rdrMaterial));
}

void drawPixel(float4* colorBuffer, int width, int height, int x, int y, const float4& color)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    colorBuffer[y * width + x] = color;
}

void drawLine(const Framebuffer& fb, int x0, int y0, int x1, int y1, const float4& color, bool MSAA = false)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    for (;;) {
        if (x0 >= 0 && x0 < fb.width && y0 >= 0 && y0 < fb.height)
        {
            int index = y0 * fb.width + x0;

            if (MSAA)
            {
                for (int k = 0; k < NB_SAMPLES; k++)
                    fb.msaaColorBuffer[index * NB_SAMPLES + k] = color;
            }
            else
                fb.colorBuffer[index] = color;
        }

        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

void drawLine(const Framebuffer& fb, const float3& p0, const float3& p1, const float4& color, bool MSAA = false)
{
    drawLine(fb, (int)roundf(p0.x), (int)roundf(p0.y), (int)roundf(p1.x), (int)roundf(p1.y), color, MSAA);
}

float3 ndcToScreenCoords(const float3& ndc, const Viewport& viewport)
{
    // Remap x and y to screen coords, and z to { 0 - 1 }
    return
    {
        remap( ndc.x, -1.f, 1.f, viewport.x, viewport.width ),
        remap(-ndc.y, -1.f, 1.f, viewport.y, viewport.height),
        remap(-ndc.z, -1.f, 1.f, 0.f, 1.f)
    };
}

void getLightColor(const Uniform& uniform, Varying& varying)
{
    float4 ambientColorSum = { 0.f, 0.f, 0.f, 0.f };
    float4 diffuseColorSum = { 0.f, 0.f, 0.f, 0.f };

    float3 normal = normalized(varying.normal);

    for (int i = 0; i < IM_ARRAYSIZE(uniform.lights); i++)
    {
        if (!uniform.lights[i].isEnable)
            continue;

        const Light& currLight = uniform.lights[i];

        float3 lightDir = currLight.lightPos.xyz / currLight.lightPos.w - currLight.lightPos.w * varying.coords;

        float  distance = magnitude(lightDir);

        lightDir /= distance;

        float attenuation;
        if (currLight.lightPos.w == 0.f)
        {
            lightDir *= -1.f;
            attenuation = 1.f;
        }
        else
        {
            attenuation = currLight.constantAttenuation +
                          currLight.linearAttenuation * distance +
                          currLight.quadraticAttenuation * distance * distance;
        }

        float NdotL = dot(lightDir, normal);

        // Ambient
        ambientColorSum += currLight.ambient / attenuation;

        // Diffuse
        diffuseColorSum += max(0.f, NdotL) * currLight.diffuse / attenuation;

        // Specular
        float3 R = normalized(2.f * NdotL * normal - lightDir);
        float3 V = normalized(uniform.cameraPos - varying.coords);

        varying.specularColor += powf(max(0.f, dot(R, V)), uniform.material.shininess) * currLight.specular / attenuation;
    }

    varying.shadedColor = uniform.material.ambientColor * (uniform.globalAmbient + ambientColorSum) +
                          uniform.material.diffuseColor * diffuseColorSum +
                          uniform.material.emissionColor;

    varying.specularColor *= uniform.material.specularColor;
}

float4 getTextureColor(const Varying& fragVars, const Uniform& uniform)
{
    // If there is no texture return a white color
    if (!uniform.texture.data || uniform.texture.height <= 0 || uniform.texture.width <= 0)
        return { 1.f, 1.f, 1.f, 1.f };

    const rdrTexture& texture = uniform.texture;

    float u = wrap01(fragVars.uv.u);
    float v = wrap01(fragVars.uv.v);

    // Get tex coords with UVs
    float s = texture.width  * u;
    float t = texture.height * v;

    // Get texel color with tex coords
    float4 texColor;
    if (uniform.textureFilter == FilterType::BILINEAR)
    {
        s -= u;
        t -= v;

        int si = int(s), ti = int(t);

        int index = ti * texture.width + si;

        // Get nearest texels to interpolate their colors
        const float4 colors[4] =
        {
            texture.data[index                    ],  // Top-left
            texture.data[index                 + 1],  // Top-right
            texture.data[index + texture.width    ],  // Bottom-left
            texture.data[index + texture.width + 1],  // Bottom-right
        };

        texColor = bilinear(s - si, t - ti, colors);
    }
    else
        // Get the nearest texel
        texColor = texture.data[int(t) * texture.width + int(s)];

    return texColor;
}

bool fragmentShader(Varying& fragVars, const Uniform& uniform, float4& outColor)
{
    // If there is no lighting, return the color with no more modification
    if (!uniform.lighting)
    {
        outColor = getTextureColor(fragVars, uniform) * fragVars.color;
        return true;
    }

    // If the phong model is used, compute the shaded color and the specular for each pixel
    if (uniform.phongModel)
        getLightColor(uniform, fragVars);

    outColor = getTextureColor(fragVars, uniform) * fragVars.color * fragVars.shadedColor +
               fragVars.specularColor;

    return true;
}

float interpolateFloat(const float3& value, const float3& weight)
{
    // Interpolate the input values with their weights
    return dot(value, weight);
}

Varying interpolateVarying(const Varying varyings[3], const float3& weight)
{
    Varying result;

    float* vr = (float*)&result;
    float* v0 = (float*)&varyings[0];
    float* v1 = (float*)&varyings[1];
    float* v2 = (float*)&varyings[2];

    // Interpolate each float in the varying
    for (int i = 0; i < sizeof(Varying) / sizeof(float); i++)
        vr[i] = interpolateFloat(float3(v0[i], v1[i], v2[i]), weight);

    return result;
}

bool getBarycentric(const float4 screenCoords[3], const float2& pixelCoords, float inversedArea, float3& inWeights)
{
    // Check if the pixel is in the triangle foreach segment
    inWeights.x = getWeight(screenCoords[1].xy, screenCoords[2].xy, pixelCoords) * inversedArea;
    if (inWeights.x < 0.f)
        return false;

    inWeights.y = getWeight(screenCoords[2].xy, screenCoords[0].xy, pixelCoords) * inversedArea;
    if (inWeights.y < 0.f)
        return false;

    inWeights.z = 1.f - inWeights.x - inWeights.y;
    if (inWeights.z < 0.f)
        return false;

    return true;
}

bool alphaTest(const Uniform& uniform, float alpha)
{
    return alpha >= uniform.cutout;
}

void perspectiveCorrection(const float3 correctionFloats, float3& weight)
{
    // Get the new weight with perspective correction
    weight *= correctionFloats / interpolateFloat(correctionFloats, weight);
}

void rasterTriangle(const Framebuffer& fb, const float4 screenCoords[3], const Varying varying[3], const Uniform& uniform)
{
    // Get the bounding box
    int xMin = min(screenCoords[0].x, min(screenCoords[1].x, screenCoords[2].x));
    int yMin = min(screenCoords[0].y, min(screenCoords[1].y, screenCoords[2].y));
    int xMax = max(screenCoords[0].x, max(screenCoords[1].x, screenCoords[2].x));
    int yMax = max(screenCoords[0].y, max(screenCoords[1].y, screenCoords[2].y));

    float area = getWeight(screenCoords[0].xy, screenCoords[1].xy, screenCoords[2].xy);

    // TODO: Fix area = 0 and black lines
    if (area == 0.f)
        return;

    float inversedArea = 1.f / area;
    
    float2 fragment;
    float3 weight;

    // Foreach pixel in the bounding box
    for (int i = xMin; i <= xMax; i++)
    {
        fragment.x = i + 0.5f;
        for (int j = yMin; j <= yMax; j++)
        {
            fragment.y = j + 0.5f;

            // Check if it is in the triangle
            if (!getBarycentric(screenCoords, fragment, inversedArea, weight))
                continue;
            /*{
                float2 edge0 = screenCoords[2].xy - screenCoords[1].xy;
                float2 edge1 = screenCoords[0].xy - screenCoords[2].xy;
                float2 edge2 = screenCoords[1].xy - screenCoords[0].xy;

                bool overlaps = true;

                // If the point is on the edge, test if it is a top or left edge, 
                // otherwise test if  the edge function is positive
                overlaps &= (weight.x == 0 ? ((edge0.y == 0 && edge0.x > 0) || edge0.y > 0) : (weight.x > 0));
                overlaps &= (weight.y == 0 ? ((edge1.y == 0 && edge1.x > 0) || edge1.y > 0) : (weight.y > 0));
                overlaps &= (weight.z == 0 ? ((edge2.y == 0 && edge2.x > 0) || edge2.y > 0) : (weight.z > 0));

                if (!overlaps)
                    continue;
            }*/

            int fbIndex = j * fb.width + i;

            // Depth test
            float z, *zBuffer = nullptr;
            if (uniform.depthTest)
            {
                z = interpolateFloat({ screenCoords[0].z, screenCoords[1].z, screenCoords[2].z }, weight);

                zBuffer = &fb.depthBuffer[fbIndex];

                // If there is a closer pixel drawn at the same screen coords, discard
                if (*zBuffer >= z)
                    continue;
            }

            // Perspective correction
            if (uniform.perspectiveCorrection)
                perspectiveCorrection({ screenCoords[0].w, screenCoords[1].w, screenCoords[2].w }, weight);

            // Get the varying of the current pixel
            Varying fragVarying = interpolateVarying(varying, weight);

            float4 fragColor;
            if (!fragmentShader(fragVarying, uniform, fragColor))
                continue;

            // If the cutout permit it, write in the depthBuffer
            if (zBuffer && alphaTest(uniform, fragColor.a))
                *zBuffer = z;

            // If there is blending, get the last pixel in the colorBuffer and add it to the fragment color
            if (uniform.blending && fragColor.a < 1.f)
                fragColor = fragColor * max(fragColor.a, 0.f) + fb.colorBuffer[fbIndex] * (1.f - min(fragColor.a, 1.f));

            if (uniform.msaa)
            {
                int msaaIndex = fbIndex * NB_SAMPLES;

                float3 sampleWeight;
                /*float2 sampleOffset = { 0.25f, 0.25f };
                for (int k = 0; k < NB_SAMPLES; k++)
                {
                    sampleOffset.x *= -1.f;
                    sampleOffset.y = sign(k - NB_SAMPLES / 2);

                    if (!getBarycentric(screenCoords, fragment + sampleOffset, inversedArea, sampleWeight))
                        continue;

                    fb.msaaColorBuffer[msaaIndex + k] = fragColor;
                }*/
                if (getBarycentric(screenCoords, fragment + float2{-0.25f,-0.25f }, inversedArea, sampleWeight))
                    fb.msaaColorBuffer[msaaIndex + 0] = fragColor;

                if (getBarycentric(screenCoords, fragment + float2{ 0.25f,-0.25f }, inversedArea, sampleWeight))
                    fb.msaaColorBuffer[msaaIndex + 1] = fragColor;

                if (getBarycentric(screenCoords, fragment + float2{-0.25f, 0.25f }, inversedArea, sampleWeight))
                    fb.msaaColorBuffer[msaaIndex + 2] = fragColor;

                if (getBarycentric(screenCoords, fragment + float2{ 0.25f, 0.25f }, inversedArea, sampleWeight))
                    fb.msaaColorBuffer[msaaIndex + 3] = fragColor;
            }
            else 
                fb.colorBuffer[fbIndex] = fragColor;
        }
    }
}

float4 vertexShader(const rdrVertex& vertex, const Uniform& uniform, Varying& varying)
{
    // Store triangle vertices positions
    float4 localCoords = uniform.model * float4(vertex.x, vertex.y, vertex.z, 1.f);

    // Get the world coords and world normals and stock it in the current varying
    varying.coords = localCoords.xyz / localCoords.w;
    varying.normal = (uniform.model * float4(vertex.nx, vertex.ny, vertex.nz, 0.f)).xyz;

    // Get the vertex color multiplied with the global color
    varying.color = float4(vertex.r, vertex.g, vertex.b, vertex.a) * uniform.globalColor;

    // If the current light model is the Gouraud model, compute the shaded color and the specular here (foreach vertex)
    if (!uniform.phongModel && uniform.lighting)
        getLightColor(uniform, varying);

    varying.uv = { vertex.u, vertex.v };

    // Return the clip coords
    return uniform.viewProj * localCoords;
}

bool faceCulling(const float3 ndcCoords[3], FaceOrientation orientation, FaceType toCull)
{
    #pragma region Get face orientation
    // Set the front face of the polygon with the orientation input
    int index1 = 1, index2 = 2;
    if (orientation == FaceOrientation::CCW)
    {
        index1 = 2;
        index2 = 1;
    }
    #pragma endregion

    // Calculate the z component face's normal
    float normalZ = ((ndcCoords[index2] - ndcCoords[0]) ^ (ndcCoords[index1] - ndcCoords[0])).z;

    #pragma region Cull face
    // Check for each case if the current face should be drawn
    switch (toCull)
    {
        case FaceType::BACK:            return normalZ > 0.f;

        case FaceType::FRONT:           return normalZ < 0.f;

        case FaceType::FRONT_AND_BACK:  return normalZ != 0.f;

        case FaceType::NONE:            return normalZ == 0.f;

        default: return false;
    }
    #pragma endregion
}

unsigned char computeClipOutcodes(const float4 clipCoords)
{
    unsigned char code = 0;

    for (int i = 0; i < 8; i++)
    {
        // Check for each coordinate if it is outside the plane, if it is -> change the outcode
        if (sign(i - 3) * clipCoords.e[i % 4] <= -clipCoords.w)
            code |= 1 << i;
    }

    return code;
}

int clipTriangle(clipPoint outputCoords[9], unsigned char outputCodes)
{
    // Fast exit if all points are in the screen
    if (!outputCodes)
        return 3;

    int finalPointCount = 3;

    //Clip against each plane
    for (int i = 0, plane = 1; i < 8; i++, plane <<= 1)
    {
        // If there is no point outside this plane, continue
        if (!(outputCodes & plane))
            continue;

        int currentVertCount = 0;

        clipPoint currentVertices[9];
        const clipPoint* currentVertex  = &outputCoords[0];
        const clipPoint* previousVertex = &outputCoords[finalPointCount - 1];

        // Get axis index and axis sign (-1, 0, 1)
        int axis = i < 4 ? i : i - 4;
        int axisSign = sign(i - 3);

        // Compute previous clipcode for the first point and the current axis value
        unsigned char   prevCode  = computeClipOutcodes(previousVertex->coords) & plane;
        float           prevValue = previousVertex->coords.w + axisSign * previousVertex->coords.e[axis];

        while (currentVertex != &outputCoords[finalPointCount])
        {
            // Compute current clipcode and the current axis value
            unsigned char   currCode  = computeClipOutcodes(currentVertex->coords) & plane;
            float           currValue = currentVertex->coords.w + axisSign * currentVertex->coords.e[axis];

            // Check if only one point is outside the plane
            if (currCode ^ prevCode)
            {
                // Get intersection factor with the current axes
                float lerpFactor = prevValue / (prevValue - currValue);

                // Insert intersection vertex at the end of the array
                currentVertices[currentVertCount++] =
                {
                    // Lerp the values
                    lerp(previousVertex->coords,  currentVertex->coords,  lerpFactor),
                    lerp(previousVertex->weights, currentVertex->weights, lerpFactor)
                };
            }

            //Insert current vertex at the end of the array if it is inside the plane 
            if (!currCode)
                currentVertices[currentVertCount++] = *currentVertex;

            // Set the previous vertex values with the current ones
            prevCode  = currCode;
            prevValue = currValue;

            //Move forward (set previous vertex and get next vertex)
            previousVertex = currentVertex++;
        }

        // Change the output coords and the count of vertex
        memcpy(outputCoords, currentVertices, sizeof(clipPoint) * currentVertCount);
        finalPointCount = currentVertCount;
    }

    return finalPointCount;
}

void drawTriangle(rdrImpl* renderer, const rdrVertex vertices[3])
{
    #pragma region Varyings, clip coords, outputPoints and outputCodes
    Varying         varying[3];
    float4          clipCoords[3];
    clipPoint       outputPoints[9];
    unsigned char   outputCodes[3];

    for (int i = 0; i < 3; i++)
    {
        // Local space (v3) -> Clip space (v4) (apply vertex shader, set the current varying values)
        clipCoords[i] = vertexShader(vertices[i], renderer->uniform, varying[i]);

        // Link clip coords and his weight
        outputPoints[i] = { clipCoords[i] };
        outputPoints[i].weights.e[i] = 1.f;

        // Compute clip codes foreach initial vertex
        outputCodes[i] = computeClipOutcodes(clipCoords[i]);
    }
    #pragma endregion

    #pragma region New outputPoints
    // Exit if all the vertices are outside the screen
    if (outputCodes[0] & outputCodes[1] & outputCodes[2])
        return;

    // Clip the triangle and get the new vertex count
    int pointCount = clipTriangle(outputPoints, outputCodes[0] | outputCodes[1] | outputCodes[2]);

    if (pointCount < 3) // Exit if there is not enough vertice in the screen
        return;
    #pragma endregion

    #pragma region Pre-compute one over w and get ndcCoords
    float   invertedW[9];
    float3  ndcCoords[9];

    for (int i = 0; i < pointCount; i++)
    {
        // Compute one over w foreach clip coord
        invertedW[i] = 1.f / outputPoints[i].coords.w;

        // Clip space (v4) to NDC (v3)
        // Get ndc coords from new clip coords
        ndcCoords[i] = outputPoints[i].coords.xyz * invertedW[i];
    }
    #pragma endregion

    #pragma region Face cull
    // Cull the faces with their 3 firsts points
    if (faceCulling(ndcCoords, renderer->uniform.faceOrientation, renderer->uniform.faceToCull))
        return;
    #pragma endregion

    #pragma region Screen coords and new varyings
    float4  screenCoords[9];
    Varying clippedVaryings[9];

    for (int i = 0; i < pointCount; i++)
    {
        // NDC (v3) to screen coords (v2 + depth + clipCoord w)
        screenCoords[i] = { ndcToScreenCoords(ndcCoords[i], renderer->viewport), invertedW[i] };

        // Get new varyings after clipping
        clippedVaryings[i] = interpolateVarying(varying, outputPoints[i].weights);
    }
    #pragma endregion

    #pragma region Rasterization and Wireframe
    // Rasterize triangles or draw triangle lines by getting the correct screenCoords and varyings
    for (int index0 = 0, index1 = 1, index2 = 2; index2 < pointCount; index1++, index2++)
    {
        const float4 pointCoords[3] = { screenCoords[index0], screenCoords[index1], screenCoords[index2] };

        if (renderer->fillTriangle)
        {
            const Varying varyings[3] = { clippedVaryings[index0], clippedVaryings[index1], clippedVaryings[index2] };
            rasterTriangle(renderer->fb, pointCoords, varyings, renderer->uniform);
        }

        if (renderer->wireframeMode)
        {
            for (int i = 0; i < 3; i++)
                drawLine(renderer->fb, pointCoords[i].xyz, pointCoords[(i + 1) % 3].xyz, renderer->lineColor, renderer->uniform.msaa);
        }
    }
    #pragma endregion
}

void rdrDrawTriangles(rdrImpl* renderer, const rdrVertex* vertices, int count)
{
    // Pre-compute view proj for the current triangle
    renderer->uniform.viewProj = renderer->uniform.projection * renderer->uniform.view;

    // Transform vertex list to triangles into colorBuffer
    for (int i = 0; i < count; i += 3)
        drawTriangle(renderer, &vertices[i]);
}

void rdrSetImGuiContext(rdrImpl* renderer, struct ImGuiContext* context)
{
    ImGui::SetCurrentContext(context);
}

void rdrShowImGuiControls(rdrImpl* renderer)
{
    ImGui::Checkbox("MSAA", &renderer->uniform.msaa);

    #pragma region Lighting tree
    if (ImGui::TreeNode("Lighting"))
    {
        ImGui::Checkbox("Lighting", &renderer->uniform.lighting);

        if (renderer->uniform.lighting)
        {
            ImGui::Checkbox("Phong model", &renderer->uniform.phongModel);
            ImGui::ColorEdit4("Global ambient", renderer->uniform.globalAmbient.e, ImGuiColorEditFlags_Float);
        }

        ImGui::TreePop();
    }
    #pragma endregion

    #pragma region Rasterization tree
    if (ImGui::TreeNode("Rasterization"))
    {
        ImGui::Checkbox("Rasterize triangle", &renderer->fillTriangle);

        if (renderer->fillTriangle)
        {
            #pragma region Texture filtering
            {
                const char* filterTypeStr[] = { "NEAREST", "BILINEAR" };
                int filterTypeIndex = (int)renderer->uniform.textureFilter;
                if (ImGui::Combo("Texture filter", &filterTypeIndex, filterTypeStr, IM_ARRAYSIZE(filterTypeStr)))
                    renderer->uniform.textureFilter = FilterType(filterTypeIndex);
            }
            #pragma endregion

            #pragma region Blending tree
            if (ImGui::TreeNode("Blending"))
            {
                ImGui::Checkbox("Blending", &renderer->uniform.blending);

                if (renderer->uniform.blending)
                    ImGui::SliderFloat("Cutout", &renderer->uniform.cutout, 0.f, 1.f);

                ImGui::TreePop();
            }
            #pragma endregion 

            #pragma region Face cull tree
            if (ImGui::TreeNode("Face culling"))
            {
                // Face orientation of front-facing polygons
                {
                    const char* faceOrientationStr[] = { "Clockwise", "Counter-Clockwise" };
                    int faceOrientationIndex = (int)renderer->uniform.faceOrientation;
                    if (ImGui::Combo("Face orientation", &faceOrientationIndex, faceOrientationStr, IM_ARRAYSIZE(faceOrientationStr)))
                        renderer->uniform.faceOrientation = FaceOrientation(faceOrientationIndex);
                }

                // Face to cull
                {
                    const char* faceTypeStr[] = { "None", "Back", "Front", "Front and back" };
                    int faceTypeIndex = (int)renderer->uniform.faceToCull;
                    if (ImGui::Combo("Face to cull", &faceTypeIndex, faceTypeStr, IM_ARRAYSIZE(faceTypeStr)))
                        renderer->uniform.faceToCull = FaceType(faceTypeIndex);
                }
                ImGui::TreePop();
            }
            #pragma endregion

            ImGui::Checkbox("Perspective correction", &renderer->uniform.perspectiveCorrection);
            ImGui::Checkbox("Depthtest", &renderer->uniform.depthTest);

            ImGui::ColorEdit4("Global color", renderer->uniform.globalColor.e, ImGuiColorEditFlags_Float);
        }

        ImGui::TreePop();
    }
    #pragma endregion

    #pragma region Wireframe tree
    if (ImGui::TreeNode("Wireframe"))
    {
        ImGui::Checkbox("Wireframe", &renderer->wireframeMode);

        if (renderer->wireframeMode)
            ImGui::ColorEdit4("Line color", renderer->lineColor.e, ImGuiColorEditFlags_Float);

        ImGui::TreePop();
    }
    #pragma endregion

    #pragma region Post-process tree
    if (ImGui::TreeNode("Post-Process"))
    {
        ImGui::Checkbox("Box blur", &renderer->boxBlur);
        ImGui::Checkbox("Gaussian blur", &renderer->gaussianBlur);
        ImGui::Checkbox("Light bloom", &renderer->lightBloom);

        if (ImGui::SliderFloat("Gamma", &renderer->gamma, 0.01f, 10.f));
            renderer->iGamma = 1.f / renderer->gamma;

        ImGui::TreePop();
    }
    #pragma endregion
}