#pragma once

#include <array>
#include <assert.h>
#include <format>
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
    SweepContactPoint& out_scp
) {
    // Find triangle normal
    gfxm::vec3 pts[3] = {
        p0, p1, p2
    };
    gfxm::vec3 triangle_edges[3] = {
        p1 - p0, p2 - p1, p0 - p2
    };
    gfxm::vec3 PN = gfxm::cross(triangle_edges[0], triangle_edges[1]);
    PN = gfxm::normalize(PN);
    float side = 1.f;
    {
        float d = gfxm::dot((from - p0), PN);
        if (d < .0f) {
            PN = -PN;
            side = -side;
        }
    }

    // Test with triangle face
    const float PD = gfxm::dot(PN, p0 + PN * sweep_radius);
    const gfxm::vec3 V = (to - from);
    const float Vlen = V.length();
    float denom = gfxm::dot(PN, V);
    bool is_parallel = false;
    float t = .0f;
    if (abs(denom) <= FLT_EPSILON) {
        is_parallel = true;
    } else {
        gfxm::vec3 vec = PN * PD - from;
        t = gfxm::dot(vec, PN) / denom;
    }

    // TODO: Should handle parallel cases
    if (!is_parallel) {
        float distance_to_plane = sweep_radius;
        float distance_from_to_plane = gfxm::dot(PN, from - p0) / (PN.x * PN.x + PN.y * PN.y + PN.z * PN.z);
        gfxm::vec3 from_on_plane = from - PN * distance_from_to_plane;
        if (distance_from_to_plane <= sweep_radius) {
            t = .0f;
            distance_to_plane = distance_from_to_plane;
        }

        gfxm::vec3 intersection_pt = from + V * t;
        gfxm::vec3 c0 = gfxm::cross(intersection_pt - p0 + PN, triangle_edges[0]);
        gfxm::vec3 c1 = gfxm::cross(intersection_pt - p1 + PN, triangle_edges[1]);
        gfxm::vec3 c2 = gfxm::cross(intersection_pt - p2 + PN, triangle_edges[2]);
        bool is_inside = gfxm::dot(c0, PN) * side <= 0 && gfxm::dot(c1, PN) * side <= 0 && gfxm::dot(c2, PN) * side <= 0;
        if (is_inside && t >= .0f && t <= 1.f) {
            out_scp.sweep_contact_pos = intersection_pt;
            out_scp.distance_traveled = (intersection_pt - from).length();
            out_scp.contact = intersection_pt - PN * distance_to_plane;
            out_scp.normal = PN;
            out_scp.type = CONTACT_POINT_TYPE::TRIANGLE_FACE;
            //dbgDrawSphere(out_scp.contact, .1f, 0xFF00FFFF);
            //dbgDrawText(intersection_pt, "face");
            return true;
        }
    }

    // Triangle corners
    float t_corner = FLT_MAX;
    float closest_corner_dist = FLT_MAX;
    int corner_id = -1;
    for (int i = 0; i < 3; ++i) {
        float dist_from_to_corner = (pts[i] - from).length();
        if (dist_from_to_corner <= sweep_radius && dist_from_to_corner < closest_corner_dist) {
            t_corner = .0f;
            corner_id = i;
            closest_corner_dist = dist_from_to_corner;
            continue;
        }

        float t = gfxm::dot(gfxm::normalize(V), pts[i] - from) / Vlen;
        gfxm::vec3 closest_on_trace = from + V * t;
        float dist = gfxm::length(pts[i] - closest_on_trace);
        if (dist > sweep_radius) {
            continue;
        }

        float Clen = gfxm::sqrt(powf(sweep_radius, 2.f) - powf(dist, 2.f));
        float t2 = t - Clen / Vlen;
        //dbgDrawText(pts[i], std::format("t: {:.2f}, t2: {:.2f}", t, t2));
        if (dist <= sweep_radius && t2 < t_corner && dist < closest_corner_dist && t2 >= .0f) {
            t_corner = t2;
            closest_corner_dist = dist;
            corner_id = i;
        }
    }

    // Triangle edges
    float closest_edge_dist = sweep_radius;
    gfxm::vec3 pt_on_edge;
    int edge_idx = -1;
    float t_edge = FLT_MAX;
    for (int i = 0; i < 3; ++i) {
        gfxm::vec3 on_edge;
        gfxm::vec3 on_trace;
        closestPointSegmentSegment(pts[i], pts[(i + 1) % 3], from, to, on_edge, on_trace);
        //dbgDrawSphere(on_edge, .1f, 0xFFFF00FF);
        //dbgDrawSphere(on_trace, .1f, 0xFF00FF00);
        gfxm::vec3 A = on_trace - on_edge;
        gfxm::vec3 ea = pts[i];
        gfxm::vec3 eb = pts[(i + 1) % 3];
        gfxm::vec3 edge_v = eb - ea;
        float dist = A.length();

        gfxm::mat3 m3 = make_orientation_yz(triangle_edges[(i + 1) % 3], triangle_edges[i]);
        gfxm::vec2 from2 = gfxm::project_point_xy(m3, ea, from);
        gfxm::vec2 to2 = gfxm::project_point_xy(m3, ea, to);
        gfxm::vec3 f3 = gfxm::unproject_point_xy(from2, on_edge, m3[0], m3[1]);
        gfxm::vec3 t3 = gfxm::unproject_point_xy(to2, on_edge, m3[0], m3[1]);

        float Clen = gfxm::sqrt(powf(sweep_radius, 2.f) - powf(dist, 2.f));
        float t = gfxm::length(on_trace - f3) - Clen;

        gfxm::vec3 pt = f3 + gfxm::normalize(t3 - f3) * t;
        gfxm::vec2 pt2 = gfxm::project_point_xy(m3, ea, pt);
        gfxm::vec3 pt3 = gfxm::unproject_point_xy(pt2, ea, m3[0], m3[1]);
        //dbgDrawLine(pt3, pt3 + (edge_v), DBG_COLOR_GREEN);

        float t2 = .0f;
        closestPointSegmentSegment(pt3, pt3 + edge_v, from, to, on_edge, on_trace, 0, &t2);

        float d = gfxm::dot(gfxm::normalize(edge_v), on_trace - ea) / gfxm::length(edge_v);
        if (d > 1.f || d < .0f) {
            continue;
        }
        gfxm::vec3 closest_on_edge = ea + edge_v * d;

        if (t2 < t_edge) {
            t_edge = t2;
            edge_idx = i;
            pt_on_edge = closest_on_edge;
            //dbgDrawText(pt_on_edge, std::format("t2: {:.3f}", t2));
        }
    }

    if (corner_id < 0 && edge_idx < 0) {
        return false;
    }

    if (t_corner < t_edge) {
        gfxm::vec3 pt = from + V * t_corner;
        out_scp.sweep_contact_pos = pt;
        out_scp.distance_traveled = t_corner * V.length();
        out_scp.contact = pts[corner_id];
        out_scp.normal = gfxm::normalize(pt - pts[corner_id]);
        out_scp.type = CONTACT_POINT_TYPE::TRIANGLE_CORNER;
        //dbgDrawText(pt, "corner");
        return true;
    } else {
        gfxm::vec3 pt = from + V * t_edge;
        out_scp.sweep_contact_pos = pt;
        out_scp.distance_traveled = t_edge * V.length();
        out_scp.contact = pt_on_edge;
        out_scp.normal = gfxm::normalize(pt - pt_on_edge);
        out_scp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
        //dbgDrawText(pt, "edge");
        return true;
    }

    return false;
}

struct TriangleMeshSweepSphereTestContext {
    SweepContactPoint pt;
    bool hasHit = false;
};
inline void TriangleMeshSweepSphereTestClosestCb(void* context, const SweepContactPoint& scp) {
    TriangleMeshSweepSphereTestContext* ctx = (TriangleMeshSweepSphereTestContext*)context;
    ctx->hasHit = true;    

    if (scp.distance_traveled <= ctx->pt.distance_traveled) {
        if (scp.distance_traveled <= FLT_EPSILON) {
            if (ctx->pt.type == CONTACT_POINT_TYPE::TRIANGLE_FACE && scp.type != CONTACT_POINT_TYPE::TRIANGLE_FACE) {
                return;
            }
        }

        ctx->pt = scp;
    }
}
class CollisionTriangleMesh;
bool intersectSweepSphereTriangleMesh(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const CollisionTriangleMesh* mesh,
    SweepContactPoint& scp
);