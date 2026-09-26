#pragma once

#include "box_collider.auto.hpp"
#include <memory>
#include "collider.hpp"
#include "collision/shape/box.hpp"


[[cppi_class]];
class BoxCollider : public Collider {
    std::unique_ptr<phyBoxShape> shape;
public:
    TYPE_ENABLE();

    BoxCollider()
        : shape(new phyBoxShape) {}

    phyShape* getShape() const override { return shape.get(); }

    [[cppi_decl, set("half_extents")]]
    void setHalfExtents(const gfxm::vec3& extents) {
        shape->half_extents = extents;
    }
    void setHalfExtents(float x, float y, float z) {
        setHalfExtents(gfxm::vec3(x, y, z));
    }
    [[cppi_decl, get("half_extents")]]
    const gfxm::vec3& getHalfExtents() const { return shape->half_extents; }
};

