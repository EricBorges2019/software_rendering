#include "soft_render/pipeline/rasterizer.hpp"
#include <cmath>
#include <algorithm>
#include <thread>
#include <vector>
#include <mutex>

namespace sr {
namespace pipeline {

// ----------------------------------------------------------------
// Near-plane clipping (clip z = -w)
// Returns 0, 1, or 2 output triangles.
// ----------------------------------------------------------------
static ClipVertex lerpVertex(const ClipVertex& a, const ClipVertex& b, float t) {
    ClipVertex out;
    out.clipPos  = a.clipPos  + (b.clipPos  - a.clipPos)  * t;
    out.worldPos = a.worldPos + (b.worldPos - a.worldPos) * t;
    out.normal   = (a.normal  + (b.normal   - a.normal)   * t).normalized();
    out.uv.x     = a.uv.x    + (b.uv.x     - a.uv.x)     * t;
    out.uv.y     = a.uv.y    + (b.uv.y     - a.uv.y)     * t;
    out.color    = a.color    + (b.color    - a.color)    * t;
    return out;
}

int Rasterizer::clipNear(const Triangle& in, Triangle out[2]) const {
    // Clip against near plane: z > -w  (keep z + w > 0)
    float d[3];
    for (int i = 0; i < 3; ++i)
        d[i] = in.v[i].clipPos.z + in.v[i].clipPos.w;

    int inside = 0;
    for (int i = 0; i < 3; ++i) if (d[i] >= 0) ++inside;

    if (inside == 3) {
        out[0] = in;
        return 1;
    }
    if (inside == 0) return 0;

    // Collect inside/outside
    const ClipVertex* ins[3]; int ni = 0;
    const ClipVertex* outs[3]; int no = 0;
    for (int i = 0; i < 3; ++i) {
        if (d[i] >= 0) ins[ni++]  = &in.v[i];
        else           outs[no++] = &in.v[i];
    }

    if (inside == 1) {
        // One vertex inside → one triangle
        float t0 = d[ins[0] - in.v] / (d[ins[0] - in.v] - (outs[0] - in.v >= 0 ? d[outs[0] - in.v] : d[outs[0] - in.v]));
        // Recompute correctly
        auto idx = [&](const ClipVertex* p) { return p - in.v; };
        float ta = d[idx(ins[0])] / (d[idx(ins[0])] - d[idx(outs[0])]);
        float tb = d[idx(ins[0])] / (d[idx(ins[0])] - d[idx(outs[1])]);
        out[0].v[0] = *ins[0];
        out[0].v[1] = lerpVertex(*ins[0], *outs[0], ta);
        out[0].v[2] = lerpVertex(*ins[0], *outs[1], tb);
        return 1;
    } else {
        // Two vertices inside → two triangles (quad)
        auto idx = [&](const ClipVertex* p) { return p - in.v; };
        float ta = d[idx(ins[0])] / (d[idx(ins[0])] - d[idx(outs[0])]);
        float tb = d[idx(ins[1])] / (d[idx(ins[1])] - d[idx(outs[0])]);
        ClipVertex ca = lerpVertex(*ins[0], *outs[0], ta);
        ClipVertex cb = lerpVertex(*ins[1], *outs[0], tb);
        out[0].v[0] = *ins[0];
        out[0].v[1] = *ins[1];
        out[0].v[2] = ca;
        out[1].v[0] = *ins[1];
        out[1].v[1] = cb;
        out[1].v[2] = ca;
        return 2;
    }
}

// ----------------------------------------------------------------
// NDC → screen-space
// ----------------------------------------------------------------
math::Vec3 Rasterizer::toScreen(const math::Vec4& clip) const {
    float invW = 1.0f / clip.w;
    float ndcX =  clip.x * invW;
    float ndcY =  clip.y * invW;
    float ndcZ =  clip.z * invW;
    float sx = (ndcX * 0.5f + 0.5f) * (fb_.width()  - 1);
    float sy = (ndcY * 0.5f + 0.5f) * (fb_.height() - 1);
    return { sx, sy, ndcZ };
}

// ----------------------------------------------------------------
// Core edge-function rasterizer
// ----------------------------------------------------------------
void Rasterizer::rasterizeNDC(const Triangle& tri, const FragmentCallback& frag) {
    const int W = fb_.width();
    const int H = fb_.height();

    // Perspective-correct: store 1/w per vertex
    float iw0 = 1.0f / tri.v[0].clipPos.w;
    float iw1 = 1.0f / tri.v[1].clipPos.w;
    float iw2 = 1.0f / tri.v[2].clipPos.w;

    math::Vec3 s0 = toScreen(tri.v[0].clipPos);
    math::Vec3 s1 = toScreen(tri.v[1].clipPos);
    math::Vec3 s2 = toScreen(tri.v[2].clipPos);

    // Bounding box (clamped to screen)
    int minX = std::max(0, (int)std::floor(std::min({s0.x, s1.x, s2.x})));
    int maxX = std::min(W - 1, (int)std::ceil(std::max({s0.x, s1.x, s2.x})));
    int minY = std::max(0, (int)std::floor(std::min({s0.y, s1.y, s2.y})));
    int maxY = std::min(H - 1, (int)std::ceil(std::max({s0.y, s1.y, s2.y})));

    if (minX > maxX || minY > maxY) return;

    float area = edgeFunction(s0, s1, s2);
    if (std::abs(area) < 1e-8f) return;
    // Back-face cull (CCW winding = front-face)
    if (area < 0) return;
    float invArea = 1.0f / area;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            math::Vec3 p = { x + 0.5f, y + 0.5f, 0 };
            float e0 = edgeFunction(s1, s2, p);
            float e1 = edgeFunction(s2, s0, p);
            float e2 = edgeFunction(s0, s1, p);

            if (e0 < 0 || e1 < 0 || e2 < 0) continue;

            float b0 = e0 * invArea;
            float b1 = e1 * invArea;
            float b2 = e2 * invArea;

            // Perspective-correct interpolation
            float wInterp = b0 * iw0 + b1 * iw1 + b2 * iw2;
            float invWInterp = 1.0f / wInterp;

            // Depth
            float depth = b0 * s0.z + b1 * s1.z + b2 * s2.z;

            if (!fb_.depthTest(x, y, depth)) continue;

            // Perspective-correct attribute interp
            auto pclerp3 = [&](const math::Vec3& a, const math::Vec3& b, const math::Vec3& c) {
                return (a * (b0 * iw0) + b * (b1 * iw1) + c * (b2 * iw2)) * invWInterp;
            };
            auto pclerp2 = [&](const math::Vec2& a, const math::Vec2& b, const math::Vec2& c) {
                math::Vec2 r;
                r.x = (a.x * b0 * iw0 + b.x * b1 * iw1 + c.x * b2 * iw2) * invWInterp;
                r.y = (a.y * b0 * iw0 + b.y * b1 * iw1 + c.y * b2 * iw2) * invWInterp;
                return r;
            };

            Fragment fg;
            fg.x        = x; fg.y = y;
            fg.depth    = depth;
            fg.worldPos = pclerp3(tri.v[0].worldPos, tri.v[1].worldPos, tri.v[2].worldPos);
            fg.normal   = pclerp3(tri.v[0].normal,   tri.v[1].normal,   tri.v[2].normal).normalized();
            fg.uv       = pclerp2(tri.v[0].uv,       tri.v[1].uv,       tri.v[2].uv);
            fg.color    = pclerp3(tri.v[0].color,     tri.v[1].color,    tri.v[2].color);
            fg.w0 = b0; fg.w1 = b1; fg.w2 = b2;

            math::Color color = frag(fg);
            fb_.setPixel(x, y, color);
        }
    }
}

// ----------------------------------------------------------------
// Public API
// ----------------------------------------------------------------
void Rasterizer::rasterize(const Triangle& tri, const FragmentCallback& frag) {
    Triangle clipped[2];
    int n = clipNear(tri, clipped);
    for (int i = 0; i < n; ++i)
        rasterizeNDC(clipped[i], frag);
}

void Rasterizer::rasterizeBatch(const Triangle* tris, int count, const FragmentCallback& frag) {
#ifdef USE_THREADS
    // Divide triangles evenly across logical cores
    int nCores = (int)std::thread::hardware_concurrency();
    if (nCores <= 0) nCores = 4; // fallback
    std::vector<std::thread> threads;
    threads.reserve(nCores);
    int chunk = (count + nCores - 1) / nCores;
    for (int t = 0; t < nCores; ++t) {
        int start = t * chunk;
        int end   = std::min(start + chunk, count);
        if (start >= end) break;
        threads.emplace_back([&, start, end]() {
            for (int i = start; i < end; ++i)
                rasterize(tris[i], frag);
        });
    }
    for (auto& th : threads) th.join();
#else
    for (int i = 0; i < count; ++i)
        rasterize(tris[i], frag);
#endif
}

}
}
