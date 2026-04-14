#include "window.h"
#include <algorithm>
#include <cstring>

Window::Window(int posX, int posY, int width, int height, uint8_t backgroundColor)
    : posX(posX), posY(posY), width(width), height(height),
      backgroundColor(backgroundColor),
      pixels(width * height, backgroundColor),
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

const std::vector<uint8_t>& Window::getPixels()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool dirty = dirty_;
    dirty_ = false;
    for (DisplayObject* obj : objects) {
        if (obj->needRefresh()) dirty = true;
    }
    if (!dirty)
        return pixels;

    std::fill(pixels.begin(), pixels.end(), backgroundColor);
    for (DisplayObject* obj : objects) {
        const auto objPixels = obj->getPixels();
        const int ox = obj->getX(), oy = obj->getY();
        const int ow = obj->getWidth(), oh = obj->getHeight();
        const int cx0 = std::max(0, -ox), cx1 = std::min(ow, width - ox);
        const int cy0 = std::max(0, -oy), cy1 = std::min(oh, height - oy);
        if (cx0 >= cx1 || cy0 >= cy1) continue;
        for (int row = cy0; row < cy1; ++row) {
            std::memcpy(pixels.data() + (oy + row) * width + (ox + cx0),
                        objPixels.data() + row * ow + cx0,
                        cx1 - cx0);
        }
    }
    return pixels;
}

void Window::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::fill(pixels.begin(), pixels.end(), backgroundColor);
}

void Window::transform(int newPosX, int newPosY, int newWidth, int newHeight)
{
    std::lock_guard<std::mutex> lock(mutex_);
    posX = newPosX;
    posY = newPosY;
    width = newWidth;
    height = newHeight;
    pixels.assign(width * height, backgroundColor);
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
