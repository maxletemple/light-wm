#pragma once

#include <cstdint>
#include <vector>

class DisplayObject {
protected:
    uint16_t x, y;
    uint16_t width, height;
    std::vector<uint8_t> pixels;

public:
    DisplayObject();
    virtual ~DisplayObject() = default;
    virtual bool needRefresh();
    virtual std::vector<uint8_t> getPixels();
};