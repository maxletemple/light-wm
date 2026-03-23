#include "display_thread.h"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <vector>
#include <thread>
#include <chrono>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

void DisplayThread::cleanup()
{
    if (fb_ptr_ && fb_ptr_ != MAP_FAILED) { munmap(fb_ptr_, fb_size_); fb_ptr_ = nullptr; }
    if (fb_fd_ >= 0) { close(fb_fd_); fb_fd_ = -1; }
}

void DisplayThread::run()
{
    // 1. Open framebuffer
    fb_fd_ = open("/dev/fb0", O_RDWR);
    if (fb_fd_ < 0) {
        std::cerr << "[Display] open /dev/fb0: " << strerror(errno) << "\n";
        return;
    }

    // 2. Read screen parameters
    if (ioctl(fb_fd_, FBIOGET_VSCREENINFO, &vinfo_) < 0) {
        std::cerr << "[Display] FBIOGET_VSCREENINFO: " << strerror(errno) << "\n";
        cleanup(); return;
    }
    if (ioctl(fb_fd_, FBIOGET_FSCREENINFO, &finfo_) < 0) {
        std::cerr << "[Display] FBIOGET_FSCREENINFO: " << strerror(errno) << "\n";
        cleanup(); return;
    }

    std::cout << "[Display] " << vinfo_.xres << "x" << vinfo_.yres
              << " " << vinfo_.bits_per_pixel << "bpp"
              << " pixclock=" << vinfo_.pixclock << "ps"
              << " line_length=" << finfo_.line_length << "\n";

    // 3. Validate bpp
    if (vinfo_.bits_per_pixel != 8 && vinfo_.bits_per_pixel != 32) {
        std::cerr << "[Display] Unsupported bpp: " << vinfo_.bits_per_pixel << " (expected 8 or 32)\n";
        cleanup(); return;
    }

    // 4. mmap framebuffer
    fb_size_ = finfo_.smem_len;
    fb_ptr_ = static_cast<uint8_t*>(
        mmap(nullptr, fb_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd_, 0)
    );
    if (fb_ptr_ == MAP_FAILED) {
        std::cerr << "[Display] mmap: " << strerror(errno) << "\n";
        fb_ptr_ = nullptr; cleanup(); return;
    }

    // 5. Back buffer (reused each frame)
    std::vector<uint8_t> back(fb_size_, 0);
    const uint32_t bpp = vinfo_.bits_per_pixel;

    while (true) {
        // 6. Wait for vsync; fall back to ~16ms sleep if unsupported
        int dummy = 0;
        if (ioctl(fb_fd_, FBIO_WAITFORVSYNC, &dummy) < 0) {
            if (errno == ENOTTY || errno == EINVAL || errno == ENOSYS)
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        // 7. Clear back buffer to black
        std::memset(back.data(), 0, fb_size_);

        // 8. Iterate window list
        const int win_count = window_list_.size();
        for (int i = 0; i < win_count; ++i) {
            Window* w = window_list_.getWindow(i);
            if (!w) continue;

            const auto& pixels = w->getPixels();
            const int px = w->getPosX(), py = w->getPosY();
            const int pw = w->getWidth(), ph = w->getHeight();

            for (int wy = 0; wy < ph; ++wy) {
                const int sy = py + wy;
                if (sy < 0 || sy >= static_cast<int>(vinfo_.yres)) continue;

                for (int wx = 0; wx < pw; ++wx) {
                    const int sx = px + wx;
                    if (sx < 0 || sx >= static_cast<int>(vinfo_.xres)) continue;

                    const uint8_t v = pixels[wy * pw + wx];

                    if (bpp == 8) {
                        size_t off = static_cast<size_t>(sy) * finfo_.line_length
                                   + static_cast<size_t>(sx);
                        back[off] = v;
                    } else { // 32
                        size_t off = static_cast<size_t>(sy) * finfo_.line_length
                                   + static_cast<size_t>(sx) * 4;
                        back[off]     = v;    // B
                        back[off + 1] = v;    // G
                        back[off + 2] = v;    // R
                        back[off + 3] = 0xFF; // A
                    }
                }
            }
        }

        // 9. Flip to framebuffer
        std::memcpy(fb_ptr_, back.data(), fb_size_);
    }
}
