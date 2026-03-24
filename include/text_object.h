#pragma once

#include <string>
#include "display_object.h"

class TextObject : public DisplayObject {
    std::string text_;
    uint16_t fontSize_;
    bool dirty_;

public:
    TextObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
               uint16_t fontSize, const std::string& text);
    bool needRefresh() override;
    std::vector<uint8_t> getPixels() override;
};
