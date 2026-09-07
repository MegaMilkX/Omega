#pragma once

#include "math/gfxm.hpp"
#include "collision/collision_world.hpp"


constexpr float OVERBOUNCE = 1.0f;

gfxm::vec3 clipVelocity(const gfxm::vec3& V, const gfxm::vec3& N, float overbounce = OVERBOUNCE);

bool slideMoveYCapsule(
    phyWorld* world, float dt,
    const gfxm::vec3& P0, float capHeight, float capRadius,
    gfxm::vec3& V_out, gfxm::vec3& velo, gfxm::vec3& grav_velo
);