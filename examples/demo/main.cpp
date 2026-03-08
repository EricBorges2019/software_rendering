#include "soft_render/render/renderer.hpp"
#include "soft_render/math/mat4.hpp"
#include "obj_loader.hpp"
#include <cstdio>
#include <cmath>

using namespace sr;
using namespace sr::math;
using namespace sr::render;
using namespace sr::pipeline;

static const float PI = 3.14159265358979f;

int main(int argc, char** argv) {
    const int W = 800, H = 600;
    const int FRAMES = 60;

    Renderer renderer(W, H);

    // Camera
    Vec3 eye(0, 1.5f, 4.0f);
    Vec3 target(0, 0, 0);
    Mat4 view = Mat4::lookAt(eye, target, {0, 1, 0});
    Mat4 proj = Mat4::perspective(PI / 3.0f, (float)W / H, 0.1f, 100.0f);
    renderer.setView(view);
    renderer.setProjection(proj);
    renderer.setCameraPos(eye);

    // Lights
    renderer.addLight({ {3, 4, 3},   {1.0f, 0.95f, 0.8f}, 6.0f, 10.0f });
    renderer.addLight({ {-3, 2, -2}, {0.3f, 0.4f,  1.0f}, 2.0f,  8.0f });

    // Choose mesh: OBJ if provided, else sphere
    demo::Mesh mesh;
    if (argc > 1) {
        if (!demo::loadOBJ(argv[1], mesh)) {
            std::fprintf(stderr, "Failed to load OBJ: %s\n", argv[1]);
            return 1;
        }
        std::fprintf(stderr, "Loaded %s: %zu verts, %zu indices\n",
            argv[1], mesh.vertices.size(), mesh.indices.size());
    } else {
        mesh = demo::makeSphere(48, 32);
    }

    // Material
    Material mat;
    mat.albedo    = {0.8f, 0.4f, 0.15f};
    mat.ambient   = 0.04f;
    mat.diffuse   = 0.85f;
    mat.specular  = 0.6f;
    mat.shininess = 64.0f;

    char path[64];
    for (int frame = 0; frame < FRAMES; ++frame) {
        float angle = (2 * PI * frame) / FRAMES;

        // Spin the model
        Mat4 model = Mat4::rotationY(angle) * Mat4::rotationX(0.3f);
        renderer.setModel(model);

        renderer.beginFrame();

        DrawCall dc;
        dc.vertices    = mesh.vertices.data();
        dc.vertexCount = (int)mesh.vertices.size();
        dc.indices     = mesh.indices.data();
        dc.indexCount  = (int)mesh.indices.size();
        dc.material    = mat;
        renderer.draw(dc);

        std::snprintf(path, sizeof(path), "frame_%04d.ppm", frame);
        renderer.framebuffer().writePPM(path);
        std::fprintf(stderr, "Wrote %s\n", path);
    }

    return 0;
}
