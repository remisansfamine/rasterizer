//#include <algorithm>

#include "framebuffer.hpp"

Framebuffer::Framebuffer(int width, int height)
    : width(width)
    , height(height)
    , colorBuffer(width* height)
    , depthBuffer(width* height)
{
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
}

Framebuffer::~Framebuffer()
{
    glDeleteTextures(1, &colorTexture);
}

void Framebuffer::clear()
{
    // Sadly too slow...
    //std::fill(colorBuffer.begin(), colorBuffer.end(), clearColor);
    //std::fill(depthBuffer.begin(), depthBuffer.end(), 0.f);

    // Clear color buffer
    {
        float4* colors = colorBuffer.data();

        // Fill the first line with the clear color
        for (size_t i = 0; i < width; ++i)
            memcpy(&colors[i], &clearColor, sizeof(float4));

        // Copy the first line onto every line
        for (size_t i = 1; i < height; ++i)
            memcpy(&colors[i * width], &colors[0], width * sizeof(float4));
    }

    // Clear depth buffer
    {
        memset(depthBuffer.data(), 0, depthBuffer.size() * sizeof(depthBuffer[0]));
    }
}

void Framebuffer::updateTexture()
{
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, colorBuffer.data());
}
