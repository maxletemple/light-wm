#pragma once

#include <vector>
#include <mutex>
#include <display_object.h>

class Window {
    private:
        mutable std::mutex mutex_;
        std::vector<DisplayObject*> objects;
        uint8_t backgroundColor;
        int posX, posY;
        int width, height;
        bool dirty_;

    public:
        Window(int posX, int posY, int width, int height, uint8_t backgroundColor = 0);
        virtual ~Window() = default;
        void render(const RenderContext& ctx);
        virtual void clear();
        int getPosX()   const { return posX; }
        int getPosY()   const { return posY; }
        int getWidth()  const { return width; }
        int getHeight() const { return height; }
        bool isDirty() const;
        void transform(int newPosX, int newPosY, int newWidth, int newHeight);
        void addObject(DisplayObject* obj);
        void setObject(int index, DisplayObject* obj);
        void removeObject(int index);
};
