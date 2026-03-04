#pragma once

#include "shape.hpp"


class phyCapsuleShape : public phyShape {
public:
    phyCapsuleShape()
        : phyShape(PHY_SHAPE_TYPE::CAPSULE) {}
    float height = 1.0f;
    float radius = 0.5f;
    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        gfxm::aabb aabb0, aabb1;
        gfxm::vec3 A = transform * gfxm::vec4(.0f,  height * .5f, .0f, 1.f);
        gfxm::vec3 B = transform * gfxm::vec4(.0f, -height * .5f, .0f, 1.f);
        aabb0.from = A - gfxm::vec3(radius, radius, radius);
        aabb0.to = A + gfxm::vec3(radius, radius, radius);
        aabb1.from = B - gfxm::vec3(radius, radius, radius);
        aabb1.to = B + gfxm::vec3(radius, radius, radius);
        gfxm::aabb aabb;
        aabb.from.x = gfxm::_min(aabb0.from.x, aabb1.from.x);
        aabb.from.y = gfxm::_min(aabb0.from.y, aabb1.from.y);
        aabb.from.z = gfxm::_min(aabb0.from.z, aabb1.from.z);
        aabb.to.x = gfxm::_max(aabb0.to.x, aabb1.to.x);
        aabb.to.y = gfxm::_max(aabb0.to.y, aabb1.to.y);
        aabb.to.z = gfxm::_max(aabb0.to.z, aabb1.to.z);
        return aabb;
    }

    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir_, const gfxm::mat4& transform) const override {
        const gfxm::vec3 dir = gfxm::inverse(transform) * gfxm::vec4(dir_, .0f);

        const float half_height = height * .5f;
        gfxm::vec3 res(0.0f, 0.0f, 0.0f);

        float len = gfxm::length(dir);
        gfxm::vec3 ndir = dir / len;

        // Pick top or bottom sphere center based on direction.y
        gfxm::vec3 capCenter = (dir.y >= 0.0f)
            ? gfxm::vec3(0.0f,  half_height, 0.0f)
            : gfxm::vec3(0.0f, -half_height, 0.0f);

        // Furthest point is sphere center + radius in direction
        res = capCenter + ndir * radius;

        res = transform * gfxm::vec4(res, 1.f);
        return res;
    }
};

