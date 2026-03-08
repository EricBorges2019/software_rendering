#pragma once

#include "vertex.hpp"
#include "../core/framebuffer.hpp"
#include <functional>
#include <array>

namespace sr {
namespace pipeline {

struct Triangle {
    ClipVertex v[3];
};

// Per-fragment data passed to the fragment stage
struct Fragment {
    int x, y;
    float depth;
    math::Vec3 worldPos;
    math::Vec3 normal;
    math::Vec2 uv;
    math::Vec3 color;

    // Perspective-correct barycentric coords
    float w0, w1, w2;
};

using FragmentCallback = std::function<math::Color(const Fragment&)>;

class Rasterizer {
public:
    explicit Rasterizer(core::Framebuffer& fb) : fb_(fb) {}

    // Rasterize a single triangle with a given fragment callback
    void rasterize(const Triangle& tri, const FragmentCallback& frag);

    // Rasterize a batch of triangles (optionally multithreaded)
    void rasterizeBatch(const Triangle* tris, int count, const FragmentCallback& frag);

private:
    core::Framebuffer& fb_;

    // Clip triangle against near plane, emit 1 or 2 result triangles
    int clipNear(const Triangle& in, Triangle out[2]) const;

    // Core inner-loop rasterizer for a single NDC triangle, restricted to rows [tileY0, tileY1)
    void rasterizeNDC(const Triangle& tri, const FragmentCallback& frag, int tileY0, int tileY1);

    // Convert clip-space vertex to screen-space
    math::Vec3 toScreen(const math::Vec4& clip) const;

    // Edge function: positive if p is on the left of (a->b)
    static float edgeFunction(const math::Vec3& a, const math::Vec3& b, const math::Vec3& p) {
        return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
    }
};

}
}
