#include "soft_render/render/renderer.hpp"
#include <cstring>
#include <cmath>

namespace sr {
namespace render {

// Compute upper-left 3x3 inverse-transpose for normal transform
static math::Mat4 normalMatrix(const math::Mat4& model) {
    // For uniform/affine transforms, transpose of inverse equals:
    // For simplicity, use the model matrix directly (works for rigid bodies).
    // For non-uniform scale, a proper inverse-transpose is needed.
    // We compute it analytically for the 3x3 sub-matrix.
    const float* m = model.m;

    // 3x3 cofactor matrix (= adjugate / det = inverse * det → after transpose = normal matrix * det)
    float c00 =  m[5]*m[10] - m[6]*m[9];
    float c01 = -(m[4]*m[10] - m[6]*m[8]);
    float c02 =  m[4]*m[9]  - m[5]*m[8];
    float c10 = -(m[1]*m[10] - m[2]*m[9]);
    float c11 =  m[0]*m[10] - m[2]*m[8];
    float c12 = -(m[0]*m[9]  - m[1]*m[8]);
    float c20 =  m[1]*m[6]  - m[2]*m[5];
    float c21 = -(m[0]*m[6]  - m[2]*m[4]);
    float c22 =  m[0]*m[5]  - m[1]*m[4];

    return {
        c00, c01, c02, 0,
        c10, c11, c12, 0,
        c20, c21, c22, 0,
        0,   0,   0,   1
    };
}

Renderer::Renderer(int width, int height)
    : fb_(width, height)
{
    uniforms_.model      = math::Mat4::identity();
    uniforms_.view       = math::Mat4::identity();
    uniforms_.projection = math::Mat4::identity();
    uniforms_.normalMatrix = math::Mat4::identity();
}

void Renderer::setModel(const math::Mat4& model) {
    uniforms_.model = model;
    uniforms_.normalMatrix = normalMatrix(model);
}

void Renderer::resize(int w, int h) {
    fb_.resize(w, h);
}

void Renderer::beginFrame() {
    fb_.clear();
}

void Renderer::drawMesh(const pipeline::Vertex* verts, int vCount,
                         const uint32_t* indices, int iCount,
                         const pipeline::Material& mat) {
    // Transform all vertices
    transformedBuffer_.resize(vCount);
    vp_.processBatch(verts, transformedBuffer_.data(), vCount, uniforms_);

    // Build triangle list
    int triCount = iCount / 3;
    triangleBuffer_.resize(triCount);
    for (int i = 0; i < triCount; ++i) {
        triangleBuffer_[i].v[0] = transformedBuffer_[indices[i*3 + 0]];
        triangleBuffer_[i].v[1] = transformedBuffer_[indices[i*3 + 1]];
        triangleBuffer_[i].v[2] = transformedBuffer_[indices[i*3 + 2]];
    }

    // Fragment shader
    pipeline::FragmentShader fs;
    auto fragCb = fs.build(mat, lighting_);

    // Rasterize
    pipeline::Rasterizer rast(fb_);
    rast.rasterizeBatch(triangleBuffer_.data(), triCount, fragCb);
}

void Renderer::draw(const DrawCall& call) {
    drawMesh(call.vertices, call.vertexCount,
             call.indices,  call.indexCount,
             call.material);
}

}
}
