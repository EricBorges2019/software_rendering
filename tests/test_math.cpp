#include "soft_render/math/vec4.hpp"
#include "soft_render/math/mat4.hpp"

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


void test_mat4_perspective() {
    std::cout << "Running test_mat4_perspective..." << std::endl;

    float fov = 1.57079632679f; // 90 degrees
    float aspect = 1.777777778f; // 16:9
    float near = 0.1f;
    float far = 100.0f;

    Mat4 p = Mat4::perspective(fov, aspect, near, far);

    float tanHalfFov = std::tan(fov * 0.5f);
    float A = -(far + near) / (far - near);
    float B = -2.0f * far * near / (far - near);

    float eps = 1e-4f;

    // Check coefficients
    assert(std::abs(p(0, 0) - (1.0f / (aspect * tanHalfFov))) < eps);
    assert(p(0, 1) == 0.0f);
    assert(p(0, 2) == 0.0f);
    assert(p(0, 3) == 0.0f);

    assert(p(1, 0) == 0.0f);
    assert(std::abs(p(1, 1) - (1.0f / tanHalfFov)) < eps);
    assert(p(1, 2) == 0.0f);
    assert(p(1, 3) == 0.0f);

    assert(p(2, 0) == 0.0f);
    assert(p(2, 1) == 0.0f);
    assert(std::abs(p(2, 2) - A) < eps);
    assert(std::abs(p(2, 3) - B) < eps);

    assert(p(3, 0) == 0.0f);
    assert(p(3, 1) == 0.0f);
    assert(p(3, 2) == -1.0f);
    assert(p(3, 3) == 0.0f);
    std::cout << "  coefficients passed" << std::endl;

    // Test near plane projection
    Vec4 pt_near = {0.0f, 0.0f, -near, 1.0f};
    Vec4 clip_near = p * pt_near;
    assert(std::abs(clip_near.w - near) < eps);
    assert(std::abs((clip_near.z / clip_near.w) - (-1.0f)) < eps);
    std::cout << "  near plane projection passed" << std::endl;

    // Test far plane projection
    Vec4 pt_far = {0.0f, 0.0f, -far, 1.0f};
    Vec4 clip_far = p * pt_far;
    assert(std::abs(clip_far.w - far) < eps);
    assert(std::abs((clip_far.z / clip_far.w) - 1.0f) < eps);
    std::cout << "  far plane projection passed" << std::endl;

    // Test top-right near plane projection
    float top = near * tanHalfFov;
    float right = top * aspect;
    Vec4 pt_tr = {right, top, -near, 1.0f};
    Vec4 clip_tr = p * pt_tr;
    assert(std::abs((clip_tr.x / clip_tr.w) - 1.0f) < eps);
    assert(std::abs((clip_tr.y / clip_tr.w) - 1.0f) < eps);
    assert(std::abs((clip_tr.z / clip_tr.w) - (-1.0f)) < eps);
    std::cout << "  top-right near plane projection passed" << std::endl;

    std::cout << "test_mat4_perspective passed!" << std::endl;
}

int main() {
    test_vec4_perspective();
    test_mat4_perspective();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
