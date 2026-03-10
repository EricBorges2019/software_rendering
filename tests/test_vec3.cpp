#include <cassert>
#include <iostream>
#include "soft_render/math/vec3.hpp"

using namespace sr::math;

void test_vec3_cross() {
    // Basic test
    Vec3 i(1, 0, 0);
    Vec3 j(0, 1, 0);
    Vec3 k(0, 0, 1);

    Vec3 k_res = i.cross(j);
    assert(k_res.x == 0 && k_res.y == 0 && k_res.z == 1);

    Vec3 i_res = j.cross(k);
    assert(i_res.x == 1 && i_res.y == 0 && i_res.z == 0);

    Vec3 j_res = k.cross(i);
    assert(j_res.x == 0 && j_res.y == 1 && j_res.z == 0);

    // Anti-commutativity
    Vec3 neg_k = j.cross(i);
    assert(neg_k.x == 0 && neg_k.y == 0 && neg_k.z == -1);

    // Cross product of parallel vectors should be zero
    Vec3 v(1, 2, 3);
    Vec3 zero_res = v.cross(v);
    assert(zero_res.x == 0 && zero_res.y == 0 && zero_res.z == 0);

    // Random test
    Vec3 a(1, 2, 3);
    Vec3 b(4, 5, 6);
    Vec3 a_cross_b = a.cross(b);
    assert(a_cross_b.x == -3 && a_cross_b.y == 6 && a_cross_b.z == -3);

    std::cout << "All Vec3 cross tests passed!" << std::endl;
}

int main() {
    test_vec3_cross();
    return 0;
}
