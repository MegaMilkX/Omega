#pragma once

#include <array>
#include "math/gfxm.hpp"
#include "collision/collision_contact_point.hpp"
#include "collision/intersection/capsule_capsule.hpp"
#include "debug_draw/debug_draw.hpp"

inline bool intersectionSphereCapsule(
    float sphere_radius, const gfxm::vec3& sphere_pos, float capsule_radius, float capsule_height,
    const gfxm::mat4&capsule_transform, ContactPoint& cp
) {
    gfxm::vec3 A = gfxm::vec3(.0f, capsule_height * .5f, .0f);
    gfxm::vec3 B = gfxm::vec3(.0f, -capsule_height * .5f, .0f);
    gfxm::vec3 P_inv = gfxm::inverse(capsule_transform) * gfxm::vec4(sphere_pos, 1.0f);
    gfxm::vec3 closest_pt_on_line_lcl = gfxm::vec3(
        .0f,
        gfxm::_min(capsule_height * .5f, gfxm::_max(-capsule_height * .5f, P_inv.y)),
        .0f
    );

    gfxm::vec3 cap_sphere_pos = capsule_transform * gfxm::vec4(closest_pt_on_line_lcl, 1.0f);

    gfxm::vec3 norm = cap_sphere_pos - sphere_pos;
    float center_distance = gfxm::length(norm);
    float radius_distance = sphere_radius + capsule_radius;
    float distance = center_distance - radius_distance;
    if (distance <= FLT_EPSILON) {
        gfxm::vec3 normal_a = norm / center_distance;
        gfxm::vec3 normal_b = -normal_a;
        gfxm::vec3 pt_a = normal_a * sphere_radius + sphere_pos;
        gfxm::vec3 pt_b = normal_b * capsule_radius + cap_sphere_pos;

        cp.point_b = pt_b;
        cp.normal_b = normal_a;
        cp.point_a = pt_a;
        cp.normal_a = normal_b;
        cp.depth = distance;
        return true;
    }
    return false;
}

inline bool intersectionSphereCapsule(
    float sphere_radius, const gfxm::vec3& sphere_pos,
    const gfxm::vec3& capsuleBegin, const gfxm::vec3& capsuleEnd, float capsule_radius,
    ContactPoint& cp
) {
    gfxm::vec3 A = capsuleBegin;
    gfxm::vec3 B = capsuleEnd;
    gfxm::vec3 P = sphere_pos;
    
    auto AB = B - A;
    auto AP = P - A;
    float lenSqrAB = AB.length2();
    float t = (AP.x * AB.x + AP.y * AB.y + AP.z * AB.z) / lenSqrAB;
    gfxm::vec3 closest_pt_on_line = gfxm::lerp(A, B, gfxm::clamp(t, .0f, 1.f));

    gfxm::vec3 norm = closest_pt_on_line - sphere_pos;
    float center_distance = gfxm::length(norm);
    float radius_distance = sphere_radius + capsule_radius;
    float distance = center_distance - radius_distance;
    if (distance <= FLT_EPSILON) {
        gfxm::vec3 normal_a = norm / center_distance;
        gfxm::vec3 normal_b = -normal_a;
        gfxm::vec3 pt_a = normal_a * sphere_radius + sphere_pos;
        gfxm::vec3 pt_b = normal_b * capsule_radius + closest_pt_on_line;

        cp.point_b = pt_b;
        cp.normal_b = normal_a;
        cp.point_a = pt_a;
        cp.normal_a = normal_b;
        cp.depth = distance;
        return true;
    }
    return false;
}

inline bool intersectionSweepSphereSphere(
    float sphere_radius, const gfxm::vec3& sphere_pos,
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    SweepContactPoint& scp
) {
    gfxm::vec3 A = from;
    gfxm::vec3 B = to;
    gfxm::vec3 P = sphere_pos;
    
    auto AB = B - A;
    auto AP = P - A;
    float lenSqrAB = AB.length2();
    float t = (AP.x * AB.x + AP.y * AB.y + AP.z * AB.z) / lenSqrAB;
    t = gfxm::clamp(t, .0f, 1.f);
    gfxm::vec3 closest_pt_on_line = gfxm::lerp(A, B, t);

    gfxm::vec3 norm = closest_pt_on_line - sphere_pos;
    float center_distance = gfxm::length(norm);
    float radius_distance = sphere_radius + sweep_radius;
    float distance = center_distance - radius_distance;
    if (distance <= FLT_EPSILON) {
        gfxm::vec3 normal_a = norm / center_distance;
        gfxm::vec3 normal_b = -normal_a;
        gfxm::vec3 pt_a = normal_a * sphere_radius + sphere_pos;
        gfxm::vec3 pt_b = normal_b * sweep_radius + closest_pt_on_line;

        float sideA = (sphere_pos - closest_pt_on_line).length();
        float sideB = sphere_radius + sweep_radius;
        float offs = gfxm::sqrt(gfxm::pow2(sideB) - gfxm::pow2(sideA));
        gfxm::vec3 sweep_stop_pos = closest_pt_on_line - gfxm::normalize(closest_pt_on_line - from) * offs;
        scp.sweep_contact_pos = sweep_stop_pos;
        scp.contact = sweep_stop_pos + gfxm::normalize(sphere_pos - sweep_stop_pos) * sweep_radius;
        scp.distance_traveled = (to - from).length() * t - offs;
        scp.normal = gfxm::normalize(scp.contact - sphere_pos);
        scp.type = CONTACT_POINT_TYPE::DEFAULT;
        return true;
    }
    return false;
}

#include "math/intersection.hpp"

inline bool intersectionSweepSphereTriangle(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    SweepContactPoint& scp
) {
    std::array<gfxm::vec3, 3> e{ p1 - p0, p2 - p1, p0 - p2 };
    gfxm::vec3 PN = gfxm::cross(e[0], e[1]);
    PN = gfxm::normalize(PN);
    float d = gfxm::dot((from - to), PN);
    if (d < .0f) {
        PN = -PN;
    }
    float PD = gfxm::dot(PN, p0 + PN * sweep_radius);
    gfxm::vec3 LN = (to - from);
    float t = .0f;
    bool is_parallel = false;
    if (!gfxm::intersect_line_plane_t(from, LN, PN, PD, t)) {
        // TODO: Handle properly
        // check if start inside triangle
        // if not, check the edges
        is_parallel = true;
    }

    gfxm::vec3 pt;
    if (!is_parallel) {
        t = gfxm::clamp(t, .0f, 1.f);
        gfxm::vec3 pt = gfxm::lerp(from, to, t);
        //dbgDrawSphere(pt, sweep_radius, 0xFF9900FF);
        //float dist = gfxm::dot(PN, (pt - p0));
        gfxm::vec3 c0 = gfxm::cross(pt - p0 + PN, e[0]);
        gfxm::vec3 c1 = gfxm::cross(pt - p1 + PN, e[1]);
        gfxm::vec3 c2 = gfxm::cross(pt - p2 + PN, e[2]);
        bool is_inside = gfxm::dot(c0, PN) <= 0 && gfxm::dot(c1, PN) <= 0 && gfxm::dot(c2, PN) <= 0;

        if (is_inside) {
            scp.sweep_contact_pos = pt;
            scp.distance_traveled = (pt - from).length();
            scp.contact = pt - PN * sweep_radius;
            scp.normal = PN;
            scp.type = CONTACT_POINT_TYPE::TRIANGLE_FACE;
            //dbgDrawSphere(pt, sweep_radius, 0xFF00FF77);
            //dbgDrawArrow(pt, PN, 0xFFFF7700);
            return true;
        }
    }

    std::array<gfxm::vec3, 6> pts_{ p0, p1, p1, p2, p2, p0 };
    float min_dist2 = INFINITY;
    gfxm::vec3 closest_on_edge;
    gfxm::vec3 closest_on_trace;
    gfxm::vec3 closest_edge;
    int closest_edge_id = 0;
    for (int i = 0; i < 3; ++i) {
        gfxm::vec3 on_edge;
        gfxm::vec3 on_trace;
        float dist2 = closestPointSegmentSegment(pts_[i * 2], pts_[i * 2 + 1], from, to, on_edge, on_trace);
        if (dist2 < min_dist2) {
            min_dist2 = dist2;
            closest_on_edge = on_edge;
            closest_on_trace = on_trace;
            closest_edge = (pts_[i * 2 + 1] - pts_[i * 2]);
            closest_edge_id = i;
        }
    }

    float points_dist = (closest_on_trace - closest_on_edge).length();
    if (points_dist > sweep_radius) {
        return false;
    }
    float offset_angle_compensation = .0f;
    {
        gfxm::vec3 trace = (to - from);
        float d = fabsf(gfxm::dot(trace, closest_edge));
        float lensq1 = trace.length2();
        float lensq2 = closest_edge.length2();
        float costheta = d / sqrt(lensq1 * lensq2);
        float sintheta = sqrt(1.f - gfxm::pow2(costheta));
        offset_angle_compensation = (sweep_radius) / sintheta;
    }
    float difflen = gfxm::sqrt(gfxm::pow2(offset_angle_compensation) - gfxm::pow2(points_dist));// gfxm::sqrt(gfxm::pow2(offset_angle_compensation) - gfxm::pow2(points_dist));
    float maxOffs = (closest_on_trace - from).length();
    pt = closest_on_trace + gfxm::normalize(from - to) * gfxm::_min(difflen, maxOffs);
    std::array<gfxm::vec3, 3> p{ p0, p1, p2 };
    {
        float lensq = e[closest_edge_id].length2();
        float t = gfxm::dot(e[closest_edge_id], (pt - p[closest_edge_id])) / lensq;
        if (t < .0f || t > 1.f) {
            t = gfxm::clamp(t, .0f, 1.f);
            closest_on_edge = gfxm::lerp(p[closest_edge_id], p[(closest_edge_id + 1) % 3], t);
            float lensq_ = (to - pt).length2();
            float tt = gfxm::dot(to - pt, to - closest_on_edge) / lensq_;
            pt = gfxm::lerp(to, pt, tt);

            float dist = (closest_on_edge - pt).length();
            pt = pt + gfxm::normalize(from - to) * gfxm::sqrt(gfxm::pow2(sweep_radius) - gfxm::pow2(dist));
        } else {
            closest_on_edge = gfxm::lerp(p[closest_edge_id], p[(closest_edge_id + 1) % 3], t);
        }
    }
    scp.sweep_contact_pos = pt;
    scp.distance_traveled = (pt - from).length();
    scp.contact = closest_on_edge;
    scp.normal = gfxm::normalize(pt - closest_on_edge);
    scp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
    /*
    dbgDrawSphere(closest_on_edge, .1f, 0xFF00FF00);
    dbgDrawSphere(pt, .1f, 0xFFFF00FF);
    dbgDrawSphere(pt, sweep_radius, 0xFF00FF77);
    dbgDrawArrow(pt, PN, 0xFFFF7700);*/
    return true;
}

struct TriangleMeshSweepSphereTestContext {
    SweepContactPoint pt;
    bool hasHit = false;
};
inline void TriangleMeshSweepSphereTestClosestCb(void* context, const SweepContactPoint& scp) {
    TriangleMeshSweepSphereTestContext* ctx = (TriangleMeshSweepSphereTestContext*)context;
    ctx->hasHit = true;
    if (scp.distance_traveled < ctx->pt.distance_traveled) {
        ctx->pt = scp;
    }
}
class CollisionTriangleMesh;
bool intersectSweepSphereTriangleMesh(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const CollisionTriangleMesh* mesh,
    SweepContactPoint& scp
);