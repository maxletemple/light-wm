#pragma once

#include <vector>
#include <mutex>
#include "window.h"

class WindowList {
    private:
        std::vector<Window*> windows_;
        std::mutex mutex_;

    public:
        ~WindowList();
        void add(Window* win);
        void remove(int index);
        Window* getWindow(int index);
        int size();
};