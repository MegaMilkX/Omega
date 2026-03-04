#pragma once

#include "shape.hpp"


class phyBoxShape : public phyShape {
public:
    phyBoxShape()
        : phyShape(PHY_SHAPE_TYPE::BOX) {}
    gfxm::vec3 half_extents = gfxm::vec3(.5f, .5f, .5f);
    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        gfxm::aabb aabb;
        gfxm::vec3 vertices[8] = {
            { -half_extents.x, -half_extents.y, -half_extents.z },
            {  half_extents.x, -half_extents.y, -half_extents.z },
            {  half_extents.x, -half_extents.y,  half_extents.z },
            { -half_extents.x, -half_extents.y,  half_extents.z },
            { -half_extents.x,  half_extents.y, -half_extents.z },
            {  half_extents.x,  half_extents.y, -half_extents.z },
            {  half_extents.x,  half_extents.y,  half_extents.z },
            { -half_extents.x,  half_extents.y,  half_extents.z }
        };
        for (int i = 0; i < 8; ++i) {
            vertices[i] = transform * gfxm::vec4(vertices[i], 1.0f);
        }
        aabb.from = vertices[0];
        aabb.to = vertices[0];
        for (int i = 1; i < 8; ++i) {
            gfxm::expand_aabb(aabb, vertices[i]);
        }
        return aabb;
    }

    gfxm::mat3 calcInertiaTensor(float mass) const override {
        float w2 = half_extents.x * half_extents.x;
        float h2 = half_extents.y * half_extents.y;
        float d2 = half_extents.z * half_extents.z;
        float Iw = (1.f / 12.f) * mass * (d2 + h2);
        float Ih = (1.f / 12.f) * mass * (w2 + d2);
        float Id = (1.f / 12.f) * mass * (w2 + h2);
        gfxm::mat3 inertia;
        inertia[0] = gfxm::vec3(Iw, .0f, .0f);
        inertia[1] = gfxm::vec3(.0f, Ih, .0f);
        inertia[2] = gfxm::vec3(.0f, .0f, Id);
        return inertia;
    }

    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir_, const gfxm::mat4& transform) const override {
        const gfxm::vec3 dir = gfxm::inverse(transform) * gfxm::vec4(dir_, .0f);
        gfxm::vec3 result = gfxm::vec3(
            (dir.x >= 0.0f ? half_extents.x : -half_extents.x),
            (dir.y >= 0.0f ? half_extents.y : -half_extents.y),
            (dir.z >= 0.0f ? half_extents.z : -half_extents.z)
        );
        result = transform * gfxm::vec4(result, 1.f);
        return result;
    }
};

