#pragma once

#include <cmath>
#include <algorithm>

#ifdef USE_SSE4_1
#include <smmintrin.h>
#endif

namespace sr {
namespace math {

struct Vec3 {
    union {
        struct { float x, y, z; };
        float data[3];
    };

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    explicit Vec3(float v) : x(v), y(v), z(v) {}

    float& operator[](int i) { return data[i]; }
    const float& operator[](int i) const { return data[i]; }

    Vec3 operator+(const Vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vec3 operator-(const Vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
    Vec3 operator*(const Vec3& v) const { return { x * v.x, y * v.y, z * v.z }; }
    Vec3 operator/(float s) const { float inv = 1.0f / s; return { x * inv, y * inv, z * inv }; }

    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float dot(const Vec3& v) const {
#ifdef USE_SSE4_1
        __m128 a = _mm_set_ps(0, z, y, x);
        __m128 b = _mm_set_ps(0, v.z, v.y, v.x);
        __m128 dp = _mm_dp_ps(a, b, 0x71);
        return _mm_cvtss_f32(dp);
#else
        return x * v.x + y * v.y + z * v.z;
#endif
    }

    Vec3 cross(const Vec3& v) const {
        return {
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        };
    }

    float lengthSq() const { return dot(*this); }
    float length() const { return std::sqrt(lengthSq()); }

    Vec3 normalized() const {
        float len = length();
        return len > 0 ? *this / len : Vec3(0);
    }

    Vec3 lerp(const Vec3& v, float t) const {
        return *this + (v - *this) * t;
    }

    static Vec3 min(const Vec3& a, const Vec3& b) {
        return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
    }

    static Vec3 max(const Vec3& a, const Vec3& b) {
        return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

using Color = Vec3;

}
}