#pragma once

#include "../math/vec2.hpp"
#include "../math/vec3.hpp"
#include "../math/vec4.hpp"

namespace sr {
namespace pipeline {

// Input vertex (model space)
struct Vertex {
    math::Vec3 position;
    math::Vec3 normal;
    math::Vec2 uv;
    math::Vec3 color{1, 1, 1};
};

// Post-transform vertex (clip/NDC space)
struct ClipVertex {
    math::Vec4 clipPos;   // clip-space position (pre-divide)
    math::Vec3 worldPos;  // world-space position (for lighting)
    math::Vec3 normal;    // world-space normal
    math::Vec2 uv;
    math::Vec3 color;
};

}
}
