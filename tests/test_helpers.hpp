#pragma once

// =============================================================================
// test_helpers.hpp — Shared test utilities and tolerance constants
//
// All floating-point comparisons use these two tiers:
//   EPS       (1e-4) — tight: single-operation results, exact math identities
//   EPS_LOOSE (1e-2) — loose: chained transforms, accumulated floating-point error
// =============================================================================

#include <cmath>
#include <cstdint>
#include "soft_render/math/vec3.hpp"
#include "soft_render/core/framebuffer.hpp"

namespace sr_test {

using sr::math::Vec3;
using sr::core::Framebuffer;
using sr::core::Pixel;

// ---------------------------------------------------------------------------
// Tolerance constants
// ---------------------------------------------------------------------------
constexpr float EPS       = 1e-4f;   // Single-operation precision
constexpr float EPS_LOOSE = 1e-2f;   // Multi-operation / chained transforms
constexpr float PI        = 3.14159265358979f;

// ---------------------------------------------------------------------------
// Floating-point comparison helpers
// ---------------------------------------------------------------------------
inline bool approx(float a, float b, float eps = EPS) {
    return std::abs(a - b) < eps;
}

inline bool approxVec3(const Vec3& a, const Vec3& b, float eps = EPS) {
    return approx(a.x, b.x, eps) && approx(a.y, b.y, eps) && approx(a.z, b.z, eps);
}

// ---------------------------------------------------------------------------
// Deterministic PRNG (xorshift32) — reproducible but not hardcodeable
// ---------------------------------------------------------------------------
inline uint32_t& rng_state() {
    static uint32_t s = 0xDEADBEEF;
    return s;
}

inline float randf() {
    auto& s = rng_state();
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return (s & 0xFFFFFF) / float(0xFFFFFF);
}

inline float randf_range(float lo, float hi) { return lo + randf() * (hi - lo); }

inline Vec3 randVec3(float range = 10.f) {
    return { randf_range(-range, range), randf_range(-range, range), randf_range(-range, range) };
}

inline Vec3 randUnitVec3() { return randVec3().normalized(); }

// ---------------------------------------------------------------------------
// Framebuffer inspection helpers
// ---------------------------------------------------------------------------
inline int countNonBlack(const Framebuffer& fb) {
    int count = 0;
    const Pixel* px = fb.pixels();
    for (int i = 0; i < fb.width() * fb.height(); ++i)
        if (px[i].r > 0 || px[i].g > 0 || px[i].b > 0) ++count;
    return count;
}

inline Pixel getPixel(const Framebuffer& fb, int x, int y) {
    return fb.pixels()[y * fb.width() + x];
}

inline bool isLit(const Framebuffer& fb, int x, int y) {
    Pixel p = getPixel(fb, x, y);
    return p.r > 0 || p.g > 0 || p.b > 0;
}

} // namespace sr_test
