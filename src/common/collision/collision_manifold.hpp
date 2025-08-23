#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "collision/collider.hpp"
#include "collision_contact_point.hpp"


class CollisionManifold {
public:
    ContactPoint* points = 0;
    int           point_count = 0;
    ContactPoint* old_points = 0;
    int           old_point_count = 0;
    Collider* collider_a = 0;
    Collider* collider_b = 0;
    gfxm::vec3 normal;
    float depth = .0f;
};