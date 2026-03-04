#pragma once

#include "math/gfxm.hpp"
#include "../types.hpp"


class phyShape {
    PHY_SHAPE_TYPE type = PHY_SHAPE_TYPE::EMPTY;
public:
    phyShape(PHY_SHAPE_TYPE type)
        : type(type) {}
    virtual ~phyShape() {}

    PHY_SHAPE_TYPE getShapeType() const {
        return type;
    }
    virtual gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const = 0;
    virtual gfxm::mat3 calcInertiaTensor(float mass) const {
        if (mass == .0f) {
            return gfxm::mat3(.0f);
        }
        return gfxm::mat3(1.f);
    }

    virtual gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir, const gfxm::mat4& transform) const {
        assert(false);
        return transform[3];
    }
};

