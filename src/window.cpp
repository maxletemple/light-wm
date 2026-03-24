#include "window.h"

Window::Window(int posX, int posY, int width, int height, uint8_t backgroundColor)
    : posX(posX), posY(posY), width(width), height(height),
      backgroundColor(backgroundColor),
      pixels(width * height, backgroundColor),
      needRefresh(true)
{}

const std::vector<uint8_t>& Window::getPixels()
{
    this->clear();
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
    needRefresh = true;
}
