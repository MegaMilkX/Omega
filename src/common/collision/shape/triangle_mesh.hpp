#pragma once

#include "shape.hpp"
#include "collision/collision_triangle_mesh.hpp"


class phyTriangleMeshShape : public phyShape {
    CollisionTriangleMesh* mesh = 0;
    const gfxm::vec3*   vertices = 0;
    const uint32_t*     indices = 0;
    int                 index_count = 0;
    gfxm::aabb local_aabb;
public:
    phyTriangleMeshShape()
    : phyShape(PHY_SHAPE_TYPE::TRIANGLE_MESH) {}
    void setMesh(CollisionTriangleMesh* mesh) {
        this->mesh = mesh;
        vertices = mesh->getVertexData();
        indices = mesh->getIndexData();
        index_count = mesh->indexCount();
        
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
    const CollisionTriangleMesh* getMesh() const {
        return mesh;
    }
    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        return gfxm::aabb_transform(local_aabb, transform);
    }

    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir, const gfxm::mat4& transform, int tri) const {
        const gfxm::vec3* vertices = getMesh()->getVertexData();
        const uint32_t* indices = getMesh()->getIndexData();
        gfxm::vec3 A, B, C;
        A = transform * gfxm::vec4(vertices[indices[tri * 3]], 1.0f);
        B = transform * gfxm::vec4(vertices[indices[tri * 3 + 1]], 1.0f);
        C = transform * gfxm::vec4(vertices[indices[tri * 3 + 2]], 1.0f);

        float da = gfxm::dot(A, dir);
        float db = gfxm::dot(B, dir);
        float dc = gfxm::dot(C, dir);
        if (da > db && da > dc) {
            return A;
        } else if (db > dc) {
            return B;
        } else {
            return C;
        }
    }

    void debugDraw(const gfxm::mat4& transform, uint32_t color) const {
        if (!mesh) {
            return;
        }
        mesh->debugDraw(transform, color);
    }
};

