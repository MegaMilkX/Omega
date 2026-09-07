#pragma once

#include "triangle_mesh_collider.auto.hpp"
#include <memory>
#include "collider.hpp"
#include "collision/shape/triangle_mesh.hpp"


[[cppi_class]];
class TriangleMeshCollider : public Collider {
    std::unique_ptr<phyTriangleMeshShape> shape;
    std::unique_ptr<CollisionTriangleMesh> tri_mesh;
public:
    TYPE_ENABLE();

    TriangleMeshCollider()
        : shape(new phyTriangleMeshShape)
        , tri_mesh(new CollisionTriangleMesh)
    {}

    phyShape* getShape() const override { return shape.get(); }
};

