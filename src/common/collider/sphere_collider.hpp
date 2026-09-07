#pragma once

#include "sphere_collider.auto.hpp"
#include <memory>
#include "collider.hpp"
#include "collision/shape/sphere.hpp"


[[cppi_class]];
class SphereCollider : public Collider {
    std::unique_ptr<phySphereShape> shape;
public:
    TYPE_ENABLE();

    SphereCollider()
    : shape(new phySphereShape) {}

    phyShape* getShape() const override { return shape.get(); }

    void setRadius(float r) {
        shape->radius = r;
    }
};

