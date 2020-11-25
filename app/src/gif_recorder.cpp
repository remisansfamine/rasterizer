
#include <cstdio>

#include "gif_recorder.hpp"

GifRecorder::GifRecorder(int width, int height)
    : width(width), height(height)
{
    gifBuffer = new unsigned char[width * height * 4]();
}

GifRecorder::~GifRecorder()
{
    delete[] gifBuffer;
}

void GifRecorder::begin()
{
    gifState = {};
    msf_gif_begin(&gifState, width, height);
}

void GifRecorder::end(const char* file)
{
    MsfGifResult result = msf_gif_end(&gifState);
    FILE* fp = fopen("anim.gif", "wb");
    fwrite(result.data, result.dataSize, 1, fp);
    fclose(fp);
    msf_gif_free(result);
}

void GifRecorder::frame(float* colorBuffer)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            unsigned char* gifPixel = &gifBuffer[(x + y * width) * 4];
            float* fbPixel = &colorBuffer[(x + y * width) * 4];

            gifPixel[0] = (unsigned char)(fbPixel[0] * 255.f);
            gifPixel[1] = (unsigned char)(fbPixel[1] * 255.f);
            gifPixel[2] = (unsigned char)(fbPixel[2] * 255.f);
            gifPixel[3] = (unsigned char)(fbPixel[3] * 255.f);
        }
    }
    msf_gif_frame(&gifState, gifBuffer, 2, 16, 0);
}

