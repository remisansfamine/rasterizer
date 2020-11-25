#pragma once

#include <msf_gif.h>

class GifRecorder
{
public:
    GifRecorder(int width, int height);
    ~GifRecorder();

    void begin();
    void end(const char* file);
    void frame(float* colorBuffer);

private:
    int width = 0;
    int height = 0;
    unsigned char* gifBuffer = nullptr;
    MsfGifState gifState = {};
};