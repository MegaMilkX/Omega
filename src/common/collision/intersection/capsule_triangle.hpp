#pragma once

#include "math/gfxm.hpp"
#include "util/util.hpp"

#include "debug_draw/debug_draw.hpp"

inline gfxm::vec3 closestPointOnTriangle(
    const gfxm::vec3& point, const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2
) {
    //gfxm::vec3 point0 = point - N * dist;
}

inline bool intersectSphereTriangle(
    float R, const gfxm::vec3& P,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    ContactPoint& cp
) {
    // Check if triangle is degenerate (zero area)
    {
        const gfxm::vec3 c = gfxm::cross(p1 - p0, p2 - p0);
        if (c.length() <= FLT_EPSILON) {
            return false;
        }
    }
    // Triangle normal
    const gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1 - p0, p2 - p0));

    // Face
    {
        float D = gfxm::dot(N, p0);
        float dist = gfxm::dot(N, P) - D;
        gfxm::vec3 closest_point_on_plane = P - dist * N;
        float d0 = gfxm::dot(N, gfxm::cross(p1 - p0, closest_point_on_plane - p0));
        float d1 = gfxm::dot(N, gfxm::cross(p2 - p1, closest_point_on_plane - p1));
        float d2 = gfxm::dot(N, gfxm::cross(p0 - p2, closest_point_on_plane - p2));
        if (d0 > 0 && d1 > 0 && d2 > 0 && dist <= R) {
            cp.point_a = gfxm::normalize(closest_point_on_plane - P) * R;
            cp.point_b = closest_point_on_plane;
            cp.normal_b = N;
            cp.normal_a = -cp.normal_b;
            cp.depth = R - dist;
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_FACE;
            return true;
        }
    }

    // Edges
    {
        float Rsq = R * R;
        gfxm::vec3 AB = p1 - p0;
        gfxm::vec3 BC = p2 - p1;
        gfxm::vec3 CA = p0 - p2;
        float t0 = gfxm::dot(AB, P - p0) / gfxm::dot(AB, AB);
        float t1 = gfxm::dot(BC, P - p1) / gfxm::dot(BC, BC);
        float t2 = gfxm::dot(CA, P - p2) / gfxm::dot(CA, CA);
        gfxm::vec3 C0 = p0 + AB * t0;
        gfxm::vec3 V0 = (P - C0);
        float D0sq = V0.length2();
        gfxm::vec3 C1 = p1 + BC * t1;
        gfxm::vec3 V1 = (P - C1);
        float D1sq = V1.length2();
        gfxm::vec3 C2 = p2 + CA * t2;
        gfxm::vec3 V2 = (P - C2);
        float D2sq = V2.length2();
        if(t0 >= .0f && t0 <= 1.f && D0sq <= Rsq) {
            cp.normal_a = gfxm::normalize(V0);
            cp.normal_b = -cp.normal_a;
            cp.point_a = cp.normal_b * R;
            cp.point_b = C0;
            cp.depth = R - sqrt(D0sq);
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
            return true;
        }
        if(t1 >= .0f && t1 <= 1.f && D1sq <= Rsq) {
            cp.normal_a = gfxm::normalize(V1);
            cp.normal_b = -cp.normal_a;
            cp.point_a = cp.normal_b * R;
            cp.point_b = C1;
            cp.depth = R - sqrt(D1sq);
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
            return true;
        }
        if(t2 >= .0f && t2 <= 1.f && D2sq <= Rsq) {
            cp.normal_a = gfxm::normalize(V2);
            cp.normal_b = -cp.normal_a;
            cp.point_a = cp.normal_b * R;
            cp.point_b = C2;
            cp.depth = R - sqrt(D2sq);
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
            return true;
        }
    }

    // Corners
    {
        gfxm::vec3 V0 = (P - p0);
        gfxm::vec3 V1 = (P - p1);
        gfxm::vec3 V2 = (P - p2);
        float D0 = V0.length();
        float D1 = V1.length();
        float D2 = V2.length();
        if (D0 <= R) {
            cp.normal_a = gfxm::normalize(V0);
            cp.normal_b = -cp.normal_a;
            cp.point_a = cp.normal_b * R;
            cp.point_b = p0;
            cp.depth = R - D0;
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_CORNER;
            return true;
        }
        if (D1 <= R) {
            cp.normal_a = gfxm::normalize(V1);
            cp.normal_b = -cp.normal_a;
            cp.point_a = cp.normal_b * R;
            cp.point_b = p1;
            cp.depth = R - D1;
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_CORNER;
            return true;
        }
        if (D2 <= R) {
            cp.normal_a = gfxm::normalize(V2);
            cp.normal_b = -cp.normal_a;
            cp.point_a = cp.normal_b * R;
            cp.point_b = p2;
            cp.depth = R - D2;
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_CORNER;
            return true;
        }
    }

    return false;
}

inline bool intersectCapsuleTriangle2(
    float capsule_radius, float capsule_height, const gfxm::mat4& capsule_transform,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    ContactPoint& cp
) {
    // Check if triangle is degenerate (zero area)
    {
        const gfxm::vec3 c = gfxm::cross(p1 - p0, p2 - p0);
        if (c.length() <= FLT_EPSILON) {
            return false;
        }
    }

    // Capsule points
    const gfxm::vec3 A = capsule_transform * gfxm::vec4(.0f, capsule_height * .5f, .0f, 1.f);
    const gfxm::vec3 B = capsule_transform * gfxm::vec4(.0f, -capsule_height * .5f, .0f, 1.f);
    // Triangle normal
    const gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1 - p0, p2 - p0));
    // Capsule normal
    const gfxm::vec3 Nc = gfxm::normalize(A - B);
    // Square radius
    const float radius_sq = capsule_radius * capsule_radius;

    const float d = gfxm::dot(N, Nc);
    const float absd = fabsf(d);
    const bool is_parallel = absd <= FLT_EPSILON;

    // Check triangle face first
    gfxm::vec3 line_plane_intersection;
    if (is_parallel) {
        line_plane_intersection = p0;
    } else {
        float t = gfxm::dot(N, (p0 - B) / d);
        line_plane_intersection = B + Nc * t;
    }

    gfxm::vec3 pt_sphere = closestPointOnSegment(line_plane_intersection, A, B);
    float dist = gfxm::dot(pt_sphere - p0, N);
    if (dist >= -capsule_radius && dist <= capsule_radius) {
        const gfxm::vec3 point0 = pt_sphere - N * dist;
        const gfxm::vec3 c0 = gfxm::cross(point0 - p0, p1 - p0);
        const gfxm::vec3 c1 = gfxm::cross(point0 - p1, p2 - p1);
        const gfxm::vec3 c2 = gfxm::cross(point0 - p2, p0 - p2);
        const bool is_inside = gfxm::dot(c0, N) <= 0 && dot(c1, N) <= 0 && dot(c2, N) <= 0;
        if (is_inside) {
            const gfxm::vec3 intersection_vec = pt_sphere - point0;
            const float len = gfxm::length(intersection_vec);
            const gfxm::vec3 penetration_normal = intersection_vec / len;
            const float penetration_depth = capsule_radius - len;
        
            cp.point_a = gfxm::normalize(point0 - pt_sphere) * capsule_radius + pt_sphere;
            cp.point_b = point0;
            cp.normal_b = penetration_normal;
            cp.normal_a = -cp.normal_b;
            cp.depth = penetration_depth;
            cp.type = CONTACT_POINT_TYPE::TRIANGLE_FACE;
            return true;
        }
    }

    // Then edges
    const gfxm::vec3 points[3] = { p0, p1, p2 };
    float min_dist_sq = FLT_MAX;
    gfxm::vec3 best_on_edge;
    gfxm::vec3 best_on_capsule;
    int edge_idx = -1;
    for (int i = 0; i < 3; ++i) {
        gfxm::vec3 on_edge;
        gfxm::vec3 on_capsule;
        gfxm::vec3 v1;
        float dist_sq;

        closestPointSegmentSegment(points[i], points[(i + 1) % 3], A, B, on_edge, on_capsule);
        v1 = on_capsule - on_edge;
        dist_sq = gfxm::dot(v1, v1);
        if (dist_sq < radius_sq && dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            best_on_edge = on_edge;
            best_on_capsule = on_capsule;
            edge_idx = i;
        }
    }

    if (min_dist_sq < radius_sq) {
        const gfxm::vec3 pt = best_on_edge;
        float depth = capsule_radius - sqrt(min_dist_sq);
        cp.point_a = gfxm::normalize(pt - best_on_capsule) * capsule_radius + best_on_capsule;
        cp.point_b = pt;
        cp.normal_b = gfxm::normalize(best_on_capsule - pt);
        cp.normal_a = -cp.normal_b;
        cp.depth = depth;
        cp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
        cp.edge_idx = edge_idx;
        return true;
    }

    return false;
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
    gfxm::vec3 center_tmp;
    gfxm::vec3 point1;// = closestPointOnSegment(center, p0, p1);
    closestPointSegmentSegment(p0, p1, A, B, point1, center_tmp);
    gfxm::vec3 v1 = center_tmp - point1;
    float distsq1 = gfxm::dot(v1, v1);
    bool intersects = distsq1 < radiussq;
    if (intersects) {
        gfxm::vec3 pt = point1;
        float depth = capsule_radius - sqrt(distsq1);
        cp.point_a = gfxm::normalize(pt - center_tmp) * capsule_radius + center_tmp;
        cp.point_b = pt;
        cp.normal_b = gfxm::normalize(center_tmp - pt);
        cp.normal_a = -cp.normal_b;
        cp.depth = depth;
        cp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
        cp.edge_idx = 0;
        return true;
    }

    gfxm::vec3 point2;// = closestPointOnSegment(center, p1, p2);
    closestPointSegmentSegment(p1, p2, A, B, point2, center_tmp);
    gfxm::vec3 v2 = center_tmp - point2;
    float distsq2 = gfxm::dot(v2, v2);
    intersects |= distsq2 < radiussq;
    if (intersects) {
        gfxm::vec3 pt = point2;
        float depth = capsule_radius - sqrt(distsq2);
        cp.point_a = gfxm::normalize(pt - center_tmp) * capsule_radius + center_tmp;
        cp.point_b = pt;
        cp.normal_b = gfxm::normalize(center_tmp - pt);
        cp.normal_a = -cp.normal_b;
        cp.depth = depth;
        cp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
        cp.edge_idx = 1;
        return true;
    }

    gfxm::vec3 point3;// = closestPointOnSegment(center, p2, p0);
    closestPointSegmentSegment(p2, p0, A, B, point3, center_tmp);
    gfxm::vec3 v3 = center_tmp - point3;
    float distsq3 = gfxm::dot(v3, v3);
    intersects |= distsq3 < radiussq;
    if (intersects) {
        gfxm::vec3 pt = point3;
        float depth = capsule_radius - sqrt(distsq3);
        cp.point_a = gfxm::normalize(pt - center_tmp) * capsule_radius + center_tmp;
        cp.point_b = pt;
        cp.normal_b = gfxm::normalize(center_tmp - pt);
        cp.normal_a = -cp.normal_b;
        cp.depth = depth;
        
        cp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
        cp.edge_idx = 2;
        
        return true;
    }

    int edge_idx = 0;
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
            edge_idx = 0;

            d = center - point2;
            float distsq = gfxm::dot(d, d);
            if (distsq < best_distsq) {
                best_distsq = distsq;
                best_point = point2;
                intersection_vec = d;
                edge_idx = 1;
            }

            d = center - point3;
            distsq = gfxm::dot(d, d);
            if (distsq < best_distsq) {
                best_distsq = distsq;
                best_point = point3;
                intersection_vec = d;
                edge_idx = 2;
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
            cp.edge_idx = edge_idx;
        }

        return true;
    }

    return false;
}