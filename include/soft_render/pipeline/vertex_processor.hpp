#pragma once

#include "vertex.hpp"
#include "../math/mat4.hpp"
#include <vector>

namespace sr {
namespace pipeline {

struct Uniforms {
    math::Mat4 model;
    math::Mat4 view;
    math::Mat4 projection;
    math::Mat4 normalMatrix; // transpose(inverse(model)) - for correct normal transform
};

class VertexProcessor {
public:
    // Transform a single vertex through the MVP pipeline
    ClipVertex process(const Vertex& v, const Uniforms& u) const;

    // Transform a batch of vertices
    void processBatch(const Vertex* in, ClipVertex* out, int count, const Uniforms& u) const;
};

}
}
