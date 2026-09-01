#pragma once

#include "math/gfxm.hpp"
#include "collision/collision_contact_point.hpp"


bool sweepSphereSphere(
    float sphere_radius, const gfxm::vec3& sphere_pos,
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    SweepContactPoint& scp
);

bool sweepSphereTriangle(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    SweepContactPoint& out_scp
);

bool sweepCapsuleTriangle(
    const gfxm::vec3& A, const gfxm::vec3& B, float radius,
    const gfxm::vec3& V,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    SweepContactPoint& out_scp
);


class CollisionTriangleMesh;
class phyConvexMesh;

bool sweepSphereTriangleMesh(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const CollisionTriangleMesh* mesh,
    SweepContactPoint& scp
);
bool sweepCapsuleTriangleMesh(
    const gfxm::vec3& capA, const gfxm::vec3& capB, float radius, const gfxm::vec3& V,
    const CollisionTriangleMesh* mesh, SweepContactPoint& scp
);
bool sweepCapsuleConvexMesh(
    const gfxm::vec3& capA, const gfxm::vec3& capB, float radius, const gfxm::vec3& V,
    const phyConvexMesh* mesh, SweepContactPoint& scp
);


bool sweepSphereConvexMesh(
    const gfxm::vec3& from,
    const gfxm::vec3& to,
    float sweep_radius,
    const phyConvexMesh* mesh,
    SweepContactPoint& scp
);

class phyHeightfieldShape;
bool sweepSphereHeightfield(
    const gfxm::vec3& from,
    const gfxm::vec3& to,
    float sweep_radius,
    const phyHeightfieldShape* heightfield,
    SweepContactPoint& scp
);

bool sweepCapsuleHeightfield(
    const gfxm::vec3& capA, const gfxm::vec3& capB, float radius, const gfxm::vec3& V,
    const phyHeightfieldShape* heightfield,
    SweepContactPoint& scp,
    const gfxm::mat4& shape_transform // for debug draw
);


