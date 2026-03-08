#pragma once

#include "vec3.hpp"
#include "vec4.hpp"
#include <cmath>
#include <cstring>

namespace sr {
namespace math {

struct Mat4 {
    union {
        float m[16];
        float data[4][4];
#ifdef USE_SSE4_1
        __m128 rows[4];
#endif
    };

    Mat4() {
        std::memset(m, 0, sizeof(m));
    }

    Mat4(std::initializer_list<float> list) {
        int i = 0;
        for (float v : list) {
            if (i < 16) m[i++] = v;
        }
    }

    static Mat4 identity() {
        return {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
    }

    float& operator()(int row, int col) { return data[row][col]; }
    const float& operator()(int row, int col) const { return data[row][col]; }

    Vec4 row(int i) const { return { data[i][0], data[i][1], data[i][2], data[i][3] }; }
    Vec4 col(int i) const { return { data[0][i], data[1][i], data[2][i], data[3][i] }; }

    Mat4 operator*(const Mat4& b) const {
        Mat4 result;
#ifdef USE_SSE4_1
        for (int i = 0; i < 4; ++i) {
            __m128 r = rows[i];
            result.rows[i] = _mm_add_ps(_mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(r, r, _MM_SHUFFLE(0,0,0,0)), b.rows[0]),
                _mm_mul_ps(_mm_shuffle_ps(r, r, _MM_SHUFFLE(1,1,1,1)), b.rows[1])),
                _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(r, r, _MM_SHUFFLE(2,2,2,2)), b.rows[2]),
                _mm_mul_ps(_mm_shuffle_ps(r, r, _MM_SHUFFLE(3,3,3,3)), b.rows[3])));
        }
#else
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.data[i][j] = data[i][0] * b.data[0][j] +
                                     data[i][1] * b.data[1][j] +
                                     data[i][2] * b.data[2][j] +
                                     data[i][3] * b.data[3][j];
            }
        }
#endif
        return result;
    }

    Vec4 operator*(const Vec4& v) const {
#ifdef USE_SSE4_1
        Vec4 result;
        for (int i = 0; i < 4; ++i) {
            __m128 dp = _mm_dp_ps(rows[i], v.simd, 0xF1);
            result.data[i] = _mm_cvtss_f32(dp);
        }
        return result;
#else
        return {
            data[0][0] * v.x + data[0][1] * v.y + data[0][2] * v.z + data[0][3] * v.w,
            data[1][0] * v.x + data[1][1] * v.y + data[1][2] * v.z + data[1][3] * v.w,
            data[2][0] * v.x + data[2][1] * v.y + data[2][2] * v.z + data[2][3] * v.w,
            data[3][0] * v.x + data[3][1] * v.y + data[3][2] * v.z + data[3][3] * v.w
        };
#endif
    }

    Mat4 transposed() const {
        return {
            m[0], m[4], m[8], m[12],
            m[1], m[5], m[9], m[13],
            m[2], m[6], m[10], m[14],
            m[3], m[7], m[11], m[15]
        };
    }

    static Mat4 translation(const Vec3& v) {
        return {
            1, 0, 0, v.x,
            0, 1, 0, v.y,
            0, 0, 1, v.z,
            0, 0, 0, 1
        };
    }

    static Mat4 scale(const Vec3& v) {
        return {
            v.x, 0, 0, 0,
            0, v.y, 0, 0,
            0, 0, v.z, 0,
            0, 0, 0, 1
        };
    }

    static Mat4 rotationX(float angle) {
        float c = std::cos(angle), s = std::sin(angle);
        return {
            1, 0, 0, 0,
            0, c, -s, 0,
            0, s, c, 0,
            0, 0, 0, 1
        };
    }

    static Mat4 rotationY(float angle) {
        float c = std::cos(angle), s = std::sin(angle);
        return {
            c, 0, s, 0,
            0, 1, 0, 0,
            -s, 0, c, 0,
            0, 0, 0, 1
        };
    }

    static Mat4 rotationZ(float angle) {
        float c = std::cos(angle), s = std::sin(angle);
        return {
            c, -s, 0, 0,
            s, c, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        // f = forward (into scene), r = right, u = up
        // View matrix rows are [r, u, -f] with translation -dot(basis, eye)
        Vec3 f = (target - eye).normalized();
        Vec3 r = f.cross(up).normalized();
        Vec3 u = r.cross(f);
        return {
            r.x,  r.y,  r.z,  -r.dot(eye),
            u.x,  u.y,  u.z,  -u.dot(eye),
            -f.x, -f.y, -f.z,  f.dot(eye),
            0,    0,    0,     1
        };
    }

    static Mat4 perspective(float fov, float aspect, float near, float far) {
        float tanHalfFov = std::tan(fov * 0.5f);
        // Standard OpenGL-style right-handed perspective:
        //   w_clip = -z_view  (positive when camera looks down -Z)
        //   z_clip = z_view * (-(near+far)/(far-near)) + (-2*far*near)/(far-near)
        float A = -(far + near) / (far - near);
        float B = -2.0f * far * near / (far - near);
        return {
            1.0f / (aspect * tanHalfFov), 0, 0, 0,
            0, 1.0f / tanHalfFov, 0, 0,
            0, 0, A, B,
            0, 0, -1, 0
        };
    }

    static Mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
        return {
            2 / (right - left), 0, 0, -(right + left) / (right - left),
            0, 2 / (top - bottom), 0, -(top + bottom) / (top - bottom),
            0, 0, -2 / (far - near), -(far + near) / (far - near),
            0, 0, 0, 1
        };
    }
};

}
}