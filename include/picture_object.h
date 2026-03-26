#pragma once

#include "display_object.h"

class PictureObject : public DisplayObject {
    bool dirty_;

public:
    PictureObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                  const uint8_t* pixels, size_t len);
    bool needRefresh() override;
    bool isDirty() const override { return dirty_; }
    std::vector<uint8_t> getPixels() override;
};
