#include "picture_object.h"

PictureObject::PictureObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                             const uint8_t* pixelData, size_t len)
    : DisplayObject(x, y, width, height, pixelData, len), dirty_(true)
{}

bool PictureObject::needRefresh()
{
    if (dirty_) { dirty_ = false; return true; }
    return false;
}

std::vector<uint8_t> PictureObject::getPixels()
{
    return pixels;
}
