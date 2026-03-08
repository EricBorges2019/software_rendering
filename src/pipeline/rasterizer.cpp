#include "soft_render/pipeline/rasterizer.hpp"
#include <cmath>
#include <algorithm>
#include <thread>
#include <vector>
#include <atomic>

namespace sr {
namespace pipeline {

// ----------------------------------------------------------------
// Helpers
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

// ----------------------------------------------------------------
// Near-plane clipping (keep z + w > 0)
// ----------------------------------------------------------------
int Rasterizer::clipNear(const Triangle& in, Triangle out[2]) const {
    float d[3];
    for (int i = 0; i < 3; ++i)
        d[i] = in.v[i].clipPos.z + in.v[i].clipPos.w;

    int inside = 0;
    for (int i = 0; i < 3; ++i) if (d[i] >= 0) ++inside;

    if (inside == 3) { out[0] = in; return 1; }
    if (inside == 0) return 0;

    const ClipVertex* ins[3];  int ni = 0;
    const ClipVertex* outs[3]; int no = 0;
    for (int i = 0; i < 3; ++i) {
        if (d[i] >= 0) ins[ni++]  = &in.v[i];
        else           outs[no++] = &in.v[i];
    }

    auto idx = [&](const ClipVertex* p) -> int { return (int)(p - in.v); };

    if (inside == 1) {
        float ta = d[idx(ins[0])] / (d[idx(ins[0])] - d[idx(outs[0])]);
        float tb = d[idx(ins[0])] / (d[idx(ins[0])] - d[idx(outs[1])]);
        out[0].v[0] = *ins[0];
        out[0].v[1] = lerpVertex(*ins[0], *outs[0], ta);
        out[0].v[2] = lerpVertex(*ins[0], *outs[1], tb);
        return 1;
    } else {
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
// NDC -> screen
// ----------------------------------------------------------------
math::Vec3 Rasterizer::toScreen(const math::Vec4& clip) const {
    float invW = 1.0f / clip.w;
    return {
        (clip.x * invW * 0.5f + 0.5f) * (fb_.width()  - 1),
        (clip.y * invW * 0.5f + 0.5f) * (fb_.height() - 1),
         clip.z * invW
    };
}

// ----------------------------------------------------------------
// Core rasterizer — optionally restricted to a Y tile [tileY0, tileY1)
// ----------------------------------------------------------------
void Rasterizer::rasterizeNDC(const Triangle& tri, const FragmentCallback& frag,
                               int tileY0, int tileY1) {
    const int W = fb_.width();
    const int H = fb_.height();

    float iw0 = 1.0f / tri.v[0].clipPos.w;
    float iw1 = 1.0f / tri.v[1].clipPos.w;
    float iw2 = 1.0f / tri.v[2].clipPos.w;

    math::Vec3 s0 = toScreen(tri.v[0].clipPos);
    math::Vec3 s1 = toScreen(tri.v[1].clipPos);
    math::Vec3 s2 = toScreen(tri.v[2].clipPos);

    int minX = std::max(0,       (int)std::floor(std::min({s0.x, s1.x, s2.x})));
    int maxX = std::min(W - 1,   (int)std::ceil (std::max({s0.x, s1.x, s2.x})));
    int minY = std::max(tileY0,  (int)std::floor(std::min({s0.y, s1.y, s2.y})));
    int maxY = std::min(tileY1 - 1, (int)std::ceil(std::max({s0.y, s1.y, s2.y})));

    if (minX > maxX || minY > maxY) return;

    float area = edgeFunction(s0, s1, s2);
    if (std::abs(area) < 1e-8f) return;
    bool ccw = area > 0;
    float invArea = 1.0f / area;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            math::Vec3 p = { x + 0.5f, y + 0.5f, 0 };
            float e0 = edgeFunction(s1, s2, p);
            float e1 = edgeFunction(s2, s0, p);
            float e2 = edgeFunction(s0, s1, p);

            if (ccw) { if (e0 < 0 || e1 < 0 || e2 < 0) continue; }
            else      { if (e0 > 0 || e1 > 0 || e2 > 0) continue; }

            float b0 = e0 * invArea;
            float b1 = e1 * invArea;
            float b2 = e2 * invArea;

            float wInterp    = b0 * iw0 + b1 * iw1 + b2 * iw2;
            float invWInterp = 1.0f / wInterp;
            float depth      = b0 * s0.z + b1 * s1.z + b2 * s2.z;

            if (!fb_.depthTest(x, y, depth)) continue;

            auto pclerp3 = [&](const math::Vec3& a, const math::Vec3& b, const math::Vec3& c) {
                return (a * (b0 * iw0) + b * (b1 * iw1) + c * (b2 * iw2)) * invWInterp;
            };
            auto pclerp2 = [&](const math::Vec2& a, const math::Vec2& b, const math::Vec2& c) {
                math::Vec2 r;
                r.x = (a.x*(b0*iw0) + b.x*(b1*iw1) + c.x*(b2*iw2)) * invWInterp;
                r.y = (a.y*(b0*iw0) + b.y*(b1*iw1) + c.y*(b2*iw2)) * invWInterp;
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

            fb_.setPixel(x, y, frag(fg));
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
        rasterizeNDC(clipped[i], frag, 0, fb_.height());
}

void Rasterizer::rasterizeBatch(const Triangle* tris, int count,
                                 const FragmentCallback& frag) {
#ifdef USE_THREADS
    // Tile-based dispatch: each thread owns exclusive horizontal bands.
    // No locks needed — threads never write to the same pixels.
    int nThreads = (int)std::thread::hardware_concurrency();
    if (nThreads <= 0) nThreads = 4;

    const int H = fb_.height();
    const int bandH = (H + nThreads - 1) / nThreads;

    std::vector<std::thread> threads;
    threads.reserve(nThreads);

    for (int t = 0; t < nThreads; ++t) {
        int y0 = t * bandH;
        int y1 = std::min(y0 + bandH, H);
        if (y0 >= H) break;

        threads.emplace_back([&, y0, y1]() {
            Triangle clipped[2];
            for (int i = 0; i < count; ++i) {
                int n = clipNear(tris[i], clipped);
                for (int k = 0; k < n; ++k)
                    rasterizeNDC(clipped[k], frag, y0, y1);
            }
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
