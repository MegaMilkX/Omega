#include "sphere_capsule.hpp"

#include "collision/collision_triangle_mesh.hpp"
#include "collision/collider.hpp"

bool intersectSweepSphereTriangleMesh(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const CollisionTriangleMesh* mesh,
    SweepContactPoint& scp
) {
    TriangleMeshSweepSphereTestContext ctx;
    ctx.hasHit = false;
    ctx.pt.distance_traveled = INFINITY;
    mesh->sweepSphereTest(from, to, sweep_radius, &ctx, &TriangleMeshSweepSphereTestClosestCb);
    scp = ctx.pt;
    return ctx.hasHit;
}

bool intersectSweptSphereConvexMesh(
    const gfxm::vec3& from,
    const gfxm::vec3& to,
    float sweep_radius,
    const CollisionConvexMesh* mesh,
    SweepContactPoint& scp
) {
    ConvexMeshSweptSphereTestContext ctx;
    ctx.hasHit = false;
    ctx.pt.distance_traveled = INFINITY;
    mesh->sweptSphereTest(from, to, sweep_radius, &ctx, &ConvexMeshSweptSphereTestClosestCb);
    scp = ctx.pt;
    return ctx.hasHit;
}