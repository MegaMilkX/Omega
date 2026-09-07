#pragma once

#include "convex_mesh_collider.auto.hpp"
#include <vector>
#include <memory>
#include "collider.hpp"
#include "collision/shape/convex_mesh.hpp"


[[cppi_class]];
class ConvexMeshCollider : public Collider {
    std::unique_ptr<phyConvexMeshShape> shape;
    std::unique_ptr<phyConvexMesh> mesh;
public:
    TYPE_ENABLE();

    ConvexMeshCollider()
        : shape(new phyConvexMeshShape)
        , mesh(new phyConvexMesh)
    {
        // idk if there is a point in doing this,
        // the mesh is empty at this point anyway
        shape->setMesh(mesh.get());
    }

    phyShape* getShape() const override { return shape.get(); }

    void setData(const gfxm::vec3* vertices, int vcount, int* indices, int icount) {
        mesh->setData(vertices, vcount, indices, icount);
        shape->setMesh(mesh.get()); // need to do this so the local aabb gets recalculated
    }
    // TODO: Should be an option to automatically calculate it on setData,
    // currently always have to set inertia matrix explicitly
    void setInertia(const gfxm::mat3& inertia) {
        shape->setInertiaTensor(inertia);
    }
};

