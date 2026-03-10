#include "soft_render/math/vec4.hpp"
#include "soft_render/math/vec3.hpp"
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


void test_vec3_dot() {
    std::cout << "Running test_vec3_dot..." << std::endl;

    // Case 1: Orthogonal vectors (dot product = 0)
    {
        Vec3 v1(1.0f, 0.0f, 0.0f);
        Vec3 v2(0.0f, 1.0f, 0.0f);
        assert(std::abs(v1.dot(v2)) < 1e-6f);
        std::cout << "  Orthogonal vectors passed" << std::endl;
    }

    // Case 2: Parallel vectors (dot product = length1 * length2)
    {
        Vec3 v1(2.0f, 0.0f, 0.0f);
        Vec3 v2(3.0f, 0.0f, 0.0f);
        assert(std::abs(v1.dot(v2) - 6.0f) < 1e-6f);
        std::cout << "  Parallel vectors passed" << std::endl;
    }

    // Case 3: Anti-parallel vectors
    {
        Vec3 v1(2.0f, 0.0f, 0.0f);
        Vec3 v2(-3.0f, 0.0f, 0.0f);
        assert(std::abs(v1.dot(v2) - (-6.0f)) < 1e-6f);
        std::cout << "  Anti-parallel vectors passed" << std::endl;
    }

    // Case 4: Vector with itself (dot product = length squared)
    {
        Vec3 v(1.0f, 2.0f, 3.0f);
        assert(std::abs(v.dot(v) - 14.0f) < 1e-6f);
        std::cout << "  Vector with itself passed" << std::endl;
    }

    // Case 5: Zero vector
    {
        Vec3 v1(0.0f, 0.0f, 0.0f);
        Vec3 v2(1.0f, 2.0f, 3.0f);
        assert(std::abs(v1.dot(v2)) < 1e-6f);
        std::cout << "  Zero vector passed" << std::endl;
    }

    // Case 6: Mixed components
    {
        Vec3 v1(1.0f, 2.0f, 3.0f);
        Vec3 v2(4.0f, -5.0f, 6.0f);
        assert(std::abs(v1.dot(v2) - 12.0f) < 1e-6f); // 4 - 10 + 18 = 12
        std::cout << "  Mixed components passed" << std::endl;
    }

    std::cout << "test_vec3_dot passed!" << std::endl;
}

int main() {
    test_vec4_perspective();
    test_vec3_dot();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
