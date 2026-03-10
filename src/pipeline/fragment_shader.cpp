#include "soft_render/pipeline/fragment_shader.hpp"
#include <cmath>
#include <algorithm>

namespace sr {
namespace pipeline {

static float saturate(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }
static float maxf(float a, float b) { return a > b ? a : b; }

math::Color FragmentShader::shade(const Fragment& frag,
                                   const Material& mat,
                                   const SceneLighting& lighting) const {
    // Sample base color
    math::Color baseColor = mat.albedo * frag.color;
    if (mat.useTexture && mat.albedoMap && mat.albedoMap->valid()) {
        baseColor = baseColor * mat.albedoMap->sample(frag.uv.x, frag.uv.y);
    }

    math::Vec3 N = frag.normal; // already normalized
    math::Vec3 V = (lighting.cameraPos - frag.worldPos).normalized();

    math::Color result = lighting.ambientColor * mat.ambient * baseColor;

    for (const auto& light : lighting.lights) {
        math::Vec3 L = light.position - frag.worldPos;
        float dist = L.length();
        L = L / dist;

        // Attenuation (inverse-square with radius falloff)
        float attn = 1.0f / (1.0f + (dist * dist) / (light.radius * light.radius));
        math::Color lightColor = light.color * (light.intensity * attn);

        // Diffuse (Lambertian)
        float NdotL = maxf(0.f, N.dot(L));
        math::Color diffuse = baseColor * NdotL * mat.diffuse;

        // Specular (Blinn-Phong)
        math::Vec3 H = (L + V).normalized();
        float NdotH = maxf(0.f, N.dot(H));
        float spec = std::pow(NdotH, mat.shininess);
        math::Color specular(spec * mat.specular);

        result += (diffuse + specular) * lightColor;
    }

    // Clamp
    return {saturate(result.x), saturate(result.y), saturate(result.z)};
}

FragmentCallback FragmentShader::build(const Material& mat,
                                        const SceneLighting& lighting) const {
    return [this, mat, lighting](const Fragment& frag) -> math::Color {
        return shade(frag, mat, lighting);
    };
}

FragmentCallback FragmentShader::buildUnlit(const Material& mat) const {
    return [mat](const Fragment& frag) -> math::Color {
        math::Color base = mat.albedo * frag.color;
        if (mat.useTexture && mat.albedoMap && mat.albedoMap->valid())
            base = base * mat.albedoMap->sample(frag.uv.x, frag.uv.y);
        return {saturate(base.x), saturate(base.y), saturate(base.z)};
    };
}

}
}
