#include "sphere_capsule.hpp"

#include "collision/collision_triangle_mesh.hpp"

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
