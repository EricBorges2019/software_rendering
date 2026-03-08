#pragma once

#include "../math/vec3.hpp"
#include <vector>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace sr {
namespace core {

struct Pixel {
    uint8_t r, g, b, a;
};

class Framebuffer {
public:
    Framebuffer(int width, int height)
        : width_(width), height_(height)
        , color_(width * height)
        , depth_(width * height, std::numeric_limits<float>::infinity())
    {}

    void resize(int w, int h) {
        width_ = w; height_ = h;
        color_.assign(w * h, {0, 0, 0, 255});
        depth_.assign(w * h, std::numeric_limits<float>::infinity());
    }

    void clear(const math::Color& color = {0, 0, 0}) {
        Pixel p = toPixel(color);
        std::fill(color_.begin(), color_.end(), p);
        std::fill(depth_.begin(), depth_.end(), std::numeric_limits<float>::infinity());
    }

    void clearDepth() {
        std::fill(depth_.begin(), depth_.end(), std::numeric_limits<float>::infinity());
    }

    // Returns true if depth test passes (and writes depth)
    inline bool depthTest(int x, int y, float z) {
        float& d = depth_[y * width_ + x];
        if (z < d) { d = z; return true; }
        return false;
    }

    inline void setPixel(int x, int y, const math::Color& c) {
        color_[y * width_ + x] = toPixel(c);
    }

    inline float getDepth(int x, int y) const {
        return depth_[y * width_ + x];
    }

    int width() const { return width_; }
    int height() const { return height_; }
    const Pixel* pixels() const { return color_.data(); }
    const float* depthBuffer() const { return depth_.data(); }

    // Write as PPM to stdout or file
    bool writePPM(const char* path) const;

private:
    int width_, height_;
    std::vector<Pixel> color_;
    std::vector<float> depth_;

    static float saturate(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }

    static Pixel toPixel(const math::Color& c) {
        return {
            static_cast<uint8_t>(saturate(c.x) * 255.f),
            static_cast<uint8_t>(saturate(c.y) * 255.f),
            static_cast<uint8_t>(saturate(c.z) * 255.f),
            255
        };
    }
};

}
}
