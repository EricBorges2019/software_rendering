#pragma once

#include "soft_render/pipeline/vertex.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace demo {

struct Mesh {
    std::vector<sr::pipeline::Vertex> vertices;
    std::vector<uint32_t> indices;
};

// Load a Wavefront OBJ file (triangulated, no MTL)
bool loadOBJ(const char* path, Mesh& out);

// Generate a unit cube mesh
Mesh makeCube();

// Generate a UV sphere
Mesh makeSphere(int slices = 32, int stacks = 16);

}
