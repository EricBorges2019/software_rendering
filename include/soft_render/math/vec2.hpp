#pragma once

#include <cmath>

namespace sr {
namespace math {

struct Vec2 {
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
    explicit Vec2(float v) : x(v), y(v) {}

    Vec2 operator+(const Vec2& v) const { return { x + v.x, y + v.y }; }
    Vec2 operator-(const Vec2& v) const { return { x - v.x, y - v.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
    Vec2 operator*(const Vec2& v) const { return { x * v.x, y * v.y }; }

    Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float dot(const Vec2& v) const { return x * v.x + y * v.y; }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const { float l = length(); return l > 0 ? *this * (1.f / l) : Vec2(0); }

    Vec2 lerp(const Vec2& v, float t) const { return *this + (v - *this) * t; }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

}
}
