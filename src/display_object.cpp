#include "display_object.h"

DisplayObject::DisplayObject()
    : x(0), y(0), width(0), height(0)
{}

DisplayObject::DisplayObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                             const uint8_t* rawData, size_t dataLen)
    : x(x), y(y), width(width), height(height),
      pixels(rawData, rawData + dataLen)
{}
