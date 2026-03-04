#pragma once

#include "shape.hpp"
#include "../convex_mesh.hpp"


class phyConvexMeshShape : public phyShape {
    const phyConvexMesh*  mesh = 0;
    const gfxm::vec3*           vertices = 0;
    gfxm::aabb                  local_aabb;
    gfxm::mat3                  inertia_tensor;
public:
    phyConvexMeshShape()
        : phyShape(PHY_SHAPE_TYPE::CONVEX_MESH) {
        inertia_tensor = gfxm::mat3(1.f);
    }
    
    void setMesh(const phyConvexMesh* mesh) {
        this->mesh = mesh;
        vertices = mesh->getVertexData();

        if (!vertices) {
            local_aabb.from = gfxm::vec3(0, 0, 0);
            local_aabb.to = gfxm::vec3(0, 0, 0);
            return;
        }
        local_aabb.from = vertices[0];
        local_aabb.to = vertices[0];
        for (int i = 1; i < mesh->vertexCount(); ++i) {
            gfxm::expand_aabb(local_aabb, vertices[i]);
        }
    }
    const phyConvexMesh* getMesh() const {
        return mesh;
    }

    void setInertiaTensor(const gfxm::mat3& tensor) {
        inertia_tensor = tensor;
    }

    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        return gfxm::aabb_transform(local_aabb, transform);
    }
    
    gfxm::mat3 calcInertiaTensor(float mass) const {
        return inertia_tensor * mass;
    }

    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir, const gfxm::mat4& transform) const override {
        if (mesh == nullptr) {
            assert(false);
            return gfxm::vec3(0,0,0);
        }

        gfxm::vec3 lcl_dir = gfxm::inverse(transform) * gfxm::vec4(dir, .0f);

        float max_d = -FLT_MAX;
        int vid = 0;
        for (int i = 0; i < mesh->vertexCount(); ++i) {
            const auto& v = mesh->getVertexData()[i];
            float d = gfxm::dot(lcl_dir, v);
            if (d > max_d) {
                vid = i;
                max_d = d;
            }
        }
        return transform * gfxm::vec4(mesh->getVertexData()[vid], 1.f);
    }

    void debugDraw(const gfxm::mat4& transform, uint32_t color) const {
        if (!mesh) {
            return;
        }
        mesh->debugDraw(transform, color);
    }
};

