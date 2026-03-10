#include <cassert>
#include <iostream>
#include <limits>
#include "soft_render/core/framebuffer.hpp"
#include "soft_render/math/vec3.hpp"

using namespace sr::core;
using namespace sr::math;

void test_framebuffer_clear() {
    std::cout << "Running test_framebuffer_clear..." << std::endl;

    int width = 10;
    int height = 10;
    Framebuffer fb(width, height);

    // Test clear with default color (black)
    fb.clear();
    const Pixel* pixels = fb.pixels();
    const float* depth = fb.depthBuffer();

    for (int i = 0; i < width * height; ++i) {
        assert(pixels[i].r == 0);
        assert(pixels[i].g == 0);
        assert(pixels[i].b == 0);
        assert(pixels[i].a == 255);
        assert(depth[i] == std::numeric_limits<float>::infinity());
    }
    std::cout << "  Default clear passed" << std::endl;

    // Test clear with custom color
    Color customColor(1.0f, 0.5f, 0.25f);
    fb.clear(customColor);

    for (int i = 0; i < width * height; ++i) {
        assert(pixels[i].r == 255);
        assert(pixels[i].g == 127);
        assert(pixels[i].b == 63);
        assert(pixels[i].a == 255);
        assert(depth[i] == std::numeric_limits<float>::infinity());
    }
    std::cout << "  Custom color clear passed" << std::endl;

    // Test clear with out-of-bounds color values
    Color outOfBoundsColor(1.5f, -0.5f, 2.0f);
    fb.clear(outOfBoundsColor);

    for (int i = 0; i < width * height; ++i) {
        assert(pixels[i].r == 255); // clamped to 1.0
        assert(pixels[i].g == 0);   // clamped to 0.0
        assert(pixels[i].b == 255); // clamped to 1.0
        assert(pixels[i].a == 255);
        assert(depth[i] == std::numeric_limits<float>::infinity());
    }
    std::cout << "  Out-of-bounds color clear passed" << std::endl;

    // Test depth test modifying depth, then clear
    fb.depthTest(0, 0, 0.5f);
    assert(fb.getDepth(0, 0) == 0.5f);
    fb.clear();
    assert(fb.getDepth(0, 0) == std::numeric_limits<float>::infinity());
    std::cout << "  Depth buffer reset passed" << std::endl;

    std::cout << "test_framebuffer_clear passed!" << std::endl;
}

int main() {
    test_framebuffer_clear();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
