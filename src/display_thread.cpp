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
              << " line_length=" << finfo_.line_length
              << " smem_len=" << finfo_.smem_len << "\n";

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

    // 5. Set up grayscale palette for 8 bpp
    const uint32_t bpp = vinfo_.bits_per_pixel;
    if (bpp == 8) {
        uint16_t red[256], green[256], blue[256];
        for (int i = 0; i < 256; ++i)
            red[i] = green[i] = blue[i] = static_cast<uint16_t>((i << 8) | i);
        struct fb_cmap cmap {};
        cmap.start = 0;
        cmap.len   = 256;
        cmap.red   = red;
        cmap.green = green;
        cmap.blue  = blue;
        cmap.transp = nullptr;
        if (ioctl(fb_fd_, FBIOPUTCMAP, &cmap) < 0)
            std::cerr << "[Display] FBIOPUTCMAP: " << strerror(errno) << "\n";
    }

    // 6. Back buffer (reused each frame)
    std::vector<uint8_t> back(fb_size_, 0);

    // LUT for 32bpp: grayscale → 0xFFRRGGBB
    uint32_t lut[256];
    for (int i = 0; i < 256; ++i)
        lut[i] = 0xFF000000u | ((uint32_t)i << 16) | ((uint32_t)i << 8) | (uint32_t)i;

    const int Bpp = static_cast<int>(bpp / 8);

    while (true) {
        // 7. Wait for vsync; fall back to ~20ms sleep if unsupported
        int dummy = 0;
        if (ioctl(fb_fd_, FBIO_WAITFORVSYNC, &dummy) < 0) {
            if (errno == ENOTTY || errno == EINVAL || errno == ENOSYS)
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        // 8. Snapshot window list (single mutex acquisition)
        const auto windows = window_list_.snapshot();

        // 9. Skip frame if scene is static
        bool scene_dirty = false;
        for (Window* w : windows)
            if (w->isDirty()) { scene_dirty = true; break; }
        if (!scene_dirty) continue;

        auto frame_start = std::chrono::steady_clock::now();

        // 10. Bounding box of all visible windows
        const int scr_w = static_cast<int>(vinfo_.xres);
        const int scr_h = static_cast<int>(vinfo_.yres);
        int bb_x0 = scr_w, bb_y0 = scr_h, bb_x1 = 0, bb_y1 = 0;
        for (Window* w : windows) {
            bb_x0 = std::min(bb_x0, std::max(0, w->getPosX()));
            bb_y0 = std::min(bb_y0, std::max(0, w->getPosY()));
            bb_x1 = std::max(bb_x1, std::min(scr_w, w->getPosX() + w->getWidth()));
            bb_y1 = std::max(bb_y1, std::min(scr_h, w->getPosY() + w->getHeight()));
        }
        if (bb_x0 >= bb_x1 || bb_y0 >= bb_y1) continue;

        // 11. Clear bounding box in back buffer
        for (int y = bb_y0; y < bb_y1; ++y)
            std::memset(back.data() + y * finfo_.line_length + bb_x0 * Bpp,
                        0, static_cast<size_t>(bb_x1 - bb_x0) * Bpp);

        // 12. Composite windows into back buffer
        auto t_composite = std::chrono::steady_clock::now();
        for (Window* w : windows) {
            const auto& pixels = w->getPixels();
            if (pixels.empty()) continue;
            const int px = w->getPosX(), py = w->getPosY();
            const int pw = w->getWidth(), ph = w->getHeight();

            // Pre-compute clip rectangle
            const int cx0 = std::max(0, -px), cx1 = std::min(pw, scr_w - px);
            const int cy0 = std::max(0, -py), cy1 = std::min(ph, scr_h - py);
            if (cx0 >= cx1 || cy0 >= cy1) continue;

            for (int wy = cy0; wy < cy1; ++wy) {
                const int sy = py + wy;
                const uint8_t* src = pixels.data() + wy * pw + cx0;
                if (bpp == 8) {
                    std::memcpy(back.data() + sy * finfo_.line_length + (px + cx0),
                                src, cx1 - cx0);
                } else { // 32bpp
                    uint32_t* dst = reinterpret_cast<uint32_t*>(
                        back.data() + sy * finfo_.line_length) + (px + cx0);
                    for (int i = 0, n = cx1 - cx0; i < n; ++i)
                        dst[i] = lut[src[i]];
                }
            }
        }

        // 13. Copy bounding box to framebuffer
        auto t_copy = std::chrono::steady_clock::now();
        for (int y = bb_y0; y < bb_y1; ++y)
            std::memcpy(fb_ptr_ + y * finfo_.line_length + bb_x0 * Bpp,
                        back.data() + y * finfo_.line_length + bb_x0 * Bpp,
                        static_cast<size_t>(bb_x1 - bb_x0) * Bpp);

        auto t_end = std::chrono::steady_clock::now();
        auto ms = [](auto a, auto b){
            return std::chrono::duration<double,std::milli>(b-a).count(); };
        std::cout << "[Display] composite=" << ms(t_composite, t_copy)
                  << "ms copy=" << ms(t_copy, t_end)
                  << "ms total=" << ms(frame_start, t_end) << "ms\n";
    }
}
