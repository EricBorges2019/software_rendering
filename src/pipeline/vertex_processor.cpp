#include "soft_render/pipeline/vertex_processor.hpp"

namespace sr {
namespace pipeline {

ClipVertex VertexProcessor::process(const Vertex& v, const Uniforms& u) const {
    ClipVertex out;

    // World position
    math::Vec4 worldPos4 = u.model * math::Vec4(v.position, 1.0f);
    out.worldPos = worldPos4.xyz();

    // Clip position: projection * view * world
    out.clipPos = u.projection * (u.view * worldPos4);

    // Normal (using normal matrix to handle non-uniform scale)
    math::Vec4 n4 = u.normalMatrix * math::Vec4(v.normal, 0.0f);
    out.normal = n4.xyz().normalized();

    out.uv = v.uv;
    out.color = v.color;

    return out;
}

void VertexProcessor::processBatch(const Vertex* in, ClipVertex* out,
                                    int count, const Uniforms& u) const {
    // Precompute combined matrices
    math::Mat4 mv  = u.view * u.model;
    math::Mat4 mvp = u.projection * mv;

    for (int i = 0; i < count; ++i) {
        math::Vec4 pos4(in[i].position, 1.0f);
        out[i].clipPos  = mvp * pos4;
        out[i].worldPos = (u.model * pos4).xyz();
        out[i].normal   = (u.normalMatrix * math::Vec4(in[i].normal, 0.0f)).xyz().normalized();
        out[i].uv       = in[i].uv;
        out[i].color    = in[i].color;
    }
}

}
}
