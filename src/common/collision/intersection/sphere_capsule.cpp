#include "sphere_capsule.hpp"

#include "collision/collider.hpp"


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


#include "collision/convex_mesh.hpp"

bool intersectSweptSphereConvexMesh(
    const gfxm::vec3& from,
    const gfxm::vec3& to,
    float sweep_radius,
    const phyConvexMesh* mesh,
    SweepContactPoint& scp
) {
    ConvexMeshSweptSphereTestContext ctx;
    ctx.hasHit = false;
    ctx.pt.distance_traveled = INFINITY;
    mesh->sweptSphereTest(from, to, sweep_radius, &ctx, &ConvexMeshSweptSphereTestClosestCb);
    scp = ctx.pt;
    return ctx.hasHit;
}


#include "collision/shape/heightfield.hpp"

bool intersectSweptSphereHeightfield(
    const gfxm::vec3& from,
    const gfxm::vec3& to,
    float sweep_radius,
    const phyHeightfieldShape* heightfield,
    SweepContactPoint& scp
) {
    float minx = gfxm::_min(from.x - sweep_radius, to.x - sweep_radius);
    float maxx = gfxm::_max(from.x + sweep_radius, to.x + sweep_radius);
    float minz = gfxm::_min(from.z - sweep_radius, to.z - sweep_radius);
    float maxz = gfxm::_max(from.z + sweep_radius, to.z + sweep_radius);
    float miny = gfxm::_min(from.y - sweep_radius, to.y - sweep_radius);
    float maxy = gfxm::_max(from.y + sweep_radius, to.y + sweep_radius);

    float cell_width = heightfield->getCellWidth();
    float cell_depth = heightfield->getCellDepth();

    int sminx = gfxm::clamp(minx / cell_width, .0f, heightfield->getSampleCountX());
    int smaxx = gfxm::clamp(1 + maxx / cell_width, .0f, heightfield->getSampleCountX());
    int sminz = gfxm::clamp(minz / cell_depth, .0f, heightfield->getSampleCountZ());
    int smaxz = gfxm::clamp(1 + maxz / cell_depth, .0f, heightfield->getSampleCountZ());

    if (sminx == smaxx || sminz == smaxz) {
        return false;
    }

    {
        gfxm::vec3 p0(minx, .0f, minz);
        gfxm::vec3 p1(maxx, .0f, minz);
        gfxm::vec3 p2(maxx, .0f, maxz);
        gfxm::vec3 p3(minx, .0f, maxz);

        dbgDrawLine(p0, p1, 0xFFFF00FF);
        dbgDrawLine(p1, p2, 0xFFFF00FF);
        dbgDrawLine(p2, p3, 0xFFFF00FF);
        dbgDrawLine(p3, p0, 0xFFFF00FF);
    }
    {
        gfxm::vec3 p0(sminx * cell_width, .0f, sminz * cell_depth);
        gfxm::vec3 p1(smaxx * cell_width, .0f, sminz * cell_depth);
        gfxm::vec3 p2(smaxx * cell_width, .0f, smaxz * cell_depth);
        gfxm::vec3 p3(sminx * cell_width, .0f, smaxz * cell_depth);

        dbgDrawLine(p0, p1, 0xFF0000FF);
        dbgDrawLine(p1, p2, 0xFF0000FF);
        dbgDrawLine(p2, p3, 0xFF0000FF);
        dbgDrawLine(p3, p0, 0xFF0000FF);
    }

    int scount_x = heightfield->getSampleCountX();

    const float* samples = heightfield->getData();
    float min_distance = gfxm::length(to - from);
    bool hasHit = false;
    for (int z = sminz; z < smaxz; ++z) {
        for (int x = sminx; x < smaxx; ++x) {
            float h0 = samples[x + z * scount_x];
            float h1 = samples[x + (z + 1) * scount_x];
            float h2 = samples[x + 1 + (z + 1) * scount_x];
            float h3 = samples[x + 1 + z * scount_x];

            float hmin = gfxm::_min(gfxm::_min(h0, h1), gfxm::_min(h2, h3));
            float hmax = gfxm::_max(gfxm::_max(h0, h1), gfxm::_max(h2, h3));

            if ((hmin < miny && hmax < miny)
                || (hmin > maxy && hmax > maxy)
            ) {
                continue;
            }

            gfxm::vec3 p0(x * cell_width, h0, z * cell_depth);
            gfxm::vec3 p1(x * cell_width, h1, (z + 1) * cell_depth);
            gfxm::vec3 p2((x + 1) * cell_width, h2, (z + 1) * cell_depth);
            gfxm::vec3 p3((x + 1) * cell_width, h3, z * cell_depth);
            
            dbgDrawLine(p0, p1, 0xFFFFFFFF);
            dbgDrawLine(p1, p2, 0xFFFFFFFF);
            dbgDrawLine(p3, p0, 0xFFFFFFFF);

            dbgDrawLine(p2, p3, 0xFFFFFFFF);
            dbgDrawLine(p3, p0, 0xFFFFFFFF);
            dbgDrawLine(p0, p2, 0xFFFFFFFF);
            
            SweepContactPoint scp2;
            if (intersectionSweepSphereTriangle(from, to, sweep_radius, p0, p1, p2, scp2)) {
                if (min_distance > scp2.distance_traveled) {
                    min_distance = scp2.distance_traveled;
                    scp = scp2;
                    hasHit = true;
                    scp.prop = phySurfaceProp{ COLLISION_SURFACE_GRAVEL };
                }
            }
            if (intersectionSweepSphereTriangle(from, to, sweep_radius, p2, p3, p0, scp2)) {
                if (min_distance > scp2.distance_traveled) {
                    min_distance = scp2.distance_traveled;
                    scp = scp2;
                    hasHit = true;
                    scp.prop = phySurfaceProp{ COLLISION_SURFACE_GRAVEL };
                }
            }
        }
    }
    
    return hasHit;
}

