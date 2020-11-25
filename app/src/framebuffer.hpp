#pragma once

#include <vector>

#include <glad/glad.h>

#include <common/types.hpp>

struct Framebuffer
{
    Framebuffer(int width, int height);
    ~Framebuffer();

    void clear();
    void updateTexture();

    float* getColorBuffer() { return reinterpret_cast<float*>(colorBuffer.data()); }
    float* getDepthBuffer() { return depthBuffer.data(); }
    int getWidth()  const   { return width; }
    int getHeight() const   { return height; }

    GLuint getColorTexture() const { return colorTexture; }

    float4 clearColor = { 0.f, 0.f, 0.f, 1.f };
private:
    int width = 0;
    int height = 0;

    // In-RAM buffers
    std::vector<float4> colorBuffer;
    std::vector<float> depthBuffer;

    // OpenGL texture (in VRAM)
    GLuint colorTexture = 0;
};