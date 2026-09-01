#pragma once

#include <memory>
#include "collider.hpp"
#include "collision/shape/sphere.hpp"


class SphereCollider : public Collider {
    std::unique_ptr<phySphereShape> shape;
public:
    SphereCollider()
    : shape(new phySphereShape) {}

    phyShape* getShape() const override { return shape.get(); }

    void setRadius(float r) {
        shape->radius = r;
    }
};

