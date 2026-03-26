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
    DisplayObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                  const uint8_t* rawData, size_t dataLen);
    virtual ~DisplayObject() = default;
    virtual bool needRefresh() = 0;
    virtual bool isDirty() const = 0;
    virtual std::vector<uint8_t> getPixels() = 0;
    uint16_t getX()      const { return x; }
    uint16_t getY()      const { return y; }
    uint16_t getWidth()  const { return width; }
    uint16_t getHeight() const { return height; }
};