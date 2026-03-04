#pragma once

#include "shape.hpp"


class phySphereShape : public phyShape {
public:
    phySphereShape()
        : phyShape(PHY_SHAPE_TYPE::SPHERE) {}
    float radius = .25f;
    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        gfxm::aabb aabb;
        aabb.from = gfxm::vec3(-radius, -radius, -radius) + gfxm::vec3(transform[3]);
        aabb.to = gfxm::vec3(radius, radius, radius) + gfxm::vec3(transform[3]);
        return aabb;
    }
    gfxm::mat3 calcInertiaTensor(float mass) const override {
        float I = .4f * mass * powf(radius, 2.f);
        gfxm::mat3 inertia;
        inertia[0] = gfxm::vec3(I, .0f, .0f);
        inertia[1] = gfxm::vec3(.0f, I, .0f);
        inertia[2] = gfxm::vec3(.0f, .0f, I);
        return inertia;
    }
    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir, const gfxm::mat4& transform) const override {
        gfxm::vec3 lcl_dir = transform * gfxm::vec4(dir, .0f);
        gfxm::vec3 P = gfxm::normalize(lcl_dir) * radius;
        return transform * gfxm::vec4(P, 1.f);
    }
};

