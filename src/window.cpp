#include "window.h"

Window::Window(int posX, int posY, int width, int height, uint8_t backgroundColor)
    : posX(posX), posY(posY), width(width), height(height),
      backgroundColor(backgroundColor),
      pixels(width * height, backgroundColor)
{}

const std::vector<uint8_t>& Window::getPixels()
{
    bool dirty = false;
    for (DisplayObject* obj : objects) {
        if (obj->needRefresh()) { dirty = true; break; }
    }
    if (!dirty)
        return pixels;

    clear();
    for (DisplayObject* obj : objects) {
        const auto objPixels = obj->getPixels();
        const int ox = obj->getX(), oy = obj->getY();
        const int ow = obj->getWidth(), oh = obj->getHeight();
        for (int py = 0; py < oh; ++py) {
            const int wy = oy + py;
            if (wy < 0 || wy >= height) continue;
            for (int px = 0; px < ow; ++px) {
                const int wx = ox + px;
                if (wx < 0 || wx >= width) continue;
                pixels[wy * width + wx] = objPixels[py * ow + px];
            }
        }
    }
    return pixels;
}

void Window::clear()
{
    for (int i = 0; i < width * height; i++) {
        pixels[i] = backgroundColor;
    }
}

void Window::transform(int newPosX, int newPosY, int newWidth, int newHeight)
{
    posX = newPosX;
    posY = newPosY;
    width = newWidth;
    height = newHeight;
    pixels.assign(width * height, backgroundColor);
}

void Window::addObject(DisplayObject* obj)
{
    objects.push_back(obj);
}

void Window::setObject(int index, DisplayObject* obj)
{
    if (index < (int)objects.size()) {
        delete objects[index];
        objects[index] = obj;
    } else {
        objects.push_back(obj);
    }
}

void Window::removeObject(int index)
{
    if (index < 0 || index >= (int)objects.size())
        return;
    delete objects[index];
    objects.erase(objects.begin() + index);
}
