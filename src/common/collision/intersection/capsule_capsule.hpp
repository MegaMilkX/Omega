#pragma once

#include <array>

#include "math/gfxm.hpp"

#include "util/util.hpp"

#include "debug_draw/debug_draw.hpp"

inline float closestPointSegmentSegment(
    const gfxm::vec3& A0, const gfxm::vec3& B0,
    const gfxm::vec3& A1, const gfxm::vec3& B1,
    gfxm::vec3& C0, gfxm::vec3& C1,
    float* out_s = 0, float* out_t = 0
) {
    float s = .0f, t = .0f;

    gfxm::vec3 d1 = B0 - A0;
    gfxm::vec3 d2 = B1 - A1;
    gfxm::vec3 r = A0 - A1;
    float a = gfxm::dot(d1, d1);
    float e = gfxm::dot(d2, d2);
    float f = gfxm::dot(d2, r);

    if (a <= FLT_EPSILON && e <= FLT_EPSILON) {
        s = t = .0f;
        C0 = A0;
        C1 = A1;
        return gfxm::dot(C0 - C1, C0 - C1);
    }
    if (a <= FLT_EPSILON) {
        s = .0f;
        t = f / e;
        t = gfxm::clamp(t, .0f, 1.f);
    } else {
        float c = gfxm::dot(d1, r);
        if (e <= FLT_EPSILON) {
            t = .0f;
            s = gfxm::clamp(-c / a, .0f, 1.f);
        } else {
            float b = gfxm::dot(d1, d2);
            float denom = a * e - b * b;
            if (denom != .0f) {
                s = gfxm::clamp((b * f - c * e) / denom, .0f, 1.f);
            } else {
                s = .0f;
            }
            t = (b * s + f) / e;
            if (t < .0f) {
                t = .0f;
                s = gfxm::clamp(-c / a, .0f, 1.f);
            } else if(t > 1.f) {
                t = 1.f;
                s = gfxm::clamp((b - c) / a, .0f, 1.f);
            }
        }
    }
    C0 = A0 + d1 * s;
    C1 = A1 + d2 * t;
    if (out_s) *out_s = s;
    if (out_t) *out_t = t;
    return gfxm::dot(C0 - C1, C0 - C1);
}

inline bool intersectCapsuleCapsule(
    float radius_a, float height_a, const gfxm::mat4& transform_a,
    float radius_b, float height_b, const gfxm::mat4& transform_b,
    ContactPoint& cp
) {
    gfxm::vec3 A = transform_a * gfxm::vec4(.0f, height_a * .5f, .0f, 1.f);
    gfxm::vec3 B = transform_a * gfxm::vec4(.0f, -height_a * .5f, .0f, 1.f);
    gfxm::vec3 C = transform_b * gfxm::vec4(.0f, height_b * .5f, .0f, 1.f);
    gfxm::vec3 D = transform_b * gfxm::vec4(.0f, -height_b * .5f, .0f, 1.f);
    gfxm::vec3 Ca;
    gfxm::vec3 Cb;
    float dist2 = closestPointSegmentSegment(
        A, B, C, D, Ca, Cb
    );
    //float r = radius_a + radius_b;
    //float r_dist2 = r * r;

    //dbgDrawSphere(Ca, .2f, DBG_COLOR_BLUE);
    //dbgDrawSphere(Cb, .2f, DBG_COLOR_GREEN);
    //dbgDrawLine(Ca, Cb, DBG_COLOR_RED);

    gfxm::vec3 norm = Cb - Ca;
    if (norm.x == .0f && norm.y == .0f && norm.z == .0f) {
        norm = gfxm::vec3(FLT_EPSILON, 0, 0);
    }
    float center_distance = gfxm::length(norm);
    float radius_distance = radius_a + radius_b;
    float distance = center_distance - radius_distance;
    if (distance <= FLT_EPSILON && center_distance >= .0f) {
        gfxm::vec3 normal_a;
        gfxm::vec3 normal_b;
        if(center_distance > FLT_EPSILON) {
            normal_a = norm / center_distance;
            normal_b = -normal_a;
        } else {
            normal_a = gfxm::vec3(1.f, .0f, .0f);
            normal_b = -gfxm::vec3(-1.f, .0f, .0f);
        }
        gfxm::vec3 pt_a = normal_a * radius_a + Ca;
        gfxm::vec3 pt_b = normal_b * radius_b + Cb;

        cp.point_a = pt_a;
        cp.point_b = pt_b;
        cp.normal_a = normal_a;
        cp.normal_b = normal_b;
        cp.depth = -distance;
        return true;
    }
    return false;
}

inline bool intersectCapsuleCapsule_old(
    float radius_a, float height_a, const gfxm::mat4& transform_a,
    float radius_b, float height_b, const gfxm::mat4& transform_b,
    ContactPoint& cp
) {
    gfxm::vec3 A = transform_a * gfxm::vec4(.0f, height_a * .5f, .0f, 1.f);
    gfxm::vec3 B = transform_a * gfxm::vec4(.0f, -height_a * .5f, .0f, 1.f);
    gfxm::vec3 C = transform_b * gfxm::vec4(.0f, height_b * .5f, .0f, 1.f);
    gfxm::vec3 D = transform_b * gfxm::vec4(.0f, -height_b * .5f, .0f, 1.f);

    gfxm::vec3 DC = D - C;
    float lineDirSqMag = gfxm::dot(DC, DC);
    gfxm::vec3 planeA = A - ((gfxm::dot(A - C, DC) / lineDirSqMag) * DC);
    gfxm::vec3 planeB = B - ((gfxm::dot(B - C, DC) / lineDirSqMag) * DC);
    gfxm::vec3 planeBA = planeB - planeA;

    float t = gfxm::dot(C - planeA, planeBA) / gfxm::dot(planeBA, planeBA);
    // TODO: Check if parallel

    gfxm::vec3 ABtoLineCD = gfxm::lerp(A, B, gfxm::clamp(t, .0f, 1.f));

    gfxm::vec3 closestA = closestPointOnSegment(ABtoLineCD, C, D);
    gfxm::vec3 closestB = closestPointOnSegment(closestA, A, B);

    //dbgDrawSphere(closestA, .2f, DBG_COLOR_BLUE);
    //dbgDrawSphere(closestB, .2f, DBG_COLOR_GREEN);

    gfxm::vec3 norm = closestB - closestA;
    float center_distance = gfxm::length(norm);
    float radius_distance = radius_a + radius_b;
    float distance = center_distance - radius_distance;
    if (distance <= FLT_EPSILON) {
        gfxm::vec3 normal_a = norm / center_distance;
        gfxm::vec3 normal_b = -normal_a;
        gfxm::vec3 pt_a = normal_a * radius_a + closestA;
        gfxm::vec3 pt_b = normal_b * radius_b + closestB;

        cp.point_a = pt_a;
        cp.point_b = pt_b;
        cp.normal_a = -normal_a;
        cp.normal_b = -normal_b;
        cp.depth = -distance;
        return true;
    }

    return false;
}

inline bool intersectCapsuleAabb(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const gfxm::aabb& aabb
) {
    bool intersects = false;

    //const gfxm::aabb aabb(gfxm::vec3(-10, 0, -10), gfxm::vec3(10, 3, 10));
    //dbgDrawAabb(aabb, 0xFFFFFFFF);

    gfxm::vec3 dir = (to - from);
    gfxm::vec3 inv_dir(1.f / dir.x, 1.f / dir.y, 1.f / dir.z);
    float tx1 = (aabb.from.x - from.x) * inv_dir.x;
    float tx2 = (aabb.to.x - from.x) * inv_dir.x;
    float tminX = gfxm::_min(tx1, tx2);
    float tmaxX = gfxm::_max(tx1, tx2);

    float ty1 = (aabb.from.y - from.y) * inv_dir.y;
    float ty2 = (aabb.to.y - from.y) * inv_dir.y;
    float tminY = gfxm::_min(ty1, ty2);
    float tmaxY = gfxm::_max(ty1, ty2);

    float tz1 = (aabb.from.z - from.z) * inv_dir.z;
    float tz2 = (aabb.to.z - from.z) * inv_dir.z;
    float tminZ = gfxm::_min(tz1, tz2);
    float tmaxZ = gfxm::_max(tz1, tz2);

    gfxm::vec3 pminX = from + dir * tminX;
    gfxm::vec3 pmaxX = from + dir * tmaxX;

    gfxm::vec3 pminY = from + dir * tminY;
    gfxm::vec3 pmaxY = from + dir * tmaxY;

    gfxm::vec3 pminZ = from + dir * tminZ;
    gfxm::vec3 pmaxZ = from + dir * tmaxZ;

    pminX.y = gfxm::clamp(pminX.y, aabb.from.y, aabb.to.y);
    pminX.z = gfxm::clamp(pminX.z, aabb.from.z, aabb.to.z);
    pmaxX.y = gfxm::clamp(pminX.y, aabb.from.y, aabb.to.y);
    pmaxX.z = gfxm::clamp(pminX.z, aabb.from.z, aabb.to.z);
    pminY.x = gfxm::clamp(pminY.x, aabb.from.x, aabb.to.x);
    pminY.z = gfxm::clamp(pminY.z, aabb.from.z, aabb.to.z);
    pmaxY.x = gfxm::clamp(pminY.x, aabb.from.x, aabb.to.x);
    pmaxY.z = gfxm::clamp(pminY.z, aabb.from.z, aabb.to.z);
    pminZ.x = gfxm::clamp(pminZ.x, aabb.from.x, aabb.to.x);
    pminZ.y = gfxm::clamp(pminZ.y, aabb.from.y, aabb.to.y);
    pmaxZ.x = gfxm::clamp(pminZ.x, aabb.from.x, aabb.to.x);
    pmaxZ.y = gfxm::clamp(pminZ.y, aabb.from.y, aabb.to.y);

    std::array<gfxm::vec3, 6> p = { pminX, pmaxX, pminY, pmaxY, pminZ, pmaxZ };
    static std::array<gfxm::vec3, 6> m = { 
        gfxm::vec3(1,0,0), gfxm::vec3(1,0,0),
        gfxm::vec3(0,1,0), gfxm::vec3(0,1,0),
        gfxm::vec3(0,0,1), gfxm::vec3(0,0,1)
    };
    float closestDist = INFINITY;
    gfxm::vec3 closest_on_aabb;
    for (int i = 0; i < p.size(); ++i) {
        auto& pt = p[i];
        auto& m_ = m[i];
        float t = gfxm::dot(pt - from, dir) / gfxm::dot(dir, dir);
        t = gfxm::clamp(t, .0f, 1.f);
        gfxm::vec3 closest_on_line = from + dir * t;
        pt.x = gfxm::clamp(
            pt.x * (m_.x) + closest_on_line.x * (1.f - m_.x),
            aabb.from.x, aabb.to.x
        );
        pt.y = gfxm::clamp(
            pt.y * (m_.y) + closest_on_line.y * (1.f - m_.y),
            aabb.from.y, aabb.to.y
        );
        pt.z = gfxm::clamp(
            pt.z * (m_.z) + closest_on_line.z * (1.f - m_.z),
            aabb.from.z, aabb.to.z
        );
        float dist = (pt - closest_on_line).length2();
        if (dist < closestDist) {
            closestDist = dist;
            closest_on_aabb = pt;
        }
    }

    const gfxm::vec3& A = from;
    const gfxm::vec3& B = to;
    const gfxm::vec3& P = closest_on_aabb;

    auto AB = B - A;
    auto AP = P - A;
    float lenSqrAB = AB.length2();
    float t = (AP.x * AB.x + AP.y * AB.y + AP.z * AB.z) / lenSqrAB;
    gfxm::vec3 closest_pt_on_line = gfxm::lerp(A, B, gfxm::clamp(t, .0f, 1.f));
    /*
    dbgDrawSphere(closest_on_aabb, .1f, 0xFF00FFFF);
    dbgDrawSphere(closest_pt_on_line, .1f, 0xFF00FFFF);*/
    float dist = (closest_on_aabb - closest_pt_on_line).length();
    if (gfxm::point_in_aabb(aabb, closest_pt_on_line) || dist <= sweep_radius) {
        intersects = true;
    }
    /*
    dbgDrawLine(from, to, 0xFF0000FF);
    dbgDrawSphere(closest_on_aabb, .1f, 0xFFFF0099);
    dbgDrawSphere(closest_pt_on_line, radius, 0xFF00FF99);
    dbgDrawAabb(aabb, 0xFFFFFFFF);*/

    return intersects;
}