#pragma once

#include "math/gfxm.hpp"
#include "collision/common.hpp"


enum class CONTACT_POINT_TYPE {
    DEFAULT,
    TRIANGLE_FACE,
    TRIANGLE_EDGE,
    TRIANGLE_CORNER
};

struct ContactPoint {
    gfxm::vec3 point_a;
    gfxm::vec3 point_b;
    gfxm::vec3 normal_a;
    gfxm::vec3 normal_b;
    gfxm::vec3 lcl_point_a;
    gfxm::vec3 lcl_point_b;
    gfxm::vec3 lcl_normal; // Always normal on A
    float depth;
    CONTACT_POINT_TYPE type;
    int edge_idx; // local to triangle
    int tick_id = 0;

    // Physics experiments
    float jn_acc = .0f;
    gfxm::vec3 jt_acc;
    float mass_normal = .0f;
    float mass_tangent1 = .0f;
    float mass_tangent2 = .0f;
    gfxm::vec3 t1, t2;
    float vn0 = .0f;
    float bias = .0f;
    bool bounceApplied = false;
    // dbg
    float dbg_vn = .0f;
};

struct RayHitPoint {
    gfxm::vec3 point;
    gfxm::vec3 normal;
    CollisionSurfaceProp prop;
    float distance;
};

struct SweepContactPoint {
    gfxm::vec3 contact;
    gfxm::vec3 normal;
    gfxm::vec3 sweep_contact_pos;
    CollisionSurfaceProp prop;
    float distance_traveled;
    CONTACT_POINT_TYPE type;
};
