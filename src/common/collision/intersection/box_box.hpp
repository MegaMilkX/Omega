#pragma once

#include "math/gfxm.hpp"
#include "collision/collision_contact_point.hpp"


bool intersectBoxBox(
    const gfxm::vec3& half_extents_a,
    const gfxm::mat4& transform_a,
    const gfxm::vec3& half_extents_b,
    const gfxm::mat4& transform_b,
    ContactPoint& cp
);