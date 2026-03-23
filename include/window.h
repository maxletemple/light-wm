#pragma once

#include <vector>
#include <display_object.h>

class Window {
    private:
        std::vector<DisplayObject> objects;
        uint8_t backgroundColor;
        int posX, posY;
        int width, height;
        bool needRefresh;
        std::vector<uint8_t> pixels;

    public:
        Window(int posX, int posY, int width, int height, uint8_t backgroundColor = 0);
        virtual ~Window() = default;
        const std::vector<uint8_t>& getPixels();
        virtual void clear();
        int getPosX()   const { return posX; }
        int getPosY()   const { return posY; }
        int getWidth()  const { return width; }
        int getHeight() const { return height; }
};