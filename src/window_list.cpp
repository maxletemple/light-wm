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

void WindowList::move(int from, int to)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (from < 0 || from >= (int)windows_.size() || to < 0 || to >= (int)windows_.size())
        return;
    Window* win = windows_[from];
    windows_.erase(windows_.begin() + from);
    windows_.insert(windows_.begin() + to, win);
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
