#include <cassert>
#include <iostream>
#include "soft_render/core/framebuffer.hpp"
#include "soft_render/math/vec3.hpp"

using namespace sr::core;
using namespace sr::math;

void test_framebuffer_setPixel() {
    std::cout << "Running test_framebuffer_setPixel..." << std::endl;

    Framebuffer fb(10, 10);

    // Test 1: Basic setting
    fb.setPixel(5, 5, Color(1.0f, 0.5f, 0.0f));
    const Pixel* pixels = fb.pixels();
    Pixel p = pixels[5 * 10 + 5];
    assert(p.r == 255);
    assert(p.g == 127);
    assert(p.b == 0);
    assert(p.a == 255);

    // Test 2: Saturation logic (clamping <0 to 0 and >1 to 1)
    fb.setPixel(0, 0, Color(-1.0f, 2.0f, 0.5f));
    Pixel p2 = pixels[0];
    assert(p2.r == 0);
    assert(p2.g == 255);
    assert(p2.b == 127);
    assert(p2.a == 255);

    // Test 3: Fractional handling
    fb.setPixel(9, 9, Color(0.25f, 0.75f, 1.0f));
    Pixel p3 = pixels[9 * 10 + 9];
    assert(p3.r == 63);
    assert(p3.g == 191);
    assert(p3.b == 255);
    assert(p3.a == 255);

    std::cout << "test_framebuffer_setPixel passed!" << std::endl;
}

int main() {
    test_framebuffer_setPixel();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
