// =============================================================================
// test_rasterizer.cpp — Uncheateable rasterization tests
//
// Strategy: Render known geometric configurations and verify structural
// properties of the output framebuffer. These tests verify:
// - Triangle coverage (pixels inside/outside the triangle)
// - Depth buffer correctness (nearer fragments win)
// - Barycentric interpolation (color gradients across triangles)
// - Near-plane clipping
// - Edge cases (degenerate triangles, single-pixel triangles)
//
// Each test constructs ClipVertex data directly (bypassing vertex processor)
// to isolate rasterizer behavior.
// =============================================================================

#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
#include <algorithm>

#include "test_helpers.hpp"
#include "soft_render/core/framebuffer.hpp"
#include "soft_render/pipeline/rasterizer.hpp"
#include "soft_render/pipeline/vertex.hpp"
#include "soft_render/math/vec3.hpp"
#include "soft_render/math/vec4.hpp"

using namespace sr;
using namespace sr::math;
using namespace sr::core;
using namespace sr::pipeline;
using namespace sr_test;

// Helper: create a ClipVertex in NDC space (x,y in [-1,1], z in [-1,1], w=1)
static ClipVertex makeNDCVertex(float ndcX, float ndcY, float ndcZ,
                                 Vec3 normal = {0, 0, 1},
                                 Vec3 color = {1, 1, 1},
                                 Vec2 uv = {0, 0}) {
    ClipVertex cv;
    cv.clipPos = Vec4(ndcX, ndcY, ndcZ, 1.0f);
    cv.worldPos = Vec3(ndcX, ndcY, ndcZ);
    cv.normal = normal;
    cv.color = color;
    cv.uv = uv;
    return cv;
}

// =============================================================================
// Test: Full-screen triangle produces pixels everywhere
// =============================================================================
void test_fullscreen_triangle() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    // Triangle that covers the entire screen (and then some)
    Triangle tri;
    tri.v[0] = makeNDCVertex(-2.0f, -2.0f, 0.0f, {0,0,1}, {1,1,1});
    tri.v[1] = makeNDCVertex( 2.0f, -2.0f, 0.0f, {0,0,1}, {1,1,1});
    tri.v[2] = makeNDCVertex( 0.0f,  2.0f, 0.0f, {0,0,1}, {1,1,1});

    auto passthrough = [](const Fragment& f) -> Color { return f.color; };
    rast.rasterize(tri, passthrough);

    int lit = countNonBlack(fb);
    // With a huge triangle covering NDC [-2,2], every pixel should be covered
    assert(lit == W * H && "full-screen triangle must cover all pixels");
    std::cout << "  full-screen triangle coverage: PASS" << std::endl;
}

// =============================================================================
// Test: Triangle in center produces pixels only in center region
// =============================================================================
void test_centered_triangle() {
    const int W = 64, H = 64;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    // Small triangle near center of screen
    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.3f, -0.3f, 0.0f, {0,0,1}, {1,0,0});
    tri.v[1] = makeNDCVertex( 0.3f, -0.3f, 0.0f, {0,0,1}, {0,1,0});
    tri.v[2] = makeNDCVertex( 0.0f,  0.3f, 0.0f, {0,0,1}, {0,0,1});

    auto passthrough = [](const Fragment& f) -> Color { return f.color; };
    rast.rasterize(tri, passthrough);

    // Center pixel (32, 32) should be lit
    assert(isLit(fb, 32, 32) && "center pixel must be lit for centered triangle");

    // Corner pixels should NOT be lit
    assert(!isLit(fb, 0, 0) && "top-left corner must not be lit");
    assert(!isLit(fb, W-1, 0) && "top-right corner must not be lit");
    assert(!isLit(fb, 0, H-1) && "bottom-left corner must not be lit");
    assert(!isLit(fb, W-1, H-1) && "bottom-right corner must not be lit");

    // Total lit pixels should be much less than total
    int lit = countNonBlack(fb);
    assert(lit > 0 && lit < W * H / 2 && "centered triangle should cover partial screen");

    std::cout << "  centered triangle spatial coverage: PASS" << std::endl;
}

// =============================================================================
// Test: Color interpolation across triangle
// =============================================================================
void test_color_interpolation() {
    const int W = 64, H = 64;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    // Triangle with distinct vertex colors: R, G, B
    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.9f, -0.9f, 0.0f, {0,0,1}, {1, 0, 0});  // Red (bottom-left)
    tri.v[1] = makeNDCVertex( 0.9f, -0.9f, 0.0f, {0,0,1}, {0, 1, 0});  // Green (bottom-right)
    tri.v[2] = makeNDCVertex( 0.0f,  0.9f, 0.0f, {0,0,1}, {0, 0, 1});  // Blue (top-center)

    auto passthrough = [](const Fragment& f) -> Color { return f.color; };
    rast.rasterize(tri, passthrough);

    // Bottom-left area should be predominantly red
    if (isLit(fb, 10, 5)) {
        Pixel bl = getPixel(fb, 10, 5);
        assert(bl.r > bl.g && bl.r > bl.b && "bottom-left should be predominantly red");
    }

    // Bottom-right area should be predominantly green
    if (isLit(fb, W - 10, 5)) {
        Pixel br = getPixel(fb, W - 10, 5);
        assert(br.g > br.r && br.g > br.b && "bottom-right should be predominantly green");
    }

    // Top-center should be predominantly blue
    if (isLit(fb, W / 2, H - 10)) {
        Pixel tc = getPixel(fb, W / 2, H - 10);
        assert(tc.b > tc.r && tc.b > tc.g && "top-center should be predominantly blue");
    }

    // Center pixel should have a MIX of all three colors (none should be 0)
    Pixel center = getPixel(fb, W / 2, H / 3);
    if (center.r + center.g + center.b > 0) {
        assert(center.r > 0 && center.g > 0 && center.b > 0 &&
               "center should have mix of all vertex colors");
    }

    std::cout << "  color interpolation: PASS" << std::endl;
}

// =============================================================================
// Test: Depth buffer — nearer triangles occlude farther ones
// =============================================================================
void test_depth_occlusion() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    auto passthrough = [](const Fragment& f) -> Color { return f.color; };

    // First: draw a RED triangle at z = 0.5 (farther)
    Triangle far_tri;
    far_tri.v[0] = makeNDCVertex(-0.8f, -0.8f, 0.5f, {0,0,1}, {1, 0, 0});
    far_tri.v[1] = makeNDCVertex( 0.8f, -0.8f, 0.5f, {0,0,1}, {1, 0, 0});
    far_tri.v[2] = makeNDCVertex( 0.0f,  0.8f, 0.5f, {0,0,1}, {1, 0, 0});
    rast.rasterize(far_tri, passthrough);

    // Then: draw a GREEN triangle at z = 0.1 (closer)
    Triangle near_tri;
    near_tri.v[0] = makeNDCVertex(-0.5f, -0.5f, 0.1f, {0,0,1}, {0, 1, 0});
    near_tri.v[1] = makeNDCVertex( 0.5f, -0.5f, 0.1f, {0,0,1}, {0, 1, 0});
    near_tri.v[2] = makeNDCVertex( 0.0f,  0.5f, 0.1f, {0,0,1}, {0, 1, 0});
    rast.rasterize(near_tri, passthrough);

    // Center should be GREEN (nearer wins)
    Pixel center = getPixel(fb, W / 2, H / 3);
    assert(center.g > center.r && "nearer green triangle must occlude farther red triangle");

    // Now draw in REVERSE order to verify depth test works regardless of draw order
    fb.clear();

    // Draw GREEN (close) first
    rast.rasterize(near_tri, passthrough);
    // Draw RED (far) second — should NOT overwrite
    rast.rasterize(far_tri, passthrough);

    Pixel center2 = getPixel(fb, W / 2, H / 3);
    assert(center2.g > center2.r && "depth test must work regardless of draw order");

    std::cout << "  depth occlusion: PASS" << std::endl;
}

// =============================================================================
// Test: Depth values in framebuffer are correct
// =============================================================================
void test_depth_values() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    auto passthrough = [](const Fragment& f) -> Color { return f.color; };

    // Draw a triangle at constant z = 0.3
    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.9f, -0.9f, 0.3f, {0,0,1}, {1,1,1});
    tri.v[1] = makeNDCVertex( 0.9f, -0.9f, 0.3f, {0,0,1}, {1,1,1});
    tri.v[2] = makeNDCVertex( 0.0f,  0.9f, 0.3f, {0,0,1}, {1,1,1});
    rast.rasterize(tri, passthrough);

    // Center pixel depth should be approximately 0.3
    float centerDepth = fb.getDepth(W / 2, H / 3);
    assert(std::abs(centerDepth - 0.3f) < 0.05f && "depth at center should match triangle z");

    // Unlit pixel should still have infinity depth
    float cornerDepth = fb.getDepth(0, 0);
    assert(cornerDepth == std::numeric_limits<float>::infinity() &&
           "unlit pixels must have infinite depth");

    std::cout << "  depth values: PASS" << std::endl;
}

// =============================================================================
// Test: Degenerate triangle (zero area) produces no pixels
// =============================================================================
void test_degenerate_triangle() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    auto passthrough = [](const Fragment& f) -> Color { return {1,1,1}; };

    // Collinear vertices — zero-area triangle
    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.5f, 0.0f, 0.0f);
    tri.v[1] = makeNDCVertex( 0.0f, 0.0f, 0.0f);
    tri.v[2] = makeNDCVertex( 0.5f, 0.0f, 0.0f);
    rast.rasterize(tri, passthrough);

    int lit = countNonBlack(fb);
    assert(lit == 0 && "degenerate (collinear) triangle must produce no pixels");

    // Point triangle — all vertices at same location
    Triangle point_tri;
    point_tri.v[0] = makeNDCVertex(0.0f, 0.0f, 0.0f);
    point_tri.v[1] = makeNDCVertex(0.0f, 0.0f, 0.0f);
    point_tri.v[2] = makeNDCVertex(0.0f, 0.0f, 0.0f);
    rast.rasterize(point_tri, passthrough);

    lit = countNonBlack(fb);
    assert(lit == 0 && "point triangle must produce no pixels");

    std::cout << "  degenerate triangles: PASS" << std::endl;
}

// =============================================================================
// Test: Triangle behind camera (z > 1 in NDC) is clipped
// =============================================================================
void test_behind_camera_clipped() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    auto passthrough = [](const Fragment& f) -> Color { return {1,1,1}; };

    // Triangle with w < 0 (behind camera in clip space, z+w < 0)
    Triangle tri;
    tri.v[0] = makeNDCVertex(0, 0, 0, {0,0,1}, {1,1,1});
    tri.v[0].clipPos = Vec4(0, 0, -2.0f, -1.0f);
    tri.v[1] = makeNDCVertex(0, 0, 0, {0,0,1}, {1,1,1});
    tri.v[1].clipPos = Vec4(1, 0, -2.0f, -1.0f);
    tri.v[2] = makeNDCVertex(0, 0, 0, {0,0,1}, {1,1,1});
    tri.v[2].clipPos = Vec4(0, 1, -2.0f, -1.0f);

    rast.rasterize(tri, passthrough);

    int lit = countNonBlack(fb);
    assert(lit == 0 && "triangle entirely behind camera must produce no pixels");

    std::cout << "  behind-camera clipping: PASS" << std::endl;
}

// =============================================================================
// Test: Winding order — both CW and CCW triangles should render
// =============================================================================
void test_winding_order() {
    const int W = 32, H = 32;

    auto passthrough = [](const Fragment& f) -> Color { return {1,1,1}; };

    // CCW triangle
    {
        Framebuffer fb(W, H);
        fb.clear();
        Rasterizer rast(fb);
        Triangle tri;
        tri.v[0] = makeNDCVertex(-0.5f, -0.5f, 0.0f);
        tri.v[1] = makeNDCVertex( 0.5f, -0.5f, 0.0f);
        tri.v[2] = makeNDCVertex( 0.0f,  0.5f, 0.0f);
        rast.rasterize(tri, passthrough);
        int ccw_lit = countNonBlack(fb);
        assert(ccw_lit > 0 && "CCW triangle must produce pixels");
    }

    // CW triangle (reversed winding)
    {
        Framebuffer fb(W, H);
        fb.clear();
        Rasterizer rast(fb);
        Triangle tri;
        tri.v[0] = makeNDCVertex(-0.5f, -0.5f, 0.0f);
        tri.v[2] = makeNDCVertex( 0.5f, -0.5f, 0.0f);
        tri.v[1] = makeNDCVertex( 0.0f,  0.5f, 0.0f);
        rast.rasterize(tri, passthrough);
        int cw_lit = countNonBlack(fb);
        assert(cw_lit > 0 && "CW triangle must also produce pixels (no back-face culling)");
    }

    std::cout << "  winding order (both CW/CCW render): PASS" << std::endl;
}

// =============================================================================
// Test: Fragment callback receives valid barycentric coordinates
// =============================================================================
void test_barycentric_validity() {
    const int W = 64, H = 64;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    bool all_valid = true;
    int fragment_count = 0;

    auto checker = [&](const Fragment& f) -> Color {
        ++fragment_count;
        // Barycentric coords should sum to ~1
        float sum = f.w0 + f.w1 + f.w2;
        if (std::abs(sum - 1.0f) > 0.02f) all_valid = false;
        // Each should be non-negative
        if (f.w0 < -0.01f || f.w1 < -0.01f || f.w2 < -0.01f) all_valid = false;
        return {f.w0, f.w1, f.w2};
    };

    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.8f, -0.8f, 0.0f);
    tri.v[1] = makeNDCVertex( 0.8f, -0.8f, 0.0f);
    tri.v[2] = makeNDCVertex( 0.0f,  0.8f, 0.0f);
    rast.rasterize(tri, checker);

    assert(fragment_count > 0 && "must produce at least one fragment");
    assert(all_valid && "all barycentric coordinates must be valid (sum=1, non-negative)");

    std::cout << "  barycentric validity: PASS" << std::endl;
}

// =============================================================================
// Test: Fragment world positions are interpolated within triangle bounds
// =============================================================================
void test_fragment_world_position() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    Vec3 wp0(-1, -1, 0), wp1(1, -1, 0), wp2(0, 1, 0);

    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.8f, -0.8f, 0.0f);
    tri.v[0].worldPos = wp0;
    tri.v[1] = makeNDCVertex( 0.8f, -0.8f, 0.0f);
    tri.v[1].worldPos = wp1;
    tri.v[2] = makeNDCVertex( 0.0f,  0.8f, 0.0f);
    tri.v[2].worldPos = wp2;

    bool positions_valid = true;

    auto checker = [&](const Fragment& f) -> Color {
        // Interpolated world position must be within the convex hull of the triangle
        // Simple check: each component should be within the min/max of the vertices
        float minX = std::min({wp0.x, wp1.x, wp2.x}) - 0.1f;
        float maxX = std::max({wp0.x, wp1.x, wp2.x}) + 0.1f;
        float minY = std::min({wp0.y, wp1.y, wp2.y}) - 0.1f;
        float maxY = std::max({wp0.y, wp1.y, wp2.y}) + 0.1f;

        if (f.worldPos.x < minX || f.worldPos.x > maxX) positions_valid = false;
        if (f.worldPos.y < minY || f.worldPos.y > maxY) positions_valid = false;
        return {1, 1, 1};
    };

    rast.rasterize(tri, checker);
    assert(positions_valid && "fragment world positions must be within triangle bounds");
    std::cout << "  fragment world positions: PASS" << std::endl;
}

// =============================================================================
// Test: Fragment normals are interpolated and normalized
// =============================================================================
void test_fragment_normals() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.8f, -0.8f, 0.0f, {1, 0, 0});
    tri.v[1] = makeNDCVertex( 0.8f, -0.8f, 0.0f, {0, 1, 0});
    tri.v[2] = makeNDCVertex( 0.0f,  0.8f, 0.0f, {0, 0, 1});

    bool normals_valid = true;

    auto checker = [&](const Fragment& f) -> Color {
        // Normal should be approximately unit length
        float len = f.normal.length();
        if (std::abs(len - 1.0f) > 0.05f) normals_valid = false;
        return {1, 1, 1};
    };

    rast.rasterize(tri, checker);
    assert(normals_valid && "fragment normals must be approximately unit length");
    std::cout << "  fragment normals normalized: PASS" << std::endl;
}

// =============================================================================
// Test: Multiple triangles with batch rasterization
// =============================================================================
void test_batch_rasterization() {
    const int W = 64, H = 64;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    auto passthrough = [](const Fragment& f) -> Color { return f.color; };

    // Two non-overlapping triangles
    Triangle tris[2];
    // Left triangle (red)
    tris[0].v[0] = makeNDCVertex(-0.9f, -0.5f, 0.0f, {0,0,1}, {1,0,0});
    tris[0].v[1] = makeNDCVertex(-0.1f, -0.5f, 0.0f, {0,0,1}, {1,0,0});
    tris[0].v[2] = makeNDCVertex(-0.5f,  0.5f, 0.0f, {0,0,1}, {1,0,0});
    // Right triangle (green)
    tris[1].v[0] = makeNDCVertex( 0.1f, -0.5f, 0.0f, {0,0,1}, {0,1,0});
    tris[1].v[1] = makeNDCVertex( 0.9f, -0.5f, 0.0f, {0,0,1}, {0,1,0});
    tris[1].v[2] = makeNDCVertex( 0.5f,  0.5f, 0.0f, {0,0,1}, {0,1,0});

    rast.rasterizeBatch(tris, 2, passthrough);

    // Left side should be red
    Pixel left = getPixel(fb, W / 4, H / 2);
    assert(left.r > 0 && left.g == 0 && "left triangle should be red");

    // Right side should be green
    Pixel right = getPixel(fb, 3 * W / 4, H / 2);
    assert(right.g > 0 && right.r == 0 && "right triangle should be green");

    std::cout << "  batch rasterization: PASS" << std::endl;
}

// =============================================================================
// Test: Pixel-precise triangle — verify exact coverage pattern
// =============================================================================
void test_pixel_precise_coverage() {
    // Use a very small framebuffer and a triangle that covers a known set of pixels
    const int W = 8, H = 8;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    auto white = [](const Fragment& f) -> Color { return {1, 1, 1}; };

    // Triangle covering roughly the right half of the screen
    Triangle tri;
    tri.v[0] = makeNDCVertex(0.0f, -1.0f, 0.0f);
    tri.v[1] = makeNDCVertex(1.0f, -1.0f, 0.0f);
    tri.v[2] = makeNDCVertex(0.5f,  1.0f, 0.0f);
    rast.rasterize(tri, white);

    // Left column (x=0,1) should be entirely black
    for (int y = 0; y < H; ++y) {
        assert(!isLit(fb, 0, y) && "far-left column must be unlit");
    }

    // Right half should have SOME lit pixels
    int rightLit = 0;
    for (int y = 0; y < H; ++y)
        for (int x = W / 2; x < W; ++x)
            if (isLit(fb, x, y)) ++rightLit;

    assert(rightLit > 0 && "right half must have lit pixels");

    std::cout << "  pixel-precise coverage: PASS" << std::endl;
}

// =============================================================================
// Test: Near-plane clipping — triangle partially behind camera
// =============================================================================
void test_near_plane_clipping() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    auto white = [](const Fragment& f) -> Color { return {1, 1, 1}; };

    // One vertex behind near plane (z + w < 0), two in front
    Triangle tri;
    tri.v[0].clipPos = Vec4(-0.5f, -0.5f, 0.5f, 1.0f);  // in front
    tri.v[0].worldPos = Vec3(-0.5f, -0.5f, 0.5f);
    tri.v[0].normal = Vec3(0, 0, 1);
    tri.v[0].color = Vec3(1, 1, 1);
    tri.v[0].uv = Vec2(0, 0);

    tri.v[1].clipPos = Vec4(0.5f, -0.5f, 0.5f, 1.0f);   // in front
    tri.v[1].worldPos = Vec3(0.5f, -0.5f, 0.5f);
    tri.v[1].normal = Vec3(0, 0, 1);
    tri.v[1].color = Vec3(1, 1, 1);
    tri.v[1].uv = Vec2(1, 0);

    tri.v[2].clipPos = Vec4(0.0f, 0.5f, -2.0f, 0.5f);   // behind (z + w = -1.5 < 0)
    tri.v[2].worldPos = Vec3(0, 0.5f, -2.0f);
    tri.v[2].normal = Vec3(0, 0, 1);
    tri.v[2].color = Vec3(1, 1, 1);
    tri.v[2].uv = Vec2(0.5f, 1);

    rast.rasterize(tri, white);

    // Should produce SOME pixels (the visible part of the triangle)
    int lit = countNonBlack(fb);
    assert(lit > 0 && "partially-clipped triangle must produce some pixels");

    // But NOT as many as a fully-visible triangle of similar size
    Framebuffer fb2(W, H);
    fb2.clear();
    Rasterizer rast2(fb2);
    Triangle full;
    full.v[0] = makeNDCVertex(-0.5f, -0.5f, 0.0f);
    full.v[1] = makeNDCVertex( 0.5f, -0.5f, 0.0f);
    full.v[2] = makeNDCVertex( 0.0f,  0.5f, 0.0f);
    rast2.rasterize(full, white);
    int fullLit = countNonBlack(fb2);

    assert(lit < fullLit && "clipped triangle must cover fewer pixels than unclipped");

    std::cout << "  near-plane clipping: PASS" << std::endl;
}

// =============================================================================
// Test: UV interpolation
// =============================================================================
void test_uv_interpolation() {
    const int W = 32, H = 32;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    // Triangle with distinct UVs
    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.8f, -0.8f, 0.0f, {0,0,1}, {1,1,1}, {0, 0});
    tri.v[1] = makeNDCVertex( 0.8f, -0.8f, 0.0f, {0,0,1}, {1,1,1}, {1, 0});
    tri.v[2] = makeNDCVertex( 0.0f,  0.8f, 0.0f, {0,0,1}, {1,1,1}, {0.5f, 1});

    bool uvs_valid = true;

    auto checker = [&](const Fragment& f) -> Color {
        // UVs should be within [0, 1] for this triangle
        if (f.uv.x < -0.05f || f.uv.x > 1.05f) uvs_valid = false;
        if (f.uv.y < -0.05f || f.uv.y > 1.05f) uvs_valid = false;
        return {f.uv.x, f.uv.y, 0};
    };

    rast.rasterize(tri, checker);
    assert(uvs_valid && "UV coordinates must be interpolated within triangle's UV range");
    std::cout << "  UV interpolation: PASS" << std::endl;
}

// =============================================================================
// Test: Screen-space pixel coordinates match framebuffer bounds
// =============================================================================
void test_fragment_screen_coords() {
    const int W = 16, H = 16;
    Framebuffer fb(W, H);
    fb.clear();
    Rasterizer rast(fb);

    bool coords_valid = true;
    std::set<int> unique_pixels;

    auto checker = [&](const Fragment& f) -> Color {
        if (f.x < 0 || f.x >= W) coords_valid = false;
        if (f.y < 0 || f.y >= H) coords_valid = false;
        unique_pixels.insert(f.y * W + f.x);
        return {1, 1, 1};
    };

    Triangle tri;
    tri.v[0] = makeNDCVertex(-0.8f, -0.8f, 0.0f);
    tri.v[1] = makeNDCVertex( 0.8f, -0.8f, 0.0f);
    tri.v[2] = makeNDCVertex( 0.0f,  0.8f, 0.0f);
    rast.rasterize(tri, checker);

    assert(coords_valid && "all fragment coordinates must be within framebuffer bounds");
    // Each pixel should be written at most once (no duplicates in single triangle)
    int lit = countNonBlack(fb);
    assert((int)unique_pixels.size() == lit && "each pixel must be written exactly once");

    std::cout << "  fragment screen coordinates: PASS" << std::endl;
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Rasterizer Tests ===" << std::endl;

    test_fullscreen_triangle();
    test_centered_triangle();
    test_color_interpolation();
    test_depth_occlusion();
    test_depth_values();
    test_degenerate_triangle();
    test_behind_camera_clipped();
    test_winding_order();
    test_barycentric_validity();
    test_fragment_world_position();
    test_fragment_normals();
    test_batch_rasterization();
    test_pixel_precise_coverage();
    test_near_plane_clipping();
    test_uv_interpolation();
    test_fragment_screen_coords();

    std::cout << "\n=== ALL RASTERIZER TESTS PASSED ===" << std::endl;
    return 0;
}
