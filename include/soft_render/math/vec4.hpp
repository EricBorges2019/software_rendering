#pragma once

#include "vec3.hpp"
#include <cmath>

#ifdef USE_SSE4_1
#include <smmintrin.h>
#endif

namespace sr {
namespace math {

struct Vec4 {
    union {
        struct { float x, y, z, w; };
        float data[4];
#ifdef USE_SSE4_1
        __m128 simd;
#endif
    };

    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    explicit Vec4(float v) : x(v), y(v), z(v), w(v) {}
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    float& operator[](int i) { return data[i]; }
    const float& operator[](int i) const { return data[i]; }

    Vec4 operator+(const Vec4& v) const {
#ifdef USE_SSE4_1
        return Vec4{_mm_add_ps(simd, v.simd)};
#else
        return { x + v.x, y + v.y, z + v.z, w + v.w };
#endif
    }

    Vec4 operator-(const Vec4& v) const {
#ifdef USE_SSE4_1
        return Vec4{_mm_sub_ps(simd, v.simd)};
#else
        return { x - v.x, y - v.y, z - v.z, w - v.w };
#endif
    }

    Vec4 operator*(float s) const {
#ifdef USE_SSE4_1
        return Vec4{_mm_mul_ps(simd, _mm_set1_ps(s))};
#else
        return { x * s, y * s, z * s, w * s };
#endif
    }

    Vec4 operator*(const Vec4& v) const {
#ifdef USE_SSE4_1
        return Vec4{_mm_mul_ps(simd, v.simd)};
#else
        return { x * v.x, y * v.y, z * v.z, w * v.w };
#endif
    }

    float dot(const Vec4& v) const {
#ifdef USE_SSE4_1
        __m128 dp = _mm_dp_ps(simd, v.simd, 0xF1);
        return _mm_cvtss_f32(dp);
#else
        return x * v.x + y * v.y + z * v.z + w * v.w;
#endif
    }

    Vec3 xyz() const { return { x, y, z }; }
    Vec3 perspective() const { float invW = 1.0f / w; return { x * invW, y * invW, z * invW }; }

#ifdef USE_SSE4_1
    explicit Vec4(__m128 m) : simd(m) {}
#endif
};

inline Vec4 operator*(float s, const Vec4& v) { return v * s; }

}
}