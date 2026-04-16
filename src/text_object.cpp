#include "text_object.h"
#include "font8x8.h"
#include <algorithm>
#include <cstring>

TextObject::TextObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                       uint16_t fontSize, const std::string& text)
    : DisplayObject(x, y, width, height, nullptr, 0),
      text_(text), fontSize_(fontSize), dirty_(true)
{
    pixels.assign(width * height, 0xFF);

    const int scale = std::max(1, (int)fontSize_ / 8);
    const int charW = 8 * scale;
    const int charH = 8 * scale;

    int curX = 0, curY = 0;
    for (unsigned char c : text_) {
        if (c == '\n') {
            curX = 0;
            curY += charH;
            if (curY + charH > height) break;
            continue;
        }
        if (curX + charW > width) {
            curX = 0;
            curY += charH;
            if (curY + charH > height) break;
        }
        if (c < 0x20 || c > 0x7E) {
            curX += charW;
            continue;
        }

        const uint8_t* glyph = font8x8_basic[c - 0x20];
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                if ((glyph[row] >> col) & 1) {
                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            int px = curX + col * scale + sx;
                            int py = curY + row * scale + sy;
                            if (px < width && py < height)
                                pixels[py * width + px] = 0x00;
                        }
                    }
                }
            }
        }
        curX += charW;
    }
}

bool TextObject::needRefresh()
{
    if (dirty_) { dirty_ = false; return true; }
    return false;
}

void TextObject::render(const RenderContext& ctx, int abs_x, int abs_y)
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
