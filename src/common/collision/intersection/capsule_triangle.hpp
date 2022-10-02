#pragma once

#include "math/gfxm.hpp"
#include "util/util.hpp"

#include "debug_draw/debug_draw.hpp"

inline gfxm::vec3 closestPointOnTriangle(
    const gfxm::vec3& point, const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2
) {
    //gfxm::vec3 point0 = point - N * dist;
}

inline bool intersectCapsuleTriangle(
    float capsule_radius, float capsule_height, const gfxm::mat4& capsule_transform,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    ContactPoint& cp
) {
    { // Check if triangle is degenerate (zero area)
        gfxm::vec3 c = gfxm::cross(p1 - p0, p2 - p0);
        float d = c.length();
        if (d <= FLT_EPSILON) {
            assert(false);
            return false;
        }
    }

    gfxm::vec3 tip = capsule_transform * gfxm::vec4(.0f, capsule_height * .5f + capsule_radius, .0f, 1.f);
    gfxm::vec3 base = capsule_transform * gfxm::vec4(.0f, -capsule_height * .5f - capsule_radius, .0f, 1.f);
    gfxm::vec3 capsule_normal = gfxm::normalize(tip - base);
    gfxm::vec3 line_end_offset = capsule_normal * capsule_radius;
    gfxm::vec3 A = base + line_end_offset;
    gfxm::vec3 B = tip - line_end_offset;
    gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1 - p0, p2 - p0));

    gfxm::vec3 line_plane_intersection;
    float dot_test = fabsf(gfxm::dot(N, capsule_normal));
    if (dot_test > FLT_EPSILON) {
        float t = gfxm::dot(N, (p0 - base) / dot_test);
        line_plane_intersection = base + capsule_normal * t;
    } else {
        //line_plane_intersection = (p0 + p1 + p2) / 3.0f;
        line_plane_intersection = p0;
    }
    gfxm::vec3 ref_pt;

    gfxm::vec3 c0 = gfxm::cross(line_plane_intersection - p0, p1 - p0);
    gfxm::vec3 c1 = gfxm::cross(line_plane_intersection - p1, p2 - p1);
    gfxm::vec3 c2 = gfxm::cross(line_plane_intersection - p2, p0 - p2);
    bool inside = gfxm::dot(c0, N) <= 0 && gfxm::dot(c1, N) <= 0 && gfxm::dot(c2, N) <= 0;
    
    if (inside) {
        ref_pt = line_plane_intersection;
    } else {
        gfxm::vec3 point1 = closestPointOnSegment(line_plane_intersection, p0, p1);
        gfxm::vec3 v1 = line_plane_intersection - point1;
        float distsq = gfxm::dot(v1, v1);
        float best_dist = distsq;
        ref_pt = point1;

        gfxm::vec3 point2 = closestPointOnSegment(line_plane_intersection, p1, p2);
        gfxm::vec3 v2 = line_plane_intersection - point2;
        distsq = gfxm::dot(v2, v2);
        if (distsq < best_dist) {
            ref_pt = point2;
            best_dist = distsq;
        }

        gfxm::vec3 point3 = closestPointOnSegment(line_plane_intersection, p2, p0);
        gfxm::vec3 v3 = line_plane_intersection - point3;
        distsq = gfxm::dot(v3, v3);
        if (distsq < best_dist) {
            ref_pt = point3;
            best_dist = distsq;
        }
    }

    gfxm::vec3 center = closestPointOnSegment(ref_pt, A, B);
    //dbgDrawSphere(center, capsule_radius, DBG_COLOR_GREEN);
    //dbgDrawCross(gfxm::translate(gfxm::mat4(1.0f), ref_pt), .3f, DBG_COLOR_BLUE);
    //dbgDrawCross(gfxm::translate(gfxm::mat4(1.0f), line_plane_intersection), .3f, DBG_COLOR_RED);

    float dist = gfxm::dot(center - p0, N);
    if (dist < -capsule_radius || dist > capsule_radius) {
        return false;
    }
    gfxm::vec3 point0 = center - N * dist;
    c0 = gfxm::cross(point0 - p0, p1 - p0);
    c1 = gfxm::cross(point0 - p1, p2 - p1);
    c2 = gfxm::cross(point0 - p2, p0 - p2);
    inside = gfxm::dot(c0, N) <= 0 && dot(c1, N) <= 0 && dot(c2, N) <= 0;

    float radiussq = capsule_radius * capsule_radius;
    gfxm::vec3 point1 = closestPointOnSegment(center, p0, p1);
    gfxm::vec3 v1 = center - point1;
    float distsq1 = gfxm::dot(v1, v1);
    bool intersects = distsq1 < radiussq;

    gfxm::vec3 point2 = closestPointOnSegment(center, p1, p2);
    gfxm::vec3 v2 = center - point2;
    float distsq2 = gfxm::dot(v2, v2);
    intersects |= distsq2 < radiussq;

    gfxm::vec3 point3 = closestPointOnSegment(center, p2, p0);
    gfxm::vec3 v3 = center - point3;
    float distsq3 = gfxm::dot(v3, v3);
    intersects |= distsq3 < radiussq;

    if (inside || intersects) {
        gfxm::vec3 best_point = point0;
        gfxm::vec3 intersection_vec;
        if (inside) {
            intersection_vec = center - point0;
        } else {
            gfxm::vec3 d = center - point1;
            float best_distsq = gfxm::dot(d, d);
            best_point = point1;
            intersection_vec = d;

            d = center - point2;
            float distsq = gfxm::dot(d, d);
            if (distsq < best_distsq) {
                best_distsq = distsq;
                best_point = point2;
                intersection_vec = d;
            }

            d = center - point3;
            distsq = gfxm::dot(d, d);
            if (distsq < best_distsq) {
                best_distsq = distsq;
                best_point = point3;
                intersection_vec = d;
            }
        }

        float len = gfxm::length(intersection_vec);
        if (len <= FLT_EPSILON) {
            // TODO: Find out what exactly happens
            // Maybe add bias?
            return false;
        }
        gfxm::vec3 penetration_normal = intersection_vec / len;
        float penetration_depth = capsule_radius - len;
        
        cp.point_a = gfxm::normalize(best_point - center) * capsule_radius + center;
        cp.point_b = best_point;
        cp.normal_b = penetration_normal;
        cp.normal_a = -cp.normal_b;
        cp.depth = penetration_depth;
        if (inside) {
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_FACE;
        } else {
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
        }
        //dbgDrawSphere(center, capsule_radius, DBG_COLOR_GREEN);
        //dbgDrawLine(center, (p0 + p1 + p2) / 3.f, DBG_COLOR_RED);
        //dbgDrawSphere(line_plane_intersection, .1f, DBG_COLOR_RED);
        return true;
    }
    
    /*
    float distance = gfxm::length(ref_pt - center);
    float penetration = capsule_radius - distance;
    
    
    if (penetration > FLT_EPSILON) {
        dbgDrawSphere(center, capsule_radius, DBG_COLOR_GREEN);
        dbgDrawSphere(ref_pt, .2f, DBG_COLOR_BLUE);
        dbgDrawSphere(line_plane_intersection, .1f, DBG_COLOR_RED);

        cp.point_a = ref_pt;
        cp.point_b = ref_pt;
        cp.normal_b = (center - ref_pt) / distance;
        cp.normal_a = -cp.normal_b;
        cp.depth = penetration;
        return true;
    }*/

    return false;
}