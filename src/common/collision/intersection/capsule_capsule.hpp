#pragma once

#include "math/gfxm.hpp"

#include "util/util.hpp"

#include "debug_draw/debug_draw.hpp"

inline float closestPointSegmentSegment(
    const gfxm::vec3& A0, const gfxm::vec3& B0,
    const gfxm::vec3& A1, const gfxm::vec3& B1,
    gfxm::vec3& C0, gfxm::vec3& C1
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

    gfxm::vec3 norm = Ca - Cb;
    float center_distance = gfxm::length(norm);
    float radius_distance = radius_a + radius_b;
    float distance = center_distance - radius_distance;
    if (distance <= FLT_EPSILON && center_distance > FLT_EPSILON) {
        gfxm::vec3 normal_a = norm / center_distance;
        gfxm::vec3 normal_b = -normal_a;
        gfxm::vec3 pt_a = normal_a * radius_a + Ca;
        gfxm::vec3 pt_b = normal_b * radius_b + Cb;

        cp.point_a = pt_a;
        cp.point_b = pt_b;
        cp.normal_a = -normal_a;
        cp.normal_b = -normal_b;
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