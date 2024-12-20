#pragma once

#include "math/gfxm.hpp"


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
    float depth;
    CONTACT_POINT_TYPE type;
};

struct RayHitPoint {
    gfxm::vec3 point;
    gfxm::vec3 normal;
    float distance;
};

struct SweepContactPoint {
    gfxm::vec3 contact;
    gfxm::vec3 normal;
    gfxm::vec3 sweep_contact_pos;
    float distance_traveled;
    CONTACT_POINT_TYPE type;
};
