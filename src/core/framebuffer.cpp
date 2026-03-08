#include "soft_render/core/framebuffer.hpp"
#include <cstdio>

namespace sr {
namespace core {

bool Framebuffer::writePPM(const char* path) const {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", width_, height_);
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            const Pixel& p = color_[y * width_ + x];
            std::fwrite(&p, 3, 1, f);
        }
    }
    std::fclose(f);
    return true;
}

}
}
