#include "soft_render/math/vec4.hpp"
#include "soft_render/math/mat4.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using std::cout;
using std::endl;
using namespace sr::math;

void test_vec4_perspective() {
    cout << "Running test_vec4_perspective..." << endl;

    // Case 1: w = 1.0 (Identity)
    {
        Vec4 v(1.0f, 2.0f, 3.0f, 1.0f);
        Vec3 p = v.perspective();
        assert(p.x == 1.0f);
        assert(p.y == 2.0f);
        assert(p.z == 3.0f);
        cout << "  w=1.0 passed" << endl;
    }

    // Case 2: w = 2.0 (Divide by 2)
    {
        Vec4 v(1.0f, 2.0f, 4.0f, 2.0f);
        Vec3 p = v.perspective();
        assert(p.x == 0.5f);
        assert(p.y == 1.0f);
        assert(p.z == 2.0f);
        cout << "  w=2.0 passed" << endl;
    }

    // Case 3: w = -1.0 (Negative w)
    {
        Vec4 v(1.0f, -2.0f, 3.0f, -1.0f);
        Vec3 p = v.perspective();
        assert(p.x == -1.0f);
        assert(p.y == 2.0f);
        assert(p.z == -3.0f);
        cout << "  w=-1.0 passed" << endl;
    }

    // Case 4: w = 0.0 (Division by zero - should result in inf/nan)
    {
        Vec4 v(1.0f, 2.0f, 3.0f, 0.0f);
        Vec3 p = v.perspective();
        assert(std::isinf(p.x) || std::isnan(p.x));
        assert(std::isinf(p.y) || std::isnan(p.y));
        assert(std::isinf(p.z) || std::isnan(p.z));
        cout << "  w=0.0 passed (inf/nan handled)" << endl;
    }

    cout << "test_vec4_perspective passed!" << endl;
}

void test_mat4_multiplication() {
    cout << "Running test_mat4_multiplication..." << endl;

    // Case 1: Identity * A = A
    {
        Mat4 I = Mat4::identity();
        Mat4 A = {
            1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f
        };
        Mat4 result = I * A;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                assert(std::abs(result(i, j) - A(i, j)) < 1e-5f);
            }
        }
        cout << "  Identity * A passed" << endl;
    }

    // Case 2: A * Identity = A
    {
        Mat4 I = Mat4::identity();
        Mat4 A = {
            1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f
        };
        Mat4 result = A * I;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                assert(std::abs(result(i, j) - A(i, j)) < 1e-5f);
            }
        }
        cout << "  A * Identity passed" << endl;
    }

    // Case 3: A * B
    {
        Mat4 A = {
            1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f
        };
        Mat4 B = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 3.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 4.0f
        };
        Mat4 result = A * B;

        Mat4 expected = {
            1.0f, 4.0f, 9.0f, 16.0f,
            5.0f, 12.0f, 21.0f, 32.0f,
            9.0f, 20.0f, 33.0f, 48.0f,
            13.0f, 28.0f, 45.0f, 64.0f
        };
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                assert(std::abs(result(i, j) - expected(i, j)) < 1e-5f);
            }
        }
        cout << "  A * B passed" << endl;
    }

    cout << "test_mat4_multiplication passed!" << endl;
}

int main() {
    test_vec4_perspective();
    test_mat4_multiplication();
    cout << "All tests passed!" << endl;
    return 0;
}
