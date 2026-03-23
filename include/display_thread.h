#pragma once

#include <cstdint>
#include <linux/fb.h>
#include <window_list.h>

class DisplayThread {
private:
    WindowList& window_list_;
    struct fb_var_screeninfo vinfo_;
    struct fb_fix_screeninfo finfo_;
    uint8_t* fb_ptr_;
    size_t   fb_size_;
    int      fb_fd_;

    void cleanup();

public:
    DisplayThread(WindowList& window_list)
        : window_list_(window_list),
          vinfo_{}, finfo_{},
          fb_ptr_(nullptr), fb_size_(0), fb_fd_(-1)
    {}

    void run();
};
