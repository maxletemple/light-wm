#include "picture_object.h"
#include <cstring>

PictureObject::PictureObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                             const uint8_t* pixelData, size_t len)
    : DisplayObject(x, y, width, height, pixelData, len), dirty_(true)
{}

bool PictureObject::needRefresh()
{
    if (dirty_) { dirty_ = false; return true; }
    return false;
}

void PictureObject::render(const RenderContext& ctx, int abs_x, int abs_y)
{
    const int cx0 = std::max(0, -abs_x), cx1 = std::min((int)width,  ctx.scr_w - abs_x);
    const int cy0 = std::max(0, -abs_y), cy1 = std::min((int)height, ctx.scr_h - abs_y);
    if (cx0 >= cx1 || cy0 >= cy1) return;

    for (int row = cy0; row < cy1; ++row) {
        const uint8_t* src = pixels.data() + row * width + cx0;
        if (ctx.bpp == 8) {
            std::memcpy(ctx.buf + (abs_y + row) * ctx.stride + (abs_x + cx0),
                        src, cx1 - cx0);
        } else {
            uint32_t* dst = reinterpret_cast<uint32_t*>(
                ctx.buf + (abs_y + row) * ctx.stride) + (abs_x + cx0);
            for (int i = 0, n = cx1 - cx0; i < n; ++i) dst[i] = ctx.lut[src[i]];
        }
    }
}
