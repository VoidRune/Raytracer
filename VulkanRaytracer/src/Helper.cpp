#include "Helper.h"
#include "../dependencies/stb/stb_image.h"

void* LoadImageFromFile(const char* filePath)
{
    int width, height, channels;
    void* data = nullptr;
    data = stbi_load("res/Image/test.jpg", &width, &height, &channels, 4);
    return data;
}

void ReleaseImageData(void* data)
{
    stbi_image_free(data);
}