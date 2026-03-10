#include "soft_render/math/vec4.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace sr::math;

void test_vec4_perspective() {
    std::cout << "Running test_vec4_perspective..." << std::endl;

    // Case 1: w = 1.0 (Identity)
    {
        Vec4 v(1.0f, 2.0f, 3.0f, 1.0f);
        Vec3 p = v.perspective();
        assert(p.x == 1.0f);
        assert(p.y == 2.0f);
        assert(p.z == 3.0f);
        std::cout << "  w=1.0 passed" << std::endl;
    }

    // Case 2: w = 2.0 (Divide by 2)
    {
        Vec4 v(1.0f, 2.0f, 4.0f, 2.0f);
        Vec3 p = v.perspective();
        assert(p.x == 0.5f);
        assert(p.y == 1.0f);
        assert(p.z == 2.0f);
        std::cout << "  w=2.0 passed" << std::endl;
    }

    // Case 3: w = -1.0 (Negative w)
    {
        Vec4 v(1.0f, -2.0f, 3.0f, -1.0f);
        Vec3 p = v.perspective();
        assert(p.x == -1.0f);
        assert(p.y == 2.0f);
        assert(p.z == -3.0f);
        std::cout << "  w=-1.0 passed" << std::endl;
    }

    // Case 4: w = 0.0 (Division by zero - should result in inf/nan)
    {
        Vec4 v(1.0f, 2.0f, 3.0f, 0.0f);
        Vec3 p = v.perspective();
        assert(std::isinf(p.x) || std::isnan(p.x));
        assert(std::isinf(p.y) || std::isnan(p.y));
        assert(std::isinf(p.z) || std::isnan(p.z));
        std::cout << "  w=0.0 passed (inf/nan handled)" << std::endl;
    }

    std::cout << "test_vec4_perspective passed!" << std::endl;
}

int main() {
    test_vec4_perspective();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
