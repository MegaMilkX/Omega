#pragma once

#include "common/collision/collision_manifold.hpp"
#include "common/math/gfxm.hpp"
#include "common/collision/bullet_physics/btVector3.h"

class CollisionWorld;

typedef btScalar dMatrix3[4 * 3];

int dBoxBox2(const btVector3& p1, const dMatrix3 R1,
    const btVector3& side1, const btVector3& p2,
    const dMatrix3 R2, const btVector3& side2,
    btVector3& normal, btScalar* depth, int* return_code,
    int maxc, CollisionWorld* world, CollisionManifold* output
);