#pragma once

#include <memory>
#include "collider.hpp"
#include "collision/shape/triangle_mesh.hpp"


class TriangleMeshCollider : public Collider {
    std::unique_ptr<phyTriangleMeshShape> shape;
    std::unique_ptr<CollisionTriangleMesh> tri_mesh;
public:
    TriangleMeshCollider()
        : shape(new phyTriangleMeshShape)
        , tri_mesh(new CollisionTriangleMesh)
    {}

    phyShape* getShape() const override { return shape.get(); }
};

