#pragma once

#include "../math/vec3.hpp"
#include "../core/texture.hpp"
#include "rasterizer.hpp"
#include <functional>

namespace sr {
namespace pipeline {

struct PointLight {
    math::Vec3 position;
    math::Vec3 color{1, 1, 1};
    float intensity{1.0f};
    float radius{100.0f};  // attenuation radius
};

struct Material {
    math::Color albedo{1, 1, 1};
    float ambient{0.05f};
    float diffuse{0.9f};
    float specular{0.3f};
    float shininess{32.0f};
    const core::Texture* albedoMap{nullptr};
    bool useTexture{false};
};

struct SceneLighting {
    math::Vec3 cameraPos;
    math::Vec3 ambientColor{0.1f, 0.1f, 0.15f};
    std::vector<PointLight> lights;
};

class FragmentShader {
public:
    // Returns a fragment callback for use with the rasterizer
    FragmentCallback build(const Material& mat, const SceneLighting& lighting) const;

    // Simple unlit (pass vertex color / albedo through)
    FragmentCallback buildUnlit(const Material& mat) const;

private:
    math::Color shade(const Fragment& frag,
                      const Material& mat,
                      const SceneLighting& lighting) const;
};

}
}
