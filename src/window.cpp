#include "window.h"
#include <algorithm>
#include <cstring>

Window::Window(int posX, int posY, int width, int height, uint8_t backgroundColor)
    : posX(posX), posY(posY), width(width), height(height),
      backgroundColor(backgroundColor),
      dirty_(true)
{}

bool Window::isDirty() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (dirty_) return true;
    for (DisplayObject* obj : objects)
        if (obj->isDirty()) return true;
    return false;
}

void Window::render(const RenderContext& ctx)
{
    std::lock_guard<std::mutex> lock(mutex_);

    bool dirty = dirty_;
    dirty_ = false;
    for (DisplayObject* obj : objects)
        if (obj->needRefresh()) dirty = true;
    if (!dirty) return;

    const int cx0 = std::max(0, posX), cy0 = std::max(0, posY);
    const int cx1 = std::min(ctx.scr_w, posX + width);
    const int cy1 = std::min(ctx.scr_h, posY + height);
    if (cx0 >= cx1 || cy0 >= cy1) return;

    const int Bpp = static_cast<int>(ctx.bpp / 8);
    const uint32_t bg32 = ctx.lut ? ctx.lut[backgroundColor] : backgroundColor;

    for (int y = cy0; y < cy1; ++y) {
        uint8_t* row = ctx.buf + y * ctx.stride + cx0 * Bpp;
        if (ctx.bpp == 8) {
            std::memset(row, backgroundColor, cx1 - cx0);
        } else {
            uint32_t* p = reinterpret_cast<uint32_t*>(row);
            for (int x = 0; x < cx1 - cx0; ++x) p[x] = bg32;
        }
    }

    for (DisplayObject* obj : objects)
        obj->render(ctx, posX + obj->getX(), posY + obj->getY());
}

void Window::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    dirty_ = true;
}

void Window::transform(int newPosX, int newPosY, int newWidth, int newHeight)
{
    std::lock_guard<std::mutex> lock(mutex_);
    posX = newPosX;
    posY = newPosY;
    width = newWidth;
    height = newHeight;
    dirty_ = true;
}

void Window::addObject(DisplayObject* obj)
{
    std::lock_guard<std::mutex> lock(mutex_);
    objects.push_back(obj);
}

void Window::setObject(int index, DisplayObject* obj)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (index < (int)objects.size()) {
        delete objects[index];
        objects[index] = obj;
    } else {
        objects.push_back(obj);
    }
}

void Window::removeObject(int index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (index < 0 || index >= (int)objects.size())
        return;
    delete objects[index];
    objects.erase(objects.begin() + index);
}
