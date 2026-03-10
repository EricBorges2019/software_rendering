#include "soft_render/pipeline/fragment_shader.hpp"
#include <chrono>
#include <iostream>
#include <vector>

using namespace sr;
using namespace sr::math;
using namespace sr::pipeline;

int main() {
    FragmentShader shader;
    Material mat;
    mat.albedo = {0.8f, 0.4f, 0.15f};
    mat.ambient = 0.04f;
    mat.diffuse = 0.85f;
    mat.specular = 0.6f;
    mat.shininess = 64.0f;

    SceneLighting lighting;
    lighting.cameraPos = {0, 1.5f, 4.0f};
    lighting.ambientColor = {0.1f, 0.1f, 0.15f};
    lighting.lights.push_back({ {3, 4, 3},   {1.0f, 0.95f, 0.8f}, 6.0f, 10.0f });
    lighting.lights.push_back({ {-3, 2, -2}, {0.3f, 0.4f,  1.0f}, 2.0f,  8.0f });

    Fragment frag;
    frag.worldPos = {0, 0, 0};
    frag.normal = {0, 1, 0};
    frag.color = {1, 1, 1};
    frag.uv = {0.5f, 0.5f};

    auto callback = shader.build(mat, lighting);

    const int iterations = 10000000;

    // Warmup
    for (int i = 0; i < 1000; ++i) {
        callback(frag);
    }

    auto start = std::chrono::high_resolution_clock::now();

    Color totalColor(0, 0, 0);
    for (int i = 0; i < iterations; ++i) {
        // Slightly vary frag to prevent some compiler optimizations if any
        frag.worldPos.x = (float)i * 0.000001f;
        Color c = callback(frag);
        totalColor += c;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "Total time: " << diff.count() << " s" << std::endl;
    std::cout << "Average time per call: " << (diff.count() / iterations) * 1e9 << " ns" << std::endl;
    std::cout << "Total color (to prevent optimization): " << totalColor.x << std::endl;

    return 0;
}
