#pragma once

#include "gfxm.hpp"


namespace gfxm {


    inline bool intersect_line_plane_t(
        const gfxm::vec3& line_origin,
        const gfxm::vec3& line_normal,
        const gfxm::vec3& plane_normal,
        float plane_offset,
        float& out
    ) {
        float denom = gfxm::dot(plane_normal, line_normal);
        if (abs(denom) <= FLT_EPSILON) {
            return false;
        }
        gfxm::vec3 vec = plane_normal * plane_offset - line_origin;
        float t = gfxm::dot(vec, plane_normal) / denom;
        out = t;
        return true;
    }

    inline bool intersect_line_plane_point(
        const gfxm::vec3& line_origin,
        const gfxm::vec3& line_normal,
        const gfxm::vec3& plane_normal,
        float plane_offset,
        gfxm::vec3& out
    ) {
        float t = .0f;
        if (!intersect_line_plane_t(line_origin, line_normal, plane_normal, plane_offset, t)) {
            return false;
        }
        out = line_origin + line_normal * t;
        return true;
    }

    inline bool intersect_ray_plane_t(
        const gfxm::vec3& ray_from,
        const gfxm::vec3& ray_to,
        const gfxm::vec3& plane_normal,
        float plane_offset,
        float& t_out
    ) {
        float t = .0f;
        const gfxm::vec3 ray_dir = (ray_to - ray_from);
        if (!intersect_line_plane_t(ray_from, ray_dir, plane_normal, plane_offset, t)) {
            return false;
        }
        if (t < .0f || t > 1.f) {
            return false;
        }
        t_out = t;
        return true;
    }

    inline bool intersect_ray_plane_point(
        const gfxm::vec3& ray_from,
        const gfxm::vec3& ray_to,
        const gfxm::vec3& plane_normal,
        float plane_offset,
        gfxm::vec3& out
    ) {
        float t = .0f;
        const gfxm::vec3 ray_dir = (ray_to - ray_from);
        if (!intersect_line_plane_t(ray_from, ray_dir, plane_normal, plane_offset, t)) {
            return false;
        }
        if (t < .0f || t > 1.f) {
            return false;
        }
        out = ray_from + ray_dir * t;
        return true;
    }

    inline bool intersect_ray_aabb(const gfxm::ray& ray, const gfxm::aabb& aabb) {
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
            return true;
        }

        return false;
    }

    inline float closest_point_line_line(
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

        float c = gfxm::dot(d1, r);
        float b = gfxm::dot(d1, d2);
        float denom = a * e - b * b;
        if (denom != .0f) {
            s = (b * f - c * e) / denom;
        } else {
            s = .0f;
        }
        t = (b * s + f) / e;
        C0 = A0 + d1 * s;
        C1 = A1 + d2 * t;
        return gfxm::dot(C0 - C1, C0 - C1);
    }


}