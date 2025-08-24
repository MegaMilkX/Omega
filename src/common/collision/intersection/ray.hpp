#pragma once

#include "math/gfxm.hpp"
#include "collision/collision_contact_point.hpp"
#include "util/util.hpp"


inline bool intersectRayAabb(const gfxm::ray& ray, const gfxm::aabb& aabb) {
    float tx1 = (aabb.from.x - ray.origin.x) * ray.direction_inverse.x;
    float tx2 = (aabb.to.x - ray.origin.x) * ray.direction_inverse.x;
    float tmin = gfxm::_min(tx1, tx2);
    float tmax = gfxm::_max(tx1, tx2);

    float ty1 = (aabb.from.y - ray.origin.y) * ray.direction_inverse.y;
    float ty2 = (aabb.to.y - ray.origin.y) * ray.direction_inverse.y;
    tmin = gfxm::_max(tmin, gfxm::_min(ty1, ty2));
    tmax = gfxm::_min(tmax, gfxm::_max(ty1, ty2));

    float tz1 = (aabb.from.z - ray.origin.z) * ray.direction_inverse.z;
    float tz2 = (aabb.to.z - ray.origin.z) * ray.direction_inverse.z;
    tmin = gfxm::_max(tmin, gfxm::_min(tz1, tz2));
    tmax = gfxm::_min(tmax, gfxm::_max(tz1, tz2));

    if (tmax < .0f) {
        return false;
    }
    if (tmin <= 1.0f && tmax >= tmin) {
        //dbgDrawSphere(ray.direction * tmin + ray.origin, 0.05f, DBG_COLOR_RED);
        return true;
    }

    return false;
    // infinite hit
    //return tmax >= tmin;
}
inline bool intersectRayAabb(const gfxm::ray& ray, const gfxm::aabb& aabb, gfxm::vec3& hit, gfxm::vec3& normal, float& distance) {
    float tx1 = (aabb.from.x - ray.origin.x) * ray.direction_inverse.x;
    float tx2 = (aabb.to.x - ray.origin.x) * ray.direction_inverse.x;
    float tmin = gfxm::_min(tx1, tx2);
    float tmax = gfxm::_max(tx1, tx2);

    float ty1 = (aabb.from.y - ray.origin.y) * ray.direction_inverse.y;
    float ty2 = (aabb.to.y - ray.origin.y) * ray.direction_inverse.y;
    tmin = gfxm::_max(tmin, gfxm::_min(ty1, ty2));
    tmax = gfxm::_min(tmax, gfxm::_max(ty1, ty2));

    float tz1 = (aabb.from.z - ray.origin.z) * ray.direction_inverse.z;
    float tz2 = (aabb.to.z - ray.origin.z) * ray.direction_inverse.z;
    tmin = gfxm::_max(tmin, gfxm::_min(tz1, tz2));
    tmax = gfxm::_min(tmax, gfxm::_max(tz1, tz2));

    if (tmax < .0f) {
        return false;
    }
    if (tmin <= 1.0f && tmax >= tmin) {
        float normal_mul = 1.0f;
        if (tmin < .0f) {
            normal_mul = -1.0f;
            hit = ray.direction * tmax + ray.origin;
            distance = gfxm::length(ray.direction) * tmax;
        } else {
            hit = ray.direction * tmin + ray.origin;
            distance = gfxm::length(ray.direction) * tmin;
        }
        gfxm::vec3 c = (aabb.from + aabb.to) * .5f;
        gfxm::vec3 p = hit - c;
        gfxm::vec3 d = (aabb.from - aabb.to) * .5f;
        float bias = 1.000001;
        normal = gfxm::normalize(gfxm::vec3(
            (int)(p.x / fabsf(d.x) * bias),
            (int)(p.y / fabsf(d.y) * bias),
            (int)(p.z / fabsf(d.z) * bias)
        )) * normal_mul;

        return true;
    }

    return false;
}

inline bool intersectLineSphereFirstHitInfinite(
    const gfxm::vec3& LA,
    const gfxm::vec3& LB,
    const gfxm::vec3& SC, // sphere pos
    float SR, // sphere radius
    gfxm::vec3& hit
) {
    gfxm::vec3 A = LA;
    gfxm::vec3 B = LB;
    gfxm::vec3 C = SC;
    float R = SR;
    float a = gfxm::pow2(B.x - A.x) + gfxm::pow2(B.y - A.y) + gfxm::pow2(B.z - A.z);
    float b = 2.0f * ((B.x - A.x) * (A.x - C.x) + (B.y - A.y) * (A.y - C.y) + (B.z - A.z) * (A.z - C.z));
    float c = gfxm::pow2(A.x - C.x) + gfxm::pow2(A.y - C.y) + gfxm::pow2(A.z - C.z) - gfxm::pow2(R);
    float delta = gfxm::pow2(b) - 4.0f * a * c;

    if (delta < .0f) {
        return false;
    }
    if (delta == .0f) {
        float t = -b / (2.0f * a);
        gfxm::vec3 hitA = LA + (LB - LA) * t;
        hit = hitA;
        return true;
    }
    if (delta > .0f) {
        float tmin = (-b - gfxm::sqrt(delta)) / (2.0f * a);
        float tmax = (-b + gfxm::sqrt(delta)) / (2.0f * a);
        hit = LA + (LB - LA) * tmin;
        return true;
    }
    return false;
}

inline bool intersectRaySphere(
    const gfxm::ray& ray,
    const gfxm::vec3& sphere_pos,
    float sphere_radius,
    RayHitPoint& rhp
) {
    gfxm::vec3 A = ray.origin;
    gfxm::vec3 B = ray.origin + ray.direction;
    gfxm::vec3 C = sphere_pos;
    float R = sphere_radius;
    float a = gfxm::pow2(B.x - A.x) + gfxm::pow2(B.y - A.y) + gfxm::pow2(B.z - A.z);
    float b = 2.0f * ((B.x - A.x) * (A.x - C.x) + (B.y - A.y) * (A.y - C.y) + (B.z - A.z) * (A.z - C.z));
    float c = gfxm::pow2(A.x - C.x) + gfxm::pow2(A.y - C.y) + gfxm::pow2(A.z - C.z) - gfxm::pow2(R);
    float delta = gfxm::pow2(b) - 4.0f * a * c;
    if (delta < .0f) {
        return false;
    }
    if (delta == .0f) {
        float t = -b / (2.0f * a);
        gfxm::vec3 hit = ray.origin + ray.direction * t;
        rhp.normal = gfxm::normalize(hit - sphere_pos);
        rhp.point = hit;
        return true;
    }
    if (delta > .0f) {
        float tmin = (-b - gfxm::sqrt(delta)) / (2.0f * a);
        float tmax = (-b + gfxm::sqrt(delta)) / (2.0f * a);
        gfxm::vec3 hit;
        gfxm::vec3 hit_normal;
        if (tmax < .0f || tmin > 1.0f) {
            return false;
        }
        if (tmin < .0f) {
            // Ray originates inside a sphere
            hit = ray.origin + ray.direction * tmax;
            // Treating hit normal as a normal of an inside wall of a sphere
            hit_normal = gfxm::normalize(sphere_pos - hit);
            rhp.distance = gfxm::length(ray.direction) * tmax;
        } else {
            hit = ray.origin + ray.direction * tmin;
            hit_normal = gfxm::normalize(hit - sphere_pos);
            rhp.distance = gfxm::length(ray.direction) * tmin;
        }
        rhp.normal = hit_normal;
        rhp.point = hit;
        return true;
    }
    return false;
}

inline bool intersectRayBox(
    const gfxm::ray& ray,
    const gfxm::mat4& box_transform,
    const gfxm::vec3& half_extents,
    RayHitPoint& rhp
) {
    gfxm::mat4 Tinverse = gfxm::inverse(box_transform);
    gfxm::vec3 lcl_origin = Tinverse * gfxm::vec4(ray.origin, 1.f);
    gfxm::vec3 lcl_direction = Tinverse * gfxm::vec4(ray.direction, .0f);
    gfxm::aabb aabb;
    aabb.from = gfxm::vec3(-half_extents.x, -half_extents.y, -half_extents.z);
    aabb.to = gfxm::vec3(half_extents.x, half_extents.y, half_extents.z);
    gfxm::vec3 lcl_hit;
    gfxm::vec3 lcl_normal;
    float distance;
    if (intersectRayAabb(gfxm::ray(lcl_origin, lcl_direction), aabb, lcl_hit, lcl_normal, distance)) {
        rhp.point = box_transform * gfxm::vec4(lcl_hit, 1.f);
        rhp.normal = box_transform * gfxm::vec4(lcl_normal, .0f);
        rhp.distance = distance;
        return true;
    }
    return false;
}

inline bool intersectRayCapsule(
    const gfxm::ray& ray,
    const gfxm::mat4& capsule_transform,
    float capsule_height, float capsule_radius,
    RayHitPoint& rhp
) {
    gfxm::mat4 Tinverse = gfxm::inverse(capsule_transform);
    gfxm::vec3 lcl_origin = Tinverse * gfxm::vec4(ray.origin, 1.0f);
    gfxm::vec3 lcl_direction = Tinverse * gfxm::vec4(ray.direction, .0f);
    gfxm::vec2 lcl_origin2d(lcl_origin.x, lcl_origin.z);
    gfxm::vec2 lcl_direction2d(lcl_direction.x, lcl_direction.z);
    gfxm::vec3 Ctip = gfxm::vec3(.0f, capsule_height * .5f, .0f);
    gfxm::vec3 Cbase = gfxm::vec3(.0f, -capsule_height * .5f, .0f);

    // Check circle vs line segment
    float a = gfxm::dot(lcl_direction2d, lcl_direction2d);
    float b = 2.0f * gfxm::dot(lcl_origin2d, lcl_direction2d);
    float c = gfxm::dot(lcl_origin2d, lcl_origin2d) - gfxm::pow2(capsule_radius);

    float discr = gfxm::pow2(b) - 4.0f * a * c;
    if (discr < .0f) {
        return false;
    } else {
        discr = gfxm::sqrt(discr);
        float tmin = (-b - discr) / (2.0f * a);
        float tmax = (-b + discr) / (2.0f * a);
        gfxm::vec3 hit0 = ray.origin + ray.direction * tmin;
        gfxm::vec3 hit1 = ray.origin + ray.direction * tmax;
        gfxm::vec3 lclhit0 = lcl_origin + lcl_direction * tmin;
        gfxm::vec3 lclhit1 = lcl_origin + lcl_direction * tmax;
        
        gfxm::vec3 C0 = closestPointOnSegment(lclhit0, Cbase, Ctip);
        gfxm::vec3 C1 = closestPointOnSegment(lclhit1, Cbase, Ctip);
        
        gfxm::vec3 C = C0;
        float normal_mul = 1.0f;
        if (tmin < .0f) {
            C = C1;
            normal_mul = -1.0f;
            lcl_origin = lcl_origin + lcl_direction;
            lcl_direction = -lcl_direction;
        }
        gfxm::vec3 hit;
        RayHitPoint rhp_;
        if (!intersectRaySphere(gfxm::ray(lcl_origin, lcl_direction), C, capsule_radius, rhp_)) {
            return false;
        }
        rhp.point = capsule_transform * gfxm::vec4(rhp_.point, 1.0f);
        rhp.normal = capsule_transform * gfxm::vec4(rhp_.normal * normal_mul, .0f);
        rhp.distance = rhp_.distance;
        return true;
    }
    
    return false;
}

bool intersectRayTriangle(
    const gfxm::ray& ray,
    const gfxm::vec3& A,
    const gfxm::vec3& B,
    const gfxm::vec3& C,
    RayHitPoint& rhp
);

class CollisionTriangleMesh;
bool intersectRayTriangleMesh(const gfxm::ray& ray, const CollisionTriangleMesh* mesh, RayHitPoint& rhp);

class CollisionConvexMesh;
bool intersectRayConvexMesh(const gfxm::ray& ray, const CollisionConvexMesh* mesh, RayHitPoint& rhp);
