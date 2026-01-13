//#include <algorithm>

#include "framebuffer.hpp"

Framebuffer::Framebuffer(int width, int height)
    : width(width)
    , height(height)
    , depthBuffer(width* height)
{
    glGenBuffers(bufferingCount, colorPixelBuffers);
    for (GLuint i = 0u; i < bufferingCount; i++)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, colorPixelBuffers[i]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * sizeof(float4), nullptr, GL_DYNAMIC_DRAW);
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, colorPixelBuffers[pixelBufferID]);
    colorBufferPtr = static_cast<float4*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, width * height * sizeof(float4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    // Avoid binding future textures with the pixel buffer
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
}

Framebuffer::~Framebuffer()
{
    if (colorBufferPtr)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, colorPixelBuffers[pixelBufferID]);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        colorBufferPtr = nullptr;
    }
    glDeleteBuffers(bufferingCount, colorPixelBuffers);

    glDeleteTextures(1, &colorTexture);
}

void Framebuffer::clear()
{
    // Sadly too slow...
    //std::fill(colorBuffer.begin(), colorBuffer.end(), clearColor);
    //std::fill(depthBuffer.begin(), depthBuffer.end(), 0.f);

    // Clear color buffer
    if (colorBufferPtr)
    {
        float4* colors = colorBufferPtr;
    
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
    const GLuint uploadPBO = colorPixelBuffers[pixelBufferID];
    const unsigned int pixelBufferNextID = (pixelBufferID + 1u) % bufferingCount;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uploadPBO);
    if (colorBufferPtr)
    {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        colorBufferPtr = nullptr;
    }

    glBindTexture(GL_TEXTURE_2D, colorTexture);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, colorBufferPtr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, 0);

    const GLuint mapPBO = colorPixelBuffers[pixelBufferNextID];

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mapPBO);
    colorBufferPtr = static_cast<float4*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, width * height * sizeof(float4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    pixelBufferID = pixelBufferNextID;
}
