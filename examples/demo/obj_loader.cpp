#include "obj_loader.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <array>

namespace demo {

using namespace sr::pipeline;
using namespace sr::math;

bool loadOBJ(const char* path, Mesh& out) {
    std::ifstream file(path);
    if (!file) return false;

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> texcoords;

    out.vertices.clear();
    out.indices.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string tok;
        ss >> tok;

        if (tok == "v") {
            Vec3 p; ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (tok == "vn") {
            Vec3 n; ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (tok == "vt") {
            Vec2 t; ss >> t.x >> t.y;
            texcoords.push_back(t);
        } else if (tok == "f") {
            // Parse face (triangulate n-gons via fan)
            std::vector<std::array<int,3>> face; // pos/tex/nrm (0-indexed, -1=absent)
            std::string token;
            while (ss >> token) {
                std::array<int,3> idx = {-1, -1, -1};
                std::replace(token.begin(), token.end(), '/', ' ');
                std::istringstream ts(token);
                int v;
                if (ts >> v) idx[0] = v > 0 ? v-1 : (int)positions.size() + v;
                if (ts >> v) idx[1] = v > 0 ? v-1 : (int)texcoords.size() + v;
                if (ts >> v) idx[2] = v > 0 ? v-1 : (int)normals.size() + v;
                face.push_back(idx);
            }
            // Fan triangulate
            for (int i = 1; i + 1 < (int)face.size(); ++i) {
                for (int k : {0, i, i+1}) {
                    Vertex vtx;
                    auto& fi = face[k];
                    if (fi[0] >= 0 && fi[0] < (int)positions.size())
                        vtx.position = positions[fi[0]];
                    if (fi[1] >= 0 && fi[1] < (int)texcoords.size())
                        vtx.uv = texcoords[fi[1]];
                    if (fi[2] >= 0 && fi[2] < (int)normals.size())
                        vtx.normal = normals[fi[2]];
                    vtx.color = {1, 1, 1};
                    out.indices.push_back((uint32_t)out.vertices.size());
                    out.vertices.push_back(vtx);
                }
            }
        }
    }
    return !out.vertices.empty();
}

// ----------------------------------------------------------------
// Procedural meshes
// ----------------------------------------------------------------
Mesh makeCube() {
    // 8 unique verts, 12 triangles (6 faces × 2 tri)
    static const float P[8][3] = {
        {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
        {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
    };
    static const int F[6][4] = {
        {0,1,2,3}, {5,4,7,6}, {4,0,3,7},
        {1,5,6,2}, {3,2,6,7}, {4,5,1,0}
    };
    static const float N[6][3] = {
        { 0, 0,-1}, { 0, 0, 1}, {-1, 0, 0},
        { 1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}
    };
    static const float UV[4][2] = {{0,0},{1,0},{1,1},{0,1}};

    Mesh m;
    for (int f = 0; f < 6; ++f) {
        uint32_t base = (uint32_t)m.vertices.size();
        for (int k = 0; k < 4; ++k) {
            Vertex v;
            const float* p = P[F[f][k]];
            v.position = {p[0], p[1], p[2]};
            v.normal   = {N[f][0], N[f][1], N[f][2]};
            v.uv       = {UV[k][0], UV[k][1]};
            v.color    = {1, 1, 1};
            m.vertices.push_back(v);
        }
        // Two triangles per quad
        m.indices.insert(m.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    return m;
}

Mesh makeSphere(int slices, int stacks) {
    Mesh m;
    const float PI = 3.14159265358979f;

    for (int st = 0; st <= stacks; ++st) {
        float phi = PI * st / stacks;
        float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
        for (int sl = 0; sl <= slices; ++sl) {
            float theta = 2 * PI * sl / slices;
            float sinT = std::sin(theta), cosT = std::cos(theta);
            Vertex v;
            v.normal   = {sinPhi * cosT, cosPhi, sinPhi * sinT};
            v.position = v.normal;
            v.uv       = {(float)sl / slices, (float)st / stacks};
            v.color    = {1, 1, 1};
            m.vertices.push_back(v);
        }
    }

    for (int st = 0; st < stacks; ++st) {
        for (int sl = 0; sl < slices; ++sl) {
            uint32_t a = st       * (slices+1) + sl;
            uint32_t b = st       * (slices+1) + sl + 1;
            uint32_t c = (st + 1) * (slices+1) + sl;
            uint32_t d = (st + 1) * (slices+1) + sl + 1;
            m.indices.insert(m.indices.end(), {a, c, b, b, c, d});
        }
    }
    return m;
}

}
