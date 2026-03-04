#pragma once

#include "shape.hpp"


class phyEmptyShape : public phyShape {
public:
    phyEmptyShape()
        : phyShape(PHY_SHAPE_TYPE::EMPTY) {}

    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        return gfxm::aabb();
    }

    gfxm::mat3 calcInertiaTensor(float mass) const override {
        return gfxm::mat3(.0f);
    }

    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir, const gfxm::mat4& transform) const override {
        return gfxm::vec3(.0f, .0f, .0f);
    }
};