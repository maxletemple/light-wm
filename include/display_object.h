#pragma once

#include <cstdint>
#include <vector>

struct RenderContext {
    uint8_t*  buf;      // fb_ptr_ ou back buffer
    uint32_t  stride;   // finfo_.line_length (octets/ligne)
    uint32_t  bpp;      // 8 ou 32
    uint32_t* lut;      // LUT grayscale→0xFFRRGGBB, nullptr si bpp==8
    int       scr_w;
    int       scr_h;
};

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
    virtual void render(const RenderContext& ctx, int abs_x, int abs_y) = 0;
    uint16_t getX()      const { return x; }
    uint16_t getY()      const { return y; }
    uint16_t getWidth()  const { return width; }
    uint16_t getHeight() const { return height; }
};
