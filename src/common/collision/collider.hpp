#pragma once

#include "math/gfxm.hpp"


enum class COLLISION_SHAPE_TYPE {
    UNKNOWN,
    SPHERE = 0b0001,
    BOX = 0b0010,
    CAPSULE = 0b0100,
    TRIANGLE_MESH = 0b1000
};
enum class COLLISION_PAIR_TYPE {
    UNKNOWN,
    SPHERE_SPHERE = (int)COLLISION_SHAPE_TYPE::SPHERE,                                          // 0b0001
    BOX_BOX = (int)COLLISION_SHAPE_TYPE::BOX,                                                   // 0b0010
    SPHERE_BOX = (int)COLLISION_SHAPE_TYPE::SPHERE | (int)COLLISION_SHAPE_TYPE::BOX,            // 0b0011
    CAPSULE_CAPSULE = (int)COLLISION_SHAPE_TYPE::CAPSULE,                                       // 0b0100
    CAPSULE_SPHERE = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::SPHERE,    // 0b0101
    CAPSULE_BOX = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::BOX,          // 0b0110
    CAPSULE_TRIANGLEMESH = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::TRIANGLE_MESH, // 0b1100
};

enum class COLLIDER_TYPE {
    COLLIDER,
    PROBE
};

enum COLLIDER_FLAGS {
    COLLIDER_STATIC = 1,
    COLLIDER_NO_RESPONSE = 2,
};

class CollisionShape {
    COLLISION_SHAPE_TYPE type = COLLISION_SHAPE_TYPE::UNKNOWN;
public:
    CollisionShape(COLLISION_SHAPE_TYPE type)
        : type(type) {}
    virtual ~CollisionShape() {}

    COLLISION_SHAPE_TYPE getShapeType() const {
        return type;
    }
    virtual gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const = 0;
};
class CollisionSphereShape : public CollisionShape {
public:
    CollisionSphereShape()
        : CollisionShape(COLLISION_SHAPE_TYPE::SPHERE) {}
    float radius = .5f;
    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        gfxm::aabb aabb;
        aabb.from = gfxm::vec3(-radius, -radius, -radius) + gfxm::vec3(transform[3]);
        aabb.to = gfxm::vec3(radius, radius, radius) + gfxm::vec3(transform[3]);
        return aabb;
    }
};
class CollisionBoxShape : public CollisionShape {
public:
    CollisionBoxShape()
        : CollisionShape(COLLISION_SHAPE_TYPE::BOX) {}
    gfxm::vec3 half_extents = gfxm::vec3(.5f, .5f, .5f);
    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        gfxm::aabb aabb;
        gfxm::vec3 vertices[8] = {
            { -half_extents.x, -half_extents.y, -half_extents.z },
            {  half_extents.x, -half_extents.y, -half_extents.z },
            {  half_extents.x, -half_extents.y,  half_extents.z },
            { -half_extents.x, -half_extents.y,  half_extents.z },
            { -half_extents.x,  half_extents.y, -half_extents.z },
            {  half_extents.x,  half_extents.y, -half_extents.z },
            {  half_extents.x,  half_extents.y,  half_extents.z },
            { -half_extents.x,  half_extents.y,  half_extents.z }
        };
        for (int i = 0; i < 8; ++i) {
            vertices[i] = transform * gfxm::vec4(vertices[i], 1.0f);
        }
        aabb.from = vertices[0];
        aabb.to = vertices[0];
        for (int i = 1; i < 8; ++i) {
            gfxm::expand_aabb(aabb, vertices[i]);
        }
        return aabb;
    }
};
class CollisionCapsuleShape : public CollisionShape {
public:
    CollisionCapsuleShape()
        : CollisionShape(COLLISION_SHAPE_TYPE::CAPSULE) {}
    float height = 1.0f;
    float radius = 0.5f;
    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        gfxm::aabb aabb0, aabb1;
        gfxm::vec3 A = transform * gfxm::vec4(.0f,  height * .5f, .0f, 1.f);
        gfxm::vec3 B = transform * gfxm::vec4(.0f, -height * .5f, .0f, 1.f);
        aabb0.from = A - gfxm::vec3(radius, radius, radius);
        aabb0.to = A + gfxm::vec3(radius, radius, radius);
        aabb1.from = B - gfxm::vec3(radius, radius, radius);
        aabb1.to = B + gfxm::vec3(radius, radius, radius);
        gfxm::aabb aabb;
        aabb.from.x = gfxm::_min(aabb0.from.x, aabb1.from.x);
        aabb.from.y = gfxm::_min(aabb0.from.y, aabb1.from.y);
        aabb.from.z = gfxm::_min(aabb0.from.z, aabb1.from.z);
        aabb.to.x = gfxm::_max(aabb0.to.x, aabb1.to.x);
        aabb.to.y = gfxm::_max(aabb0.to.y, aabb1.to.y);
        aabb.to.z = gfxm::_max(aabb0.to.z, aabb1.to.z);
        return aabb;
    }
};
#include "collision/collision_triangle_mesh.hpp"
class CollisionTriangleMeshShape : public CollisionShape {
    CollisionTriangleMesh* mesh = 0;
    const gfxm::vec3*   vertices = 0;
    const uint32_t*     indices = 0;
    int                 index_count = 0;
    gfxm::aabb local_aabb;
public:
    CollisionTriangleMeshShape()
    : CollisionShape(COLLISION_SHAPE_TYPE::TRIANGLE_MESH) {}
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

    void debugDraw(const gfxm::mat4& transform, uint32_t color) const {
        if (!mesh) {
            return;
        }
        mesh->debugDraw(transform, color);
    }
};


constexpr uint64_t COLLISION_LAYER_DEFAULT = 1;
constexpr uint64_t COLLISION_LAYER_CHARACTER = 1 << 1;
constexpr uint64_t COLLISION_LAYER_PROBE = 1 << 2;
constexpr uint64_t COLLISION_LAYER_BEACON = 1 << 3;

constexpr uint64_t COLLISION_MASK_EVERYTHING
    = COLLISION_LAYER_DEFAULT
    | COLLISION_LAYER_CHARACTER
    | COLLISION_LAYER_PROBE
    | COLLISION_LAYER_BEACON;


#include "aabb_tree/aabb_tree.hpp"
enum COLLIDER_USER {
    COLLIDER_USER_NONE,
    COLLIDER_USER_ACTOR,
    COLLIDER_USER_NODE
};
struct ColliderUserData {
    uint64_t    type;
    void*       user_ptr;
};
class CollisionWorld;
class Collider {
    friend CollisionWorld;
protected:
    int dirty_transform_index = -1;
    int flags = 0;
    COLLIDER_TYPE type;
    const CollisionShape* shape = 0;
    CollisionWorld* collision_world = 0;

    gfxm::vec3 center_offset = gfxm::vec3(0, 0, 0);
    gfxm::vec3 position = gfxm::vec3(0, 0, 0);
    gfxm::vec3 prev_pos = gfxm::vec3(0, 0, 0);
    gfxm::quat rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::mat4 world_transform = gfxm::mat4(1.f);
    gfxm::aabb world_aabb = gfxm::aabb(gfxm::vec3(-.5f, -.5f, -.5f), gfxm::vec3(.5f, .5f, .5f));

    Collider(COLLIDER_TYPE type)
        : type(type) {
        user_data.type = 0;
        user_data.user_ptr = 0;
        tree_elem.collider = this;
    }
public:
    ColliderUserData user_data;
    uint64_t collision_mask = COLLISION_LAYER_DEFAULT | COLLISION_LAYER_CHARACTER;
    uint64_t collision_group = COLLISION_LAYER_DEFAULT;    
    AabbTreeElement tree_elem;

    Collider()
    : type(COLLIDER_TYPE::COLLIDER) {
        tree_elem.collider = this;
    }
    virtual ~Collider() {}

    void setCenterOffset(const gfxm::vec3& offset) {
        center_offset = offset;
    }
    void setPosition(const gfxm::vec3& pos) {
        position = pos;
    }
    void setRotation(const gfxm::quat& q) {
        rotation = q;
    }
    void translate(const gfxm::vec3& t) {
        position += t;
    }

    void markAsExternallyTransformed();

    const gfxm::vec3& getPosition() const { return position; }
    const gfxm::quat& getRotation() const { return rotation; }

    const gfxm::mat4& getTransform() {
        world_transform
            = gfxm::translate(gfxm::mat4(1.f), position)
            * gfxm::to_mat4(rotation);
        return world_transform;
    }
    gfxm::mat4 getShapeTransform() {
        return gfxm::translate(gfxm::mat4(1.f), position)
        * gfxm::to_mat4(rotation)
        * gfxm::translate(gfxm::mat4(1.f), center_offset);
    }

    COLLIDER_TYPE getType() const {
        return type;
    }
    int getFlags() const {
        return flags;
    }

    const gfxm::aabb& getBoundingAabb() const {
        return world_aabb;
    }

    void setFlags(int flags) {
        this->flags = flags;
    }
    void setShape(const CollisionShape* shape) {
        this->shape = shape;
    }
    const CollisionShape* getShape() const {
        return shape;
    }
};

class ColliderProbe : public Collider {
    std::vector<Collider*> overlapping_colliders;
public:
    ColliderProbe()
    : Collider(COLLIDER_TYPE::PROBE) {
        flags |= COLLIDER_NO_RESPONSE;
    }

    int overlappingColliderCount() const {
        return overlapping_colliders.size();
    }
    Collider* getOverlappingCollider(int i) {
        return overlapping_colliders[i];
    }

    void _addOverlappingCollider(Collider* c) {
        overlapping_colliders.push_back(c);
    }
    void _clearOverlappingColliders() {
        overlapping_colliders.clear();
    }
};