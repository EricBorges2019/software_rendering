#pragma once

#include "../core/framebuffer.hpp"
#include "../core/texture.hpp"
#include "../pipeline/vertex.hpp"
#include "../pipeline/vertex_processor.hpp"
#include "../pipeline/rasterizer.hpp"
#include "../pipeline/fragment_shader.hpp"
#include "../math/mat4.hpp"
#include <vector>

namespace sr {
namespace render {

struct DrawCall {
    const pipeline::Vertex* vertices;
    int vertexCount;
    const uint32_t* indices;   // if null, sequential triangles
    int indexCount;
    pipeline::Material material;
};

class Renderer {
public:
    Renderer(int width, int height);

    // Camera
    void setView(const math::Mat4& view) { uniforms_.view = view; }
    void setProjection(const math::Mat4& proj) { uniforms_.projection = proj; }
    void setModel(const math::Mat4& model);

    // Lighting
    void setLighting(const pipeline::SceneLighting& lighting) { lighting_ = lighting; }
    void addLight(const pipeline::PointLight& light) { lighting_.lights.push_back(light); }
    void setCameraPos(const math::Vec3& pos) { lighting_.cameraPos = pos; }

    // Drawing
    void beginFrame();
    void draw(const DrawCall& call);
    void drawMesh(const pipeline::Vertex* verts, int vCount,
                  const uint32_t* indices, int iCount,
                  const pipeline::Material& mat);

    // Output
    core::Framebuffer& framebuffer() { return fb_; }
    const core::Framebuffer& framebuffer() const { return fb_; }

    void resize(int w, int h);
    int width() const { return fb_.width(); }
    int height() const { return fb_.height(); }

private:
    core::Framebuffer fb_;
    pipeline::VertexProcessor vp_;
    pipeline::FragmentShader fs_;
    pipeline::Uniforms uniforms_;
    pipeline::SceneLighting lighting_;
    std::vector<pipeline::ClipVertex> transformedBuffer_;
    std::vector<pipeline::Triangle> triangleBuffer_;
};

}
}
