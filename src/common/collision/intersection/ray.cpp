#include "ray.hpp"

#include "collision/collision_triangle_mesh.hpp"


bool intersectRayTriangle(
    const gfxm::ray& ray,
    const gfxm::vec3& p0,
    const gfxm::vec3& p1,
    const gfxm::vec3& p2,
    RayHitPoint& rhp
) {
    gfxm::vec3 cross = gfxm::cross(p1 - p0, p2 - p0);
    { // Check if triangle is degenerate (zero area)
        float d = cross.length();
        if (d <= FLT_EPSILON) {
            assert(false);
            return false;
        }
    }
    
    gfxm::vec3 N = gfxm::normalize(cross);

    gfxm::vec3 line_plane_intersection;
    float dot_test = (gfxm::dot(N, ray.direction));
    float t;
    if (fabsf(dot_test) > FLT_EPSILON) {
        t = gfxm::dot(N, (p0 - ray.origin) / dot_test);
        line_plane_intersection = ray.origin + ray.direction * t;
        if (t < .0f || t > 1.0f) {
            return false;
        }
    } else {
        return false;
    }

    gfxm::vec3 c0 = gfxm::cross(line_plane_intersection - p0, p1 - p0);
    gfxm::vec3 c1 = gfxm::cross(line_plane_intersection - p1, p2 - p1);
    gfxm::vec3 c2 = gfxm::cross(line_plane_intersection - p2, p0 - p2);
    bool inside = gfxm::dot(c0, N) <= 0 && gfxm::dot(c1, N) <= 0 && gfxm::dot(c2, N) <= 0;
    if (!inside) {
        return false;
    }

    rhp.point = line_plane_intersection;
    if (dot_test < .0f) {
        rhp.normal = N;
        rhp.distance = gfxm::length(ray.direction) * t;
    } else { // backface
        rhp.normal = -N;
        rhp.distance = gfxm::length(ray.direction) * t;
    }

    //dbgDrawSphere(line_plane_intersection, .1f, DBG_COLOR_RED);
    //dbgDrawLine(p0, p1, DBG_COLOR_RED);
    //dbgDrawLine(p1, p2, DBG_COLOR_RED);
    //dbgDrawLine(p2, p0, DBG_COLOR_RED);

    return true;
}



struct TriangleMeshRayTestContext {
    RayHitPoint pt;
    bool hasHit = false;
};

void TriangleMeshRayTestClosestCb(void* context, const RayHitPoint& rhp) {
    TriangleMeshRayTestContext* ctx = (TriangleMeshRayTestContext*)context;
    ctx->hasHit = true;
    if (rhp.distance < ctx->pt.distance) {
        ctx->pt = rhp;
    }
}

bool intersectRayTriangleMesh(
    const gfxm::ray& ray,
    const CollisionTriangleMesh* mesh,
    RayHitPoint& rhp
) {
    TriangleMeshRayTestContext ctx;
    ctx.hasHit = false;
    ctx.pt.distance = INFINITY;
    mesh->rayTest(ray, &ctx, &TriangleMeshRayTestClosestCb);
    rhp = ctx.pt;
    return ctx.hasHit;
}
