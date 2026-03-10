// =============================================================================
// test_math_invariants.cpp — Uncheateable mathematical property tests
//
// Strategy: Test INVARIANTS that hold for ALL inputs, not specific values.
// A stub implementation cannot pass these without implementing the real math.
// Uses a deterministic PRNG so tests are reproducible but inputs are varied.
//
// Tolerances:
//   EPS       (1e-4) — single operations (dot, cross, normalize, etc.)
//   EPS_LOOSE (1e-2) — chained transforms (rotation composition, MVP chains)
// =============================================================================

#include <cassert>
#include <cmath>
#include <iostream>

#include "test_helpers.hpp"
#include "soft_render/math/vec3.hpp"
#include "soft_render/math/vec4.hpp"
#include "soft_render/math/mat4.hpp"

using namespace sr::math;
using namespace sr_test;

// =============================================================================
// Vec3 invariant tests
// =============================================================================

void test_vec3_cross_perpendicularity() {
    // INVARIANT: cross(a, b) is perpendicular to both a and b
    for (int i = 0; i < 200; ++i) {
        Vec3 a = randVec3();
        Vec3 b = randVec3();
        Vec3 c = a.cross(b);

        if (c.length() < 1e-3f) continue;

        float dot_a = std::abs(c.dot(a));
        float dot_b = std::abs(c.dot(b));
        assert(dot_a < EPS_LOOSE && "cross product must be perpendicular to first operand");
        assert(dot_b < EPS_LOOSE && "cross product must be perpendicular to second operand");
    }
    std::cout << "  cross perpendicularity: PASS" << std::endl;
}

void test_vec3_cross_magnitude() {
    // INVARIANT: |a x b|^2 + (a . b)^2 = |a|^2 * |b|^2 (Lagrange identity)
    for (int i = 0; i < 200; ++i) {
        Vec3 a = randVec3();
        Vec3 b = randVec3();
        Vec3 c = a.cross(b);

        float lhs = c.lengthSq() + a.dot(b) * a.dot(b);
        float rhs = a.lengthSq() * b.lengthSq();
        // Use relative tolerance for large values
        float tol = std::max(EPS_LOOSE, std::abs(rhs) * EPS);
        assert(approx(lhs, rhs, tol) && "Lagrange identity must hold");
    }
    std::cout << "  cross magnitude (Lagrange identity): PASS" << std::endl;
}

void test_vec3_cross_anticommutativity() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        Vec3 b = randVec3();
        Vec3 ab = a.cross(b);
        Vec3 ba = b.cross(a);
        assert(approxVec3(ab, ba * -1.0f, EPS) && "cross must be anti-commutative");
    }
    std::cout << "  cross anti-commutativity: PASS" << std::endl;
}

void test_vec3_cross_self_is_zero() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        Vec3 c = a.cross(a);
        assert(c.length() < EPS && "cross product with self must be zero");
    }
    std::cout << "  cross self = zero: PASS" << std::endl;
}

void test_vec3_dot_commutative() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        Vec3 b = randVec3();
        assert(approx(a.dot(b), b.dot(a), EPS) && "dot must be commutative");
    }
    std::cout << "  dot commutativity: PASS" << std::endl;
}

void test_vec3_dot_self_equals_lengthsq() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        assert(approx(a.dot(a), a.lengthSq(), EPS) && "a.a must equal |a|^2");
    }
    std::cout << "  dot(a,a) = lengthSq(a): PASS" << std::endl;
}

void test_vec3_normalize_unit_length() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        if (a.length() < 1e-6f) continue;
        Vec3 n = a.normalized();
        assert(approx(n.length(), 1.0f, EPS) && "normalized vector must have unit length");
    }
    std::cout << "  normalize produces unit length: PASS" << std::endl;
}

void test_vec3_normalize_preserves_direction() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        if (a.length() < 1e-6f) continue;
        Vec3 n = a.normalized();
        Vec3 c = n.cross(a);
        assert(c.length() < EPS_LOOSE && "normalized vector must be parallel to original");
        assert(n.dot(a) > 0 && "normalized must preserve direction");
    }
    std::cout << "  normalize preserves direction: PASS" << std::endl;
}

void test_vec3_lerp_endpoints() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        Vec3 b = randVec3();
        assert(approxVec3(a.lerp(b, 0.0f), a, EPS) && "lerp(0) must return start");
        assert(approxVec3(a.lerp(b, 1.0f), b, EPS) && "lerp(1) must return end");
    }
    std::cout << "  lerp endpoints: PASS" << std::endl;
}

void test_vec3_lerp_midpoint() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        Vec3 b = randVec3();
        Vec3 mid = a.lerp(b, 0.5f);
        Vec3 expected = { (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f };
        assert(approxVec3(mid, expected, EPS) && "lerp(0.5) must be midpoint");
    }
    std::cout << "  lerp midpoint: PASS" << std::endl;
}

void test_vec3_arithmetic_identities() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        Vec3 zero(0, 0, 0);

        assert(approxVec3(a + zero, a, EPS) && "a + 0 must equal a");
        assert(approxVec3(a - a, zero, EPS) && "a - a must equal 0");
        assert(approxVec3(a * 1.0f, a, EPS) && "a * 1 must equal a");
        assert(approxVec3(a * 0.0f, zero, EPS) && "a * 0 must equal 0");
    }
    std::cout << "  arithmetic identities: PASS" << std::endl;
}

void test_vec3_distributive() {
    // a . (b + c) = a.b + a.c
    for (int i = 0; i < 100; ++i) {
        Vec3 a = randVec3();
        Vec3 b = randVec3();
        Vec3 c = randVec3();
        float lhs = a.dot(b + c);
        float rhs = a.dot(b) + a.dot(c);
        // Use relative tolerance — large vectors produce large dot products
        float tol = std::max(EPS, std::abs(rhs) * EPS);
        assert(approx(lhs, rhs, tol) && "dot must distribute over addition");
    }
    std::cout << "  dot distributive: PASS" << std::endl;
}

// =============================================================================
// Mat4 invariant tests
// =============================================================================

static float mat4_determinant_3x3(const Mat4& m) {
    return m(0,0) * (m(1,1)*m(2,2) - m(1,2)*m(2,1))
         - m(0,1) * (m(1,0)*m(2,2) - m(1,2)*m(2,0))
         + m(0,2) * (m(1,0)*m(2,1) - m(1,1)*m(2,0));
}

void test_rotation_determinant_is_one() {
    for (int i = 0; i < 100; ++i) {
        float angle = randf_range(-PI, PI);
        Mat4 rx = Mat4::rotationX(angle);
        Mat4 ry = Mat4::rotationY(angle);
        Mat4 rz = Mat4::rotationZ(angle);

        assert(approx(mat4_determinant_3x3(rx), 1.0f, EPS) && "rotationX det must be 1");
        assert(approx(mat4_determinant_3x3(ry), 1.0f, EPS) && "rotationY det must be 1");
        assert(approx(mat4_determinant_3x3(rz), 1.0f, EPS) && "rotationZ det must be 1");
    }
    std::cout << "  rotation determinant = 1: PASS" << std::endl;
}

void test_rotation_orthogonality() {
    // R^T * R = I — uses EPS_LOOSE because of chained multiply
    for (int i = 0; i < 100; ++i) {
        float angle = randf_range(-PI, PI);
        Mat4 R = Mat4::rotationX(angle) * Mat4::rotationY(randf_range(-PI, PI));
        Mat4 Rt = R.transposed();
        Mat4 RtR = Rt * R;

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                float expected = (r == c) ? 1.0f : 0.0f;
                assert(approx(RtR(r, c), expected, EPS_LOOSE) && "R^T * R must be identity");
            }
        }
    }
    std::cout << "  rotation orthogonality (R^T*R = I): PASS" << std::endl;
}

void test_rotation_preserves_length() {
    // Uses EPS_LOOSE: 3 chained rotations + matrix-vector multiply
    for (int i = 0; i < 100; ++i) {
        float angle = randf_range(-PI, PI);
        Mat4 R = Mat4::rotationX(angle) * Mat4::rotationY(randf_range(-PI, PI))
                 * Mat4::rotationZ(randf_range(-PI, PI));
        Vec3 v = randVec3();
        Vec4 v4(v, 0.0f);
        Vec4 rotated = R * v4;
        Vec3 r3 = rotated.xyz();

        assert(approx(v.length(), r3.length(), EPS_LOOSE) && "rotation must preserve length");
    }
    std::cout << "  rotation preserves length: PASS" << std::endl;
}

void test_rotation_composition() {
    // R(a) * R(b) = R(a + b) — EPS_LOOSE for chained ops
    for (int i = 0; i < 100; ++i) {
        float a = randf_range(-PI, PI);
        float b = randf_range(-PI, PI);

        Mat4 Ra = Mat4::rotationZ(a);
        Mat4 Rb = Mat4::rotationZ(b);
        Mat4 Rab = Mat4::rotationZ(a + b);
        Mat4 composed = Ra * Rb;

        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                assert(approx(composed(r, c), Rab(r, c), EPS_LOOSE) && "rotation composition must match");
    }
    std::cout << "  rotation composition R(a)*R(b) = R(a+b): PASS" << std::endl;
}

void test_translation_applies_to_points() {
    for (int i = 0; i < 100; ++i) {
        Vec3 offset = randVec3();
        Vec3 point = randVec3();
        Mat4 T = Mat4::translation(offset);
        Vec4 result = T * Vec4(point, 1.0f);

        assert(approxVec3(result.xyz(), point + offset, EPS) && "translation must offset points");
        assert(approx(result.w, 1.0f, EPS) && "translation must preserve w=1");
    }
    std::cout << "  translation applies to points: PASS" << std::endl;
}

void test_translation_ignores_directions() {
    for (int i = 0; i < 100; ++i) {
        Vec3 offset = randVec3();
        Vec3 dir = randVec3();
        Mat4 T = Mat4::translation(offset);
        Vec4 result = T * Vec4(dir, 0.0f);

        assert(approxVec3(result.xyz(), dir, EPS) && "translation must not affect directions");
        assert(approx(result.w, 0.0f, EPS) && "translation must preserve w=0");
    }
    std::cout << "  translation ignores directions (w=0): PASS" << std::endl;
}

void test_scale_multiplies_components() {
    for (int i = 0; i < 100; ++i) {
        Vec3 s = randVec3();
        Vec3 v = randVec3();
        Mat4 S = Mat4::scale(s);
        Vec4 result = S * Vec4(v, 1.0f);
        Vec3 expected = { v.x * s.x, v.y * s.y, v.z * s.z };

        assert(approxVec3(result.xyz(), expected, EPS) && "scale must multiply components");
    }
    std::cout << "  scale multiplies components: PASS" << std::endl;
}

void test_mat4_multiplication_associative() {
    // (A * B) * C = A * (B * C) — EPS_LOOSE for triple chain
    for (int i = 0; i < 50; ++i) {
        float a1 = randf_range(-PI, PI), a2 = randf_range(-PI, PI);
        Mat4 A = Mat4::rotationX(a1);
        Mat4 B = Mat4::translation(randVec3());
        Mat4 C = Mat4::rotationY(a2);

        Mat4 lhs = (A * B) * C;
        Mat4 rhs = A * (B * C);

        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                assert(approx(lhs(r, c), rhs(r, c), EPS_LOOSE) && "matrix multiplication must be associative");
    }
    std::cout << "  matrix multiplication associativity: PASS" << std::endl;
}

void test_mat4_identity_is_neutral() {
    for (int i = 0; i < 50; ++i) {
        Mat4 A = Mat4::rotationX(randf_range(-PI, PI)) *
                 Mat4::translation(randVec3()) *
                 Mat4::scale(randVec3());
        Mat4 I = Mat4::identity();

        Mat4 AI = A * I;
        Mat4 IA = I * A;

        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c) {
                assert(approx(AI(r, c), A(r, c), EPS) && "A * I must equal A");
                assert(approx(IA(r, c), A(r, c), EPS) && "I * A must equal A");
            }
    }
    std::cout << "  identity is neutral element: PASS" << std::endl;
}

void test_mat4_vec_multiplication_distributes() {
    // M * (a + b) = M*a + M*b — EPS_LOOSE for chained ops with large values
    for (int i = 0; i < 50; ++i) {
        Mat4 M = Mat4::rotationZ(randf_range(-PI, PI)) * Mat4::scale(randVec3());
        Vec4 a(randVec3(), 0.0f);
        Vec4 b(randVec3(), 0.0f);

        Vec4 lhs = M * (a + b);
        Vec4 rhs = (M * a) + (M * b);

        for (int j = 0; j < 4; ++j)
            assert(approx(lhs[j], rhs[j], EPS_LOOSE) && "M*(a+b) must equal M*a + M*b");
    }
    std::cout << "  matrix-vector distributivity: PASS" << std::endl;
}

void test_transpose_involution() {
    for (int i = 0; i < 50; ++i) {
        Mat4 A = Mat4::rotationX(randf_range(-PI, PI)) * Mat4::translation(randVec3());
        Mat4 Att = A.transposed().transposed();
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                assert(approx(Att(r, c), A(r, c), EPS) && "double transpose must be identity");
    }
    std::cout << "  transpose involution: PASS" << std::endl;
}

void test_transpose_product() {
    // (A * B)^T = B^T * A^T — EPS_LOOSE for chained ops
    for (int i = 0; i < 50; ++i) {
        Mat4 A = Mat4::rotationX(randf_range(-PI, PI));
        Mat4 B = Mat4::rotationY(randf_range(-PI, PI));
        Mat4 lhs = (A * B).transposed();
        Mat4 rhs = B.transposed() * A.transposed();

        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                assert(approx(lhs(r, c), rhs(r, c), EPS_LOOSE) && "(AB)^T must equal B^T * A^T");
    }
    std::cout << "  transpose product rule: PASS" << std::endl;
}

// =============================================================================
// Perspective projection invariants
// =============================================================================

void test_perspective_maps_near_to_neg1() {
    for (int i = 0; i < 50; ++i) {
        float near = randf_range(0.01f, 10.0f);
        float far = near + randf_range(1.0f, 100.0f);
        float fov = randf_range(0.3f, 2.5f);
        float aspect = randf_range(0.5f, 2.0f);

        Mat4 P = Mat4::perspective(fov, aspect, near, far);
        Vec4 nearPt(0, 0, -near, 1.0f);
        Vec4 clip = P * nearPt;
        float ndcZ = clip.z / clip.w;

        assert(approx(ndcZ, -1.0f, EPS_LOOSE) && "near plane must map to NDC z = -1");
    }
    std::cout << "  perspective: near plane -> z=-1: PASS" << std::endl;
}

void test_perspective_maps_far_to_pos1() {
    for (int i = 0; i < 50; ++i) {
        float near = randf_range(0.01f, 10.0f);
        float far = near + randf_range(1.0f, 100.0f);
        float fov = randf_range(0.3f, 2.5f);
        float aspect = randf_range(0.5f, 2.0f);

        Mat4 P = Mat4::perspective(fov, aspect, near, far);
        Vec4 farPt(0, 0, -far, 1.0f);
        Vec4 clip = P * farPt;
        float ndcZ = clip.z / clip.w;

        assert(approx(ndcZ, 1.0f, EPS_LOOSE) && "far plane must map to NDC z = +1");
    }
    std::cout << "  perspective: far plane -> z=+1: PASS" << std::endl;
}

void test_perspective_center_maps_to_origin() {
    for (int i = 0; i < 50; ++i) {
        float near = randf_range(0.01f, 10.0f);
        float far = near + randf_range(1.0f, 100.0f);
        float fov = randf_range(0.3f, 2.5f);
        float aspect = randf_range(0.5f, 2.0f);
        float z = randf_range(-far, -near);

        Mat4 P = Mat4::perspective(fov, aspect, near, far);
        Vec4 pt(0, 0, z, 1.0f);
        Vec4 clip = P * pt;
        float ndcX = clip.x / clip.w;
        float ndcY = clip.y / clip.w;

        assert(approx(ndcX, 0.0f, EPS) && "center point must map to NDC x=0");
        assert(approx(ndcY, 0.0f, EPS) && "center point must map to NDC y=0");
    }
    std::cout << "  perspective: center -> origin: PASS" << std::endl;
}

void test_perspective_depth_monotonic() {
    float near = 0.1f, far = 100.0f;
    Mat4 P = Mat4::perspective(1.0f, 1.0f, near, far);

    float prevNdcZ = -2.0f;
    for (int i = 0; i < 50; ++i) {
        float z = -near - (far - near) * (i / 49.0f);
        Vec4 clip = P * Vec4(0, 0, z, 1.0f);
        float ndcZ = clip.z / clip.w;

        assert(ndcZ >= prevNdcZ - EPS && "depth must be monotonically increasing with distance");
        prevNdcZ = ndcZ;
    }
    std::cout << "  perspective: depth monotonicity: PASS" << std::endl;
}

// =============================================================================
// LookAt invariants
// =============================================================================

void test_lookat_maps_eye_to_origin() {
    // Uses EPS_LOOSE: lookAt involves cross products and normalization
    for (int i = 0; i < 50; ++i) {
        Vec3 eye = randVec3();
        Vec3 target = randVec3();
        if ((target - eye).length() < 0.1f) continue;
        Vec3 up(0, 1, 0);

        Mat4 V = Mat4::lookAt(eye, target, up);
        Vec4 result = V * Vec4(eye, 1.0f);

        assert(approx(result.x, 0, EPS_LOOSE) && "eye must map to x=0");
        assert(approx(result.y, 0, EPS_LOOSE) && "eye must map to y=0");
        assert(approx(result.z, 0, EPS_LOOSE) && "eye must map to z=0");
    }
    std::cout << "  lookAt maps eye to origin: PASS" << std::endl;
}

void test_lookat_target_on_neg_z() {
    // Uses EPS_LOOSE: same reasoning as above
    for (int i = 0; i < 50; ++i) {
        Vec3 eye = randVec3();
        Vec3 target = randVec3();
        if ((target - eye).length() < 0.1f) continue;
        Vec3 up(0, 1, 0);

        Mat4 V = Mat4::lookAt(eye, target, up);
        Vec4 targetView = V * Vec4(target, 1.0f);

        assert(approx(targetView.x, 0, EPS_LOOSE) && "target must be at x=0 in view space");
        assert(approx(targetView.y, 0, EPS_LOOSE) && "target must be at y=0 in view space");
        assert(targetView.z < 0 && "target must be at z<0 in view space (right-handed)");
    }
    std::cout << "  lookAt: target on -Z axis: PASS" << std::endl;
}

// =============================================================================
// Ortho invariants
// =============================================================================

void test_ortho_maps_center_to_origin() {
    for (int i = 0; i < 50; ++i) {
        float l = randf_range(-10, -0.1f);
        float r = randf_range(0.1f, 10);
        float b = randf_range(-10, -0.1f);
        float t = randf_range(0.1f, 10);
        float n = randf_range(0.01f, 5.0f);
        float f = n + randf_range(1.0f, 50.0f);

        Mat4 O = Mat4::ortho(l, r, b, t, n, f);
        Vec4 center((l + r) * 0.5f, (b + t) * 0.5f, -(n + f) * 0.5f, 1.0f);
        Vec4 result = O * center;

        assert(approx(result.x, 0, EPS_LOOSE) && "ortho center must map to x=0");
        assert(approx(result.y, 0, EPS_LOOSE) && "ortho center must map to y=0");
        assert(approx(result.z, 0, EPS_LOOSE) && "ortho center must map to z=0");
    }
    std::cout << "  ortho: center -> origin: PASS" << std::endl;
}

// =============================================================================
// Vec4 tests
// =============================================================================

void test_vec4_perspective_divide() {
    for (int i = 0; i < 100; ++i) {
        float w = randf_range(0.1f, 10.0f);
        Vec3 xyz = randVec3();
        Vec4 v(xyz, w);
        Vec3 p = v.perspective();

        assert(approx(p.x, xyz.x / w, EPS) && "perspective x must be x/w");
        assert(approx(p.y, xyz.y / w, EPS) && "perspective y must be y/w");
        assert(approx(p.z, xyz.z / w, EPS) && "perspective z must be z/w");
    }
    std::cout << "  Vec4 perspective divide: PASS" << std::endl;
}

void test_vec4_dot_matches_manual() {
    for (int i = 0; i < 100; ++i) {
        Vec3 a3 = randVec3(), b3 = randVec3();
        float aw = randf(), bw = randf();
        Vec4 a(a3, aw);
        Vec4 b(b3, bw);

        float dot = a.dot(b);
        float expected = a3.x*b3.x + a3.y*b3.y + a3.z*b3.z + aw*bw;
        assert(approx(dot, expected, EPS) && "Vec4 dot must match manual computation");
    }
    std::cout << "  Vec4 dot matches manual: PASS" << std::endl;
}

// =============================================================================
// Combined transform chain tests
// =============================================================================

void test_mvp_pipeline_roundtrip() {
    // EPS_LOOSE: chained matrix multiply + matrix-vector multiply
    for (int i = 0; i < 50; ++i) {
        float angle = randf_range(-PI, PI);
        Vec3 trans = randVec3();

        Mat4 combined = Mat4::translation(trans) * Mat4::rotationZ(angle);

        Vec3 pt = randVec3();
        Vec4 pt4(pt, 1.0f);

        Vec4 res1 = combined * pt4;
        Vec4 res2 = Mat4::translation(trans) * (Mat4::rotationZ(angle) * pt4);

        for (int j = 0; j < 4; ++j)
            assert(approx(res1[j], res2[j], EPS_LOOSE) && "combined and sequential transforms must match");
    }
    std::cout << "  MVP pipeline consistency: PASS" << std::endl;
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Math Invariant Tests ===" << std::endl;

    std::cout << "\n--- Vec3 ---" << std::endl;
    test_vec3_cross_perpendicularity();
    test_vec3_cross_magnitude();
    test_vec3_cross_anticommutativity();
    test_vec3_cross_self_is_zero();
    test_vec3_dot_commutative();
    test_vec3_dot_self_equals_lengthsq();
    test_vec3_normalize_unit_length();
    test_vec3_normalize_preserves_direction();
    test_vec3_lerp_endpoints();
    test_vec3_lerp_midpoint();
    test_vec3_arithmetic_identities();
    test_vec3_distributive();

    std::cout << "\n--- Mat4 ---" << std::endl;
    test_rotation_determinant_is_one();
    test_rotation_orthogonality();
    test_rotation_preserves_length();
    test_rotation_composition();
    test_translation_applies_to_points();
    test_translation_ignores_directions();
    test_scale_multiplies_components();
    test_mat4_multiplication_associative();
    test_mat4_identity_is_neutral();
    test_mat4_vec_multiplication_distributes();
    test_transpose_involution();
    test_transpose_product();

    std::cout << "\n--- Perspective ---" << std::endl;
    test_perspective_maps_near_to_neg1();
    test_perspective_maps_far_to_pos1();
    test_perspective_center_maps_to_origin();
    test_perspective_depth_monotonic();

    std::cout << "\n--- LookAt ---" << std::endl;
    test_lookat_maps_eye_to_origin();
    test_lookat_target_on_neg_z();

    std::cout << "\n--- Ortho ---" << std::endl;
    test_ortho_maps_center_to_origin();

    std::cout << "\n--- Vec4 ---" << std::endl;
    test_vec4_perspective_divide();
    test_vec4_dot_matches_manual();

    std::cout << "\n--- Transform Chain ---" << std::endl;
    test_mvp_pipeline_roundtrip();

    std::cout << "\n=== ALL MATH INVARIANT TESTS PASSED ===" << std::endl;
    return 0;
}
