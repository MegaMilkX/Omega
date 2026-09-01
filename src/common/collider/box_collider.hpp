#pragma once

#include <memory>
#include "collider.hpp"
#include "collision/shape/box.hpp"


class BoxCollider : public Collider {
    std::unique_ptr<phyBoxShape> shape;
public:
    BoxCollider()
        : shape(new phyBoxShape) {}

    phyShape* getShape() const override { return shape.get(); }

    void setHalfExtents(const gfxm::vec3& extents) {
        shape->half_extents = extents;
    }
    void setHalfExtents(float x, float y, float z) {
        setHalfExtents(gfxm::vec3(x, y, z));
    }
};

