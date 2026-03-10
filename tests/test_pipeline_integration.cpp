// =============================================================================
// test_pipeline_integration.cpp — Full pipeline integration tests
//
// Strategy: Test the vertex processor, fragment shader, and renderer as
// integrated units. These tests verify that the full pipeline produces
// correct rendering output — something that cannot be faked without
// implementing the actual graphics pipeline.
// =============================================================================

#include <cassert>
#include <cmath>
#include <iostream>
#include <cstdint>
#include <limits>
#include <vector>

#include "soft_render/math/vec2.hpp"
#include "soft_render/math/vec3.hpp"
#include "soft_render/math/vec4.hpp"
#include "soft_render/math/mat4.hpp"
#include "soft_render/core/framebuffer.hpp"
#include "soft_render/core/texture.hpp"
#include "soft_render/pipeline/vertex.hpp"
#include "soft_render/pipeline/vertex_processor.hpp"
#include "soft_render/pipeline/rasterizer.hpp"
#include "soft_render/pipeline/fragment_shader.hpp"
#include "soft_render/render/renderer.hpp"

using namespace sr;
using namespace sr::math;
using namespace sr::core;
using namespace sr::pipeline;
using namespace sr::render;

static const float PI = 3.14159265358979f;
static const float EPS = 1e-4f;

static bool approx(float a, float b, float eps = EPS) { return std::abs(a - b) < eps; }

// Helper: count non-black pixels
static int countNonBlack(const Framebuffer& fb) {
    int count = 0;
    const Pixel* px = fb.pixels();
    for (int i = 0; i < fb.width() * fb.height(); ++i)
        if (px[i].r > 0 || px[i].g > 0 || px[i].b > 0) ++count;
    return count;
}

static Pixel getPixel(const Framebuffer& fb, int x, int y) {
    return fb.pixels()[y * fb.width() + x];
}

static bool isLit(const Framebuffer& fb, int x, int y) {
    Pixel p = getPixel(fb, x, y);
    return p.r > 0 || p.g > 0 || p.b > 0;
}

// =============================================================================
// Vertex Processor Tests
// =============================================================================

void test_vp_identity_transform() {
    // With identity MVP, clipPos should equal Vec4(position, 1)
    VertexProcessor vp;
    Uniforms u;
    u.model = Mat4::identity();
    u.view = Mat4::identity();
    u.projection = Mat4::identity();
    u.normalMatrix = Mat4::identity();

    Vertex v;
    v.position = Vec3(1, 2, 3);
    v.normal = Vec3(0, 1, 0);
    v.uv = Vec2(0.5f, 0.5f);
    v.color = Vec3(1, 0, 0);

    ClipVertex cv = vp.process(v, u);

    assert(approx(cv.clipPos.x, 1.0f) && "identity: clipPos.x must match position.x");
    assert(approx(cv.clipPos.y, 2.0f) && "identity: clipPos.y must match position.y");
    assert(approx(cv.clipPos.z, 3.0f) && "identity: clipPos.z must match position.z");
    assert(approx(cv.clipPos.w, 1.0f) && "identity: clipPos.w must be 1");
    assert(approx(cv.worldPos.x, 1.0f) && "identity: worldPos must match position");
    assert(approx(cv.normal.y, 1.0f) && "identity: normal must be preserved");
    assert(approx(cv.uv.x, 0.5f) && "UV must be passed through");
    assert(approx(cv.color.x, 1.0f) && "color must be passed through");

    std::cout << "  VP identity transform: PASS" << std::endl;
}

void test_vp_translation() {
    VertexProcessor vp;
    Uniforms u;
    u.model = Mat4::translation(Vec3(10, 20, 30));
    u.view = Mat4::identity();
    u.projection = Mat4::identity();
    u.normalMatrix = Mat4::identity();

    Vertex v;
    v.position = Vec3(1, 2, 3);
    v.normal = Vec3(0, 1, 0);

    ClipVertex cv = vp.process(v, u);
    assert(approx(cv.worldPos.x, 11.0f) && "translation must offset x");
    assert(approx(cv.worldPos.y, 22.0f) && "translation must offset y");
    assert(approx(cv.worldPos.z, 33.0f) && "translation must offset z");

    std::cout << "  VP translation: PASS" << std::endl;
}

void test_vp_batch_matches_single() {
    // INVARIANT: processBatch must produce identical results to individual process calls
    VertexProcessor vp;
    Uniforms u;
    u.model = Mat4::rotationY(0.7f) * Mat4::translation(Vec3(1, 2, 3));
    u.view = Mat4::lookAt(Vec3(0, 0, 5), Vec3(0, 0, 0), Vec3(0, 1, 0));
    u.projection = Mat4::perspective(PI / 4, 1.0f, 0.1f, 100.0f);
    u.normalMatrix = Mat4::identity();

    const int N = 10;
    Vertex verts[N];
    for (int i = 0; i < N; ++i) {
        verts[i].position = Vec3(i * 0.1f, i * 0.2f, i * 0.3f);
        verts[i].normal = Vec3(0, 1, 0);
        verts[i].uv = Vec2(i * 0.1f, 0);
        verts[i].color = Vec3(1, 1, 1);
    }

    // Single processing
    ClipVertex singles[N];
    for (int i = 0; i < N; ++i)
        singles[i] = vp.process(verts[i], u);

    // Batch processing
    ClipVertex batch[N];
    vp.processBatch(verts, batch, N, u);

    for (int i = 0; i < N; ++i) {
        assert(approx(singles[i].clipPos.x, batch[i].clipPos.x, 0.01f) &&
               "batch clipPos.x must match single");
        assert(approx(singles[i].clipPos.y, batch[i].clipPos.y, 0.01f) &&
               "batch clipPos.y must match single");
        assert(approx(singles[i].clipPos.z, batch[i].clipPos.z, 0.01f) &&
               "batch clipPos.z must match single");
        assert(approx(singles[i].clipPos.w, batch[i].clipPos.w, 0.01f) &&
               "batch clipPos.w must match single");
        assert(approx(singles[i].worldPos.x, batch[i].worldPos.x, 0.01f) &&
               "batch worldPos must match single");
    }

    std::cout << "  VP batch matches single: PASS" << std::endl;
}

void test_vp_rotation_preserves_distance() {
    // INVARIANT: Rotation should not change distance from origin (in world space)
    VertexProcessor vp;
    Uniforms u;
    u.model = Mat4::rotationY(1.23f) * Mat4::rotationX(0.45f);
    u.view = Mat4::identity();
    u.projection = Mat4::identity();
    u.normalMatrix = u.model;  // For pure rotation, normal matrix = model

    Vertex v;
    v.position = Vec3(3, 4, 5);
    v.normal = Vec3(0, 1, 0);

    ClipVertex cv = vp.process(v, u);
    float origDist = v.position.length();
    float worldDist = cv.worldPos.length();

    assert(approx(origDist, worldDist, 0.01f) && "rotation must preserve distance from origin");
    std::cout << "  VP rotation preserves distance: PASS" << std::endl;
}

// =============================================================================
// Fragment Shader Tests
// =============================================================================

void test_fs_unlit_returns_albedo() {
    FragmentShader fs;
    Material mat;
    mat.albedo = Color(0.5f, 0.3f, 0.8f);

    auto cb = fs.buildUnlit(mat);

    Fragment frag;
    frag.color = Vec3(1, 1, 1);
    frag.normal = Vec3(0, 0, 1);
    frag.uv = Vec2(0, 0);

    Color result = cb(frag);
    // Unlit should return albedo * frag.color, clamped
    assert(approx(result.x, 0.5f, 0.02f) && "unlit red must match albedo");
    assert(approx(result.y, 0.3f, 0.02f) && "unlit green must match albedo");
    assert(approx(result.z, 0.8f, 0.02f) && "unlit blue must match albedo");

    std::cout << "  FS unlit returns albedo: PASS" << std::endl;
}

void test_fs_lit_brighter_facing_light() {
    // INVARIANT: A fragment facing a light should be brighter than one facing away
    FragmentShader fs;
    Material mat;
    mat.albedo = Color(1, 1, 1);
    mat.ambient = 0.05f;
    mat.diffuse = 0.9f;
    mat.specular = 0.0f;

    SceneLighting lighting;
    lighting.cameraPos = Vec3(0, 0, 5);
    lighting.ambientColor = Color(0.1f, 0.1f, 0.1f);
    lighting.lights.push_back({{0, 0, 10}, {1,1,1}, 5.0f, 100.0f});

    auto cb = fs.build(mat, lighting);

    // Fragment facing light (normal toward +Z)
    Fragment facingLight;
    facingLight.worldPos = Vec3(0, 0, 0);
    facingLight.normal = Vec3(0, 0, 1);
    facingLight.color = Vec3(1, 1, 1);
    facingLight.uv = Vec2(0, 0);

    // Fragment facing away (normal toward -Z)
    Fragment facingAway;
    facingAway.worldPos = Vec3(0, 0, 0);
    facingAway.normal = Vec3(0, 0, -1);
    facingAway.color = Vec3(1, 1, 1);
    facingAway.uv = Vec2(0, 0);

    Color brightResult = cb(facingLight);
    Color darkResult = cb(facingAway);

    float brightLum = brightResult.x + brightResult.y + brightResult.z;
    float darkLum = darkResult.x + darkResult.y + darkResult.z;

    assert(brightLum > darkLum && "fragment facing light must be brighter than one facing away");

    std::cout << "  FS lit vs unlit brightness: PASS" << std::endl;
}

void test_fs_light_attenuation() {
    // INVARIANT: Closer light should produce brighter result
    FragmentShader fs;
    Material mat;
    mat.albedo = Color(1, 1, 1);
    mat.ambient = 0.0f;
    mat.diffuse = 1.0f;
    mat.specular = 0.0f;

    // Close light
    SceneLighting nearLighting;
    nearLighting.cameraPos = Vec3(0, 0, 5);
    nearLighting.ambientColor = Color(0, 0, 0);
    nearLighting.lights.push_back({{0, 0, 2}, {1,1,1}, 1.0f, 50.0f});

    // Far light
    SceneLighting farLighting;
    farLighting.cameraPos = Vec3(0, 0, 5);
    farLighting.ambientColor = Color(0, 0, 0);
    farLighting.lights.push_back({{0, 0, 20}, {1,1,1}, 1.0f, 50.0f});

    auto nearCb = fs.build(mat, nearLighting);
    auto farCb = fs.build(mat, farLighting);

    Fragment frag;
    frag.worldPos = Vec3(0, 0, 0);
    frag.normal = Vec3(0, 0, 1);
    frag.color = Vec3(1, 1, 1);
    frag.uv = Vec2(0, 0);

    Color nearResult = nearCb(frag);
    Color farResult = farCb(frag);

    float nearLum = nearResult.x + nearResult.y + nearResult.z;
    float farLum = farResult.x + farResult.y + farResult.z;

    assert(nearLum > farLum && "closer light must produce brighter fragment");
    std::cout << "  FS light attenuation: PASS" << std::endl;
}

void test_fs_specular_highlight() {
    // INVARIANT: Specular highlight is strongest when view direction reflects the light
    FragmentShader fs;
    Material mat;
    mat.albedo = Color(1, 1, 1);
    mat.ambient = 0.0f;
    mat.diffuse = 0.0f;
    mat.specular = 1.0f;
    mat.shininess = 64.0f;

    // Light at (0, 0, 5), camera at (0, 0, 5), fragment at origin with normal (0,0,1)
    // Perfect mirror reflection → maximum specular
    SceneLighting lighting;
    lighting.cameraPos = Vec3(0, 0, 5);
    lighting.ambientColor = Color(0, 0, 0);
    lighting.lights.push_back({{0, 0, 5}, {1,1,1}, 5.0f, 100.0f});

    auto cb = fs.build(mat, lighting);

    Fragment perfectReflect;
    perfectReflect.worldPos = Vec3(0, 0, 0);
    perfectReflect.normal = Vec3(0, 0, 1);
    perfectReflect.color = Vec3(1, 1, 1);
    perfectReflect.uv = Vec2(0, 0);

    Fragment offAngle;
    offAngle.worldPos = Vec3(0, 0, 0);
    offAngle.normal = Vec3(0.5f, 0.5f, 0.707f).normalized();
    offAngle.color = Vec3(1, 1, 1);
    offAngle.uv = Vec2(0, 0);

    Color perfectResult = cb(perfectReflect);
    Color offResult = cb(offAngle);

    float perfectLum = perfectResult.x + perfectResult.y + perfectResult.z;
    float offLum = offResult.x + offResult.y + offResult.z;

    assert(perfectLum > offLum && "perfect reflection must produce stronger specular");
    std::cout << "  FS specular highlight: PASS" << std::endl;
}

void test_fs_output_clamped() {
    // INVARIANT: Fragment shader output must be in [0, 1]
    FragmentShader fs;
    Material mat;
    mat.albedo = Color(1, 1, 1);
    mat.ambient = 1.0f;
    mat.diffuse = 1.0f;
    mat.specular = 1.0f;

    SceneLighting lighting;
    lighting.cameraPos = Vec3(0, 0, 1);
    lighting.ambientColor = Color(1, 1, 1);
    // Very bright, close light
    lighting.lights.push_back({{0, 0, 0.01f}, {10,10,10}, 100.0f, 1.0f});

    auto cb = fs.build(mat, lighting);

    Fragment frag;
    frag.worldPos = Vec3(0, 0, 0);
    frag.normal = Vec3(0, 0, 1);
    frag.color = Vec3(1, 1, 1);
    frag.uv = Vec2(0, 0);

    Color result = cb(frag);
    assert(result.x >= 0 && result.x <= 1.0f && "output R must be clamped to [0,1]");
    assert(result.y >= 0 && result.y <= 1.0f && "output G must be clamped to [0,1]");
    assert(result.z >= 0 && result.z <= 1.0f && "output B must be clamped to [0,1]");
    std::cout << "  FS output clamped: PASS" << std::endl;
}

// =============================================================================
// Texture Tests
// =============================================================================

void test_texture_checkerboard_pattern() {
    // INVARIANT: Checkerboard has alternating colors
    Texture tex = Texture::checkerboard(64, 64, 16);

    Color c00 = tex.sampleNearest(0.0f, 0.0f);
    Color c01 = tex.sampleNearest(0.25f, 0.0f); // Different square
    // c00 and c01 should be different colors (one bright, one dark)
    float lum0 = c00.x + c00.y + c00.z;
    float lum1 = c01.x + c01.y + c01.z;
    assert(std::abs(lum0 - lum1) > 0.5f && "checkerboard must have contrasting squares");

    std::cout << "  texture checkerboard pattern: PASS" << std::endl;
}

void test_texture_bilinear_continuity() {
    // INVARIANT: Bilinear sampling should be continuous (no jumps between adjacent samples)
    Texture tex = Texture::checkerboard(32, 32, 8);

    float maxJump = 0;
    for (int i = 0; i < 100; ++i) {
        float u = i / 100.0f;
        Color c0 = tex.sample(u, 0.5f);
        Color c1 = tex.sample(u + 0.001f, 0.5f);
        float jump = std::abs(c0.x - c1.x) + std::abs(c0.y - c1.y) + std::abs(c0.z - c1.z);
        maxJump = std::max(maxJump, jump);
    }
    // Bilinear should have smooth transitions — max jump should be small
    assert(maxJump < 0.5f && "bilinear sampling must be approximately continuous");
    std::cout << "  texture bilinear continuity: PASS" << std::endl;
}

void test_texture_wrap() {
    // INVARIANT: sample(u+1, v) = sample(u, v) (wrapping)
    Texture tex = Texture::checkerboard(32, 32, 8);
    for (float u = 0; u < 1.0f; u += 0.1f) {
        Color c0 = tex.sample(u, 0.3f);
        Color c1 = tex.sample(u + 1.0f, 0.3f);
        assert(approx(c0.x, c1.x, 0.01f) && "texture must wrap in U");
        assert(approx(c0.y, c1.y, 0.01f) && "texture must wrap in U");
    }
    std::cout << "  texture UV wrapping: PASS" << std::endl;
}

void test_texture_empty() {
    Texture empty;
    assert(!empty.valid() && "empty texture must report invalid");
    Color c = empty.sample(0.5f, 0.5f);
    assert(approx(c.x, 1.0f) && "empty texture sample must return white");
    std::cout << "  empty texture: PASS" << std::endl;
}

// =============================================================================
// Full Renderer Integration Tests
// =============================================================================

// Define a simple quad (two triangles)
static void makeQuad(Vertex verts[4], uint32_t indices[6]) {
    verts[0] = {{-1, -1, 0}, {0, 0, 1}, {0, 0}, {1, 1, 1}};
    verts[1] = {{ 1, -1, 0}, {0, 0, 1}, {1, 0}, {1, 1, 1}};
    verts[2] = {{ 1,  1, 0}, {0, 0, 1}, {1, 1}, {1, 1, 1}};
    verts[3] = {{-1,  1, 0}, {0, 0, 1}, {0, 1}, {1, 1, 1}};
    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    indices[3] = 0; indices[4] = 2; indices[5] = 3;
}

void test_renderer_basic_draw() {
    // Render a quad in front of camera — should produce pixels
    Renderer r(64, 64);
    r.beginFrame();

    r.setView(Mat4::lookAt(Vec3(0, 0, 3), Vec3(0, 0, 0), Vec3(0, 1, 0)));
    r.setProjection(Mat4::perspective(PI / 3, 1.0f, 0.1f, 100.0f));
    r.setModel(Mat4::identity());
    r.setCameraPos(Vec3(0, 0, 3));
    r.addLight({{0, 0, 5}, {1,1,1}, 5.0f, 100.0f});

    Vertex verts[4];
    uint32_t indices[6];
    makeQuad(verts, indices);

    Material mat;
    mat.albedo = Color(1, 0.5f, 0.3f);

    r.drawMesh(verts, 4, indices, 6, mat);

    int lit = countNonBlack(r.framebuffer());
    assert(lit > 100 && "basic quad render must produce significant pixel coverage");
    std::cout << "  renderer basic draw: PASS" << std::endl;
}

void test_renderer_nothing_behind_camera() {
    // Object behind camera should produce no pixels
    Renderer r(64, 64);
    r.beginFrame();

    r.setView(Mat4::lookAt(Vec3(0, 0, 3), Vec3(0, 0, 0), Vec3(0, 1, 0)));
    r.setProjection(Mat4::perspective(PI / 3, 1.0f, 0.1f, 100.0f));
    r.setModel(Mat4::translation(Vec3(0, 0, 10)));  // Behind camera
    r.setCameraPos(Vec3(0, 0, 3));
    r.addLight({{0, 0, 5}, {1,1,1}, 5.0f, 100.0f});

    Vertex verts[4];
    uint32_t indices[6];
    makeQuad(verts, indices);

    Material mat;
    mat.albedo = Color(1, 1, 1);

    r.drawMesh(verts, 4, indices, 6, mat);

    int lit = countNonBlack(r.framebuffer());
    assert(lit == 0 && "object behind camera must produce no pixels");
    std::cout << "  renderer nothing behind camera: PASS" << std::endl;
}

void test_renderer_closer_is_larger() {
    // INVARIANT: Closer object should cover more pixels (perspective projection)
    auto renderQuadAtZ = [](float modelZ) -> int {
        Renderer r(128, 128);
        r.beginFrame();
        r.setView(Mat4::lookAt(Vec3(0, 0, 5), Vec3(0, 0, 0), Vec3(0, 1, 0)));
        r.setProjection(Mat4::perspective(PI / 3, 1.0f, 0.1f, 100.0f));
        r.setModel(Mat4::translation(Vec3(0, 0, modelZ)));
        r.setCameraPos(Vec3(0, 0, 5));
        r.addLight({{0, 0, 10}, {1,1,1}, 5.0f, 200.0f});

        Vertex verts[4];
        uint32_t indices[6];
        makeQuad(verts, indices);

        Material mat;
        mat.albedo = Color(1, 1, 1);
        mat.ambient = 0.5f;

        r.drawMesh(verts, 4, indices, 6, mat);
        return countNonBlack(r.framebuffer());
    };

    int closePixels = renderQuadAtZ(0.0f);   // z=0, camera at z=5 → distance = 5
    int farPixels = renderQuadAtZ(-10.0f);    // z=-10, camera at z=5 → distance = 15

    assert(closePixels > farPixels && "closer object must cover more pixels than farther one");
    std::cout << "  renderer closer is larger (perspective): PASS" << std::endl;
}

void test_renderer_resize() {
    Renderer r(32, 32);
    assert(r.width() == 32 && r.height() == 32);

    r.resize(64, 128);
    assert(r.width() == 64 && r.height() == 128 && "resize must update dimensions");

    r.beginFrame();
    int lit = countNonBlack(r.framebuffer());
    assert(lit == 0 && "resized framebuffer must be cleared");
    std::cout << "  renderer resize: PASS" << std::endl;
}

void test_renderer_begin_frame_clears() {
    Renderer r(32, 32);

    // Draw something
    r.beginFrame();
    r.setView(Mat4::lookAt(Vec3(0, 0, 3), Vec3(0, 0, 0), Vec3(0, 1, 0)));
    r.setProjection(Mat4::perspective(PI / 3, 1.0f, 0.1f, 100.0f));
    r.setModel(Mat4::identity());
    r.setCameraPos(Vec3(0, 0, 3));
    r.addLight({{0, 0, 5}, {1,1,1}, 5.0f, 100.0f});

    Vertex verts[4];
    uint32_t indices[6];
    makeQuad(verts, indices);
    Material mat;
    mat.albedo = Color(1, 1, 1);
    r.drawMesh(verts, 4, indices, 6, mat);

    int litBefore = countNonBlack(r.framebuffer());
    assert(litBefore > 0);

    // beginFrame should clear everything
    r.beginFrame();
    int litAfter = countNonBlack(r.framebuffer());
    assert(litAfter == 0 && "beginFrame must clear all pixels");

    std::cout << "  renderer beginFrame clears: PASS" << std::endl;
}

void test_renderer_multiple_draws() {
    // Two separate draw calls should both contribute pixels
    Renderer r(128, 128);
    r.beginFrame();
    r.setView(Mat4::lookAt(Vec3(0, 0, 5), Vec3(0, 0, 0), Vec3(0, 1, 0)));
    r.setProjection(Mat4::perspective(PI / 3, 1.0f, 0.1f, 100.0f));
    r.setCameraPos(Vec3(0, 0, 5));
    r.addLight({{0, 0, 10}, {1,1,1}, 5.0f, 200.0f});

    Vertex verts[4];
    uint32_t indices[6];
    makeQuad(verts, indices);

    Material mat;
    mat.albedo = Color(1, 1, 1);
    mat.ambient = 0.5f;

    // Draw left quad
    r.setModel(Mat4::translation(Vec3(-2, 0, 0)));
    r.drawMesh(verts, 4, indices, 6, mat);
    int afterFirst = countNonBlack(r.framebuffer());

    // Draw right quad
    r.setModel(Mat4::translation(Vec3(2, 0, 0)));
    r.drawMesh(verts, 4, indices, 6, mat);
    int afterSecond = countNonBlack(r.framebuffer());

    assert(afterSecond > afterFirst && "second draw must add more pixels");
    std::cout << "  renderer multiple draws: PASS" << std::endl;
}

// =============================================================================
// Framebuffer Additional Tests
// =============================================================================

void test_framebuffer_depth_test_closer_wins() {
    Framebuffer fb(10, 10);
    fb.clear();

    // Write depth 0.8
    assert(fb.depthTest(5, 5, 0.8f) == true && "first write must pass");
    assert(approx(fb.getDepth(5, 5), 0.8f));

    // Write closer depth 0.3 — must pass
    assert(fb.depthTest(5, 5, 0.3f) == true && "closer depth must pass");
    assert(approx(fb.getDepth(5, 5), 0.3f));

    // Write farther depth 0.5 — must fail
    assert(fb.depthTest(5, 5, 0.5f) == false && "farther depth must fail");
    assert(approx(fb.getDepth(5, 5), 0.3f) && "depth must not change on failed test");

    // Write equal depth — must fail (strict less-than)
    assert(fb.depthTest(5, 5, 0.3f) == false && "equal depth must fail");

    std::cout << "  framebuffer depth test: PASS" << std::endl;
}

void test_framebuffer_pixel_readback() {
    Framebuffer fb(4, 4);
    fb.clear();

    // Write specific pixels and read them back
    fb.setPixel(0, 0, Color(1, 0, 0));
    fb.setPixel(3, 3, Color(0, 1, 0));
    fb.setPixel(1, 2, Color(0, 0, 1));

    Pixel p00 = fb.pixels()[0 * 4 + 0];
    assert(p00.r == 255 && p00.g == 0 && p00.b == 0 && "pixel(0,0) must be red");

    Pixel p33 = fb.pixels()[3 * 4 + 3];
    assert(p33.r == 0 && p33.g == 255 && p33.b == 0 && "pixel(3,3) must be green");

    Pixel p12 = fb.pixels()[2 * 4 + 1];
    assert(p12.r == 0 && p12.g == 0 && p12.b == 255 && "pixel(1,2) must be blue");

    // Unwritten pixel should be black
    Pixel p10 = fb.pixels()[0 * 4 + 1];
    assert(p10.r == 0 && p10.g == 0 && p10.b == 0 && "unwritten pixel must be black");

    std::cout << "  framebuffer pixel readback: PASS" << std::endl;
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Pipeline Integration Tests ===" << std::endl;

    std::cout << "\n--- Vertex Processor ---" << std::endl;
    test_vp_identity_transform();
    test_vp_translation();
    test_vp_batch_matches_single();
    test_vp_rotation_preserves_distance();

    std::cout << "\n--- Fragment Shader ---" << std::endl;
    test_fs_unlit_returns_albedo();
    test_fs_lit_brighter_facing_light();
    test_fs_light_attenuation();
    test_fs_specular_highlight();
    test_fs_output_clamped();

    std::cout << "\n--- Texture ---" << std::endl;
    test_texture_checkerboard_pattern();
    test_texture_bilinear_continuity();
    test_texture_wrap();
    test_texture_empty();

    std::cout << "\n--- Full Renderer ---" << std::endl;
    test_renderer_basic_draw();
    test_renderer_nothing_behind_camera();
    test_renderer_closer_is_larger();
    test_renderer_resize();
    test_renderer_begin_frame_clears();
    test_renderer_multiple_draws();

    std::cout << "\n--- Framebuffer ---" << std::endl;
    test_framebuffer_depth_test_closer_wins();
    test_framebuffer_pixel_readback();

    std::cout << "\n=== ALL PIPELINE INTEGRATION TESTS PASSED ===" << std::endl;
    return 0;
}
