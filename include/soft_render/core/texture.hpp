#pragma once

#include "../math/vec3.hpp"
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace sr {
namespace core {

class Texture {
public:
    Texture() : width_(0), height_(0) {}
    Texture(int w, int h) : width_(w), height_(h), data_(w * h * 3, 255) {}

    static Texture checkerboard(int w, int h, int squareSize = 16) {
        Texture t(w, h);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                bool even = ((x / squareSize) + (y / squareSize)) % 2 == 0;
                uint8_t v = even ? 255 : 50;
                int idx = (y * w + x) * 3;
                t.data_[idx] = v;
                t.data_[idx+1] = v;
                t.data_[idx+2] = v;
            }
        }
        return t;
    }

    // Bilinear sample, uv in [0,1]
    math::Color sample(float u, float v) const {
        if (width_ == 0 || height_ == 0) return {1, 1, 1};

        // Wrap
        u = u - std::floor(u);
        v = v - std::floor(v);

        float fx = u * (width_ - 1);
        float fy = v * (height_ - 1);
        int x0 = static_cast<int>(fx), y0 = static_cast<int>(fy);
        int x1 = std::min(x0 + 1, width_ - 1);
        int y1 = std::min(y0 + 1, height_ - 1);
        float tx = fx - x0, ty = fy - y0;

        math::Color c00 = fetch(x0, y0);
        math::Color c10 = fetch(x1, y0);
        math::Color c01 = fetch(x0, y1);
        math::Color c11 = fetch(x1, y1);

        return c00 * ((1 - tx) * (1 - ty)) +
               c10 * (tx * (1 - ty)) +
               c01 * ((1 - tx) * ty) +
               c11 * (tx * ty);
    }

    // Nearest-neighbor (faster)
    math::Color sampleNearest(float u, float v) const {
        if (width_ == 0 || height_ == 0) return {1, 1, 1};
        u = u - std::floor(u);
        v = v - std::floor(v);
        int x = static_cast<int>(u * width_) % width_;
        int y = static_cast<int>(v * height_) % height_;
        return fetch(x, y);
    }

    int width() const { return width_; }
    int height() const { return height_; }
    bool valid() const { return width_ > 0 && height_ > 0; }

    uint8_t* rawData() { return data_.data(); }
    const uint8_t* rawData() const { return data_.data(); }

private:
    int width_, height_;
    std::vector<uint8_t> data_; // RGB, row-major

    math::Color fetch(int x, int y) const {
        int idx = (y * width_ + x) * 3;
        return {
            data_[idx]   / 255.f,
            data_[idx+1] / 255.f,
            data_[idx+2] / 255.f
        };
    }
};

}
}
