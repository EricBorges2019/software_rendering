#include "soft_render/core/framebuffer.hpp"
#include <cstdio>
#include <vector>

namespace sr {
namespace core {

bool Framebuffer::writePPM(const char* path) const {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", width_, height_);

    // Buffer a whole row to reduce fwrite system calls
    // PPM requires RGB format (3 bytes per pixel)
    // Use size_t arithmetic to avoid int overflow for large widths
    const size_t rowBytes = static_cast<size_t>(width_) * 3;
    std::vector<uint8_t> rowBuffer(rowBytes);
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            const Pixel& p = color_[y * width_ + x];
            rowBuffer[x * 3 + 0] = p.r;
            rowBuffer[x * 3 + 1] = p.g;
            rowBuffer[x * 3 + 2] = p.b;
        }
        if (std::fwrite(rowBuffer.data(), 1, rowBytes, f) != rowBytes) {
            std::fclose(f);
            return false;
        }
    }
    std::fclose(f);
    return true;
}

}
}
