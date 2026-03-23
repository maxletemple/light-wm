#include "window_list.h"

WindowList::~WindowList()
{
    for (Window* w : windows_)
        delete w;
}

void WindowList::add(Window* win)
{
    std::lock_guard<std::mutex> lock(mutex_);
    windows_.push_back(win);
}

void WindowList::remove(int index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    delete windows_[index];
    windows_.erase(windows_.begin() + index);
}

Window* WindowList::getWindow(int index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return windows_[index];
}

int WindowList::size()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return windows_.size();
}
