#pragma once

#include "math/gfxm.hpp"
#include <vector>
#include "debug_draw/debug_draw.hpp"
#include "collision/collision_contact_point.hpp"
#include "collision/intersection/sphere_capsule.hpp"
#include "collision/intersection/ray.hpp"

enum class COLLISION_SHAPE_TYPE {
    UNKNOWN,
    SPHERE = 0b00001,
    BOX = 0b00010,
    CAPSULE = 0b00100,
    TRIANGLE_MESH = 0b01000,
    CONVEX_MESH = 0b10000
};
enum class COLLISION_PAIR_TYPE {
    UNKNOWN,

    SPHERE_SPHERE = (int)COLLISION_SHAPE_TYPE::SPHERE,                                          // 0b0001

    BOX_BOX = (int)COLLISION_SHAPE_TYPE::BOX,                                                   // 0b0010
    SPHERE_BOX = (int)COLLISION_SHAPE_TYPE::SPHERE | (int)COLLISION_SHAPE_TYPE::BOX,            // 0b0011

    CAPSULE_CAPSULE = (int)COLLISION_SHAPE_TYPE::CAPSULE,                                       // 0b0100
    CAPSULE_SPHERE = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::SPHERE,    // 0b0101
    CAPSULE_BOX = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::BOX,          // 0b0110

    SPHERE_TRIANGLEMESH = (int)COLLISION_SHAPE_TYPE::SPHERE | (int)COLLISION_SHAPE_TYPE::TRIANGLE_MESH, // 0b1001
    BOX_TRIANGLEMESH = (int)COLLISION_SHAPE_TYPE::BOX | (int)COLLISION_SHAPE_TYPE::TRIANGLE_MESH, // 0b1010
    CAPSULE_TRIANGLEMESH = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::TRIANGLE_MESH, // 0b1100

    SPHERE_CONVEX_MESH = (int)COLLISION_SHAPE_TYPE::SPHERE | (int)COLLISION_SHAPE_TYPE::CONVEX_MESH,
    BOX_CONVEX_MESH = (int)COLLISION_SHAPE_TYPE::BOX | (int)COLLISION_SHAPE_TYPE::CONVEX_MESH,
    CAPSULE_CONVEX_MESH = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::CONVEX_MESH,
    TRIANGLE_MESH_CONVEX_MESH = (int)COLLISION_SHAPE_TYPE::TRIANGLE_MESH | (int)COLLISION_SHAPE_TYPE::CONVEX_MESH,
    CONVEX_MESH_CONVEX_MESH = (int)COLLISION_SHAPE_TYPE::CONVEX_MESH,
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
    virtual gfxm::mat3 calcInertiaTensor(float mass) const {
        if (mass == .0f) {
            return gfxm::mat3(.0f);
        }
        return gfxm::mat3(1.f);
    }

    virtual gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir, const gfxm::mat4& transform) const {
        assert(false);
        return transform[3];
    }
};
class CollisionSphereShape : public CollisionShape {
public:
    CollisionSphereShape()
        : CollisionShape(COLLISION_SHAPE_TYPE::SPHERE) {}
    float radius = .25f;
    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override {
        gfxm::aabb aabb;
        aabb.from = gfxm::vec3(-radius, -radius, -radius) + gfxm::vec3(transform[3]);
        aabb.to = gfxm::vec3(radius, radius, radius) + gfxm::vec3(transform[3]);
        return aabb;
    }
    gfxm::mat3 calcInertiaTensor(float mass) const override {
        float I = .4f * mass * powf(radius, 2.f);
        gfxm::mat3 inertia;
        inertia[0] = gfxm::vec3(I, .0f, .0f);
        inertia[1] = gfxm::vec3(.0f, I, .0f);
        inertia[2] = gfxm::vec3(.0f, .0f, I);
        return inertia;
    }
    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir, const gfxm::mat4& transform) const override {
        gfxm::vec3 lcl_dir = transform * gfxm::vec4(dir, .0f);
        gfxm::vec3 P = gfxm::normalize(lcl_dir) * radius;
        return transform * gfxm::vec4(P, 1.f);
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

    gfxm::mat3 calcInertiaTensor(float mass) const override {
        float w2 = half_extents.x * half_extents.x;
        float h2 = half_extents.y * half_extents.y;
        float d2 = half_extents.z * half_extents.z;
        float Iw = (1.f / 12.f) * mass * (d2 + h2);
        float Ih = (1.f / 12.f) * mass * (w2 + d2);
        float Id = (1.f / 12.f) * mass * (w2 + h2);
        gfxm::mat3 inertia;
        inertia[0] = gfxm::vec3(Iw, .0f, .0f);
        inertia[1] = gfxm::vec3(.0f, Ih, .0f);
        inertia[2] = gfxm::vec3(.0f, .0f, Id);
        return inertia;
    }

    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir_, const gfxm::mat4& transform) const override {
        const gfxm::vec3 dir = gfxm::inverse(transform) * gfxm::vec4(dir_, .0f);
        gfxm::vec3 result = gfxm::vec3(
            (dir.x >= 0.0f ? half_extents.x : -half_extents.x),
            (dir.y >= 0.0f ? half_extents.y : -half_extents.y),
            (dir.z >= 0.0f ? half_extents.z : -half_extents.z)
        );
        result = transform * gfxm::vec4(result, 1.f);
        return result;
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

    gfxm::vec3 getMinkowskiSupportPoint(const gfxm::vec3& dir_, const gfxm::mat4& transform) const override {
        const gfxm::vec3 dir = gfxm::inverse(transform) * gfxm::vec4(dir_, .0f);

        const float half_height = height * .5f;
        gfxm::vec3 res(0.0f, 0.0f, 0.0f);

        float len = gfxm::length(dir);
        gfxm::vec3 ndir = dir / len;

        // Pick top or bottom sphere center based on direction.y
        gfxm::vec3 capCenter = (dir.y >= 0.0f)
            ? gfxm::vec3(0.0f,  half_height, 0.0f)
            : gfxm::vec3(0.0f, -half_height, 0.0f);

        // Furthest point is sphere center + radius in direction
        res = capCenter + ndir * radius;

        res = transform * gfxm::vec4(res, 1.f);
        return res;
    }
};

class CollisionConvexMesh {
    std::vector<gfxm::vec3> vertices;
    std::vector<int> indices;

    void fixWinding() {
        if (vertices.empty() || indices.empty()) {
            return;
        }
        gfxm::vec3 centroid;
        for (int i = 0; i < vertices.size(); ++i) {
            centroid += vertices[i];
        }
        centroid /= (float)vertices.size();

        for (int i = 0; i < indices.size(); i += 3) {
            const gfxm::vec3& a = vertices[indices[i + 0]];
            const gfxm::vec3& b = vertices[indices[i + 1]];
            const gfxm::vec3& c = vertices[indices[i + 2]];

            const gfxm::vec3 N = gfxm::normalize(gfxm::cross(b - a, c - a));
            const gfxm::vec3 to_center = centroid - a;
            if (gfxm::dot(N, to_center) > .0f) {
                std::swap(indices[i + 1], indices[i + 2]);
            }
        }
    }
public:
    void setData(const gfxm::vec3* verts, int vertex_count, const int* inds, int index_count) {
        vertices.clear();
        vertices.insert(vertices.end(), verts, verts + vertex_count);
        indices.clear();
        indices.insert(indices.end(), inds, inds + index_count);

        fixWinding();
    }

    int vertexCount() const {
        return vertices.size();
    }
    const gfxm::vec3* getVertexData() const {
        return vertices.data();
    }

    int indexCount() const {
        return indices.size();
    }
    const int* getIndexData() const {
        return indices.data();
    }

    bool intersectRay(const gfxm::vec3& rO, const gfxm::vec3& rV) {
        assert(false);
        return false;
    }

    void rayTest(const gfxm::ray& ray, void* context, void(*callback_fn)(void*, const RayHitPoint&)) const {
        // TODO: Optimize

        for (int i = 0; i < indices.size(); i += 3) {
            const gfxm::vec3& A = vertices[indices[i + 0]];
            const gfxm::vec3& B = vertices[indices[i + 1]];
            const gfxm::vec3& C = vertices[indices[i + 2]];

            RayHitPoint rhp;
            if (intersectRayTriangle(ray, A, B, C, rhp)) {
                callback_fn(context, rhp);
            }
        }
    }

    void sweptSphereTest(const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius, void* context, void(*callback_fn)(void*, const SweepContactPoint&)) const {
        // TODO: Optimize

        for (int i = 0; i < indices.size(); i += 3) {
            const gfxm::vec3& A = vertices[indices[i + 0]];
            const gfxm::vec3& B = vertices[indices[i + 1]];
            const gfxm::vec3& C = vertices[indices[i + 2]];
            
            SweepContactPoint scp;
            if (intersectionSweepSphereTriangle(from, to, sweep_radius, A, B, C, scp)) {
                callback_fn(context, scp);
            }
        }
    }
    
    void debugDraw(const gfxm::mat4& transform, uint32_t color) const {
        if (indices.empty()) {
            assert(false);
            return;
        }
        for (int i = 0; i < indices.size(); i += 3) {
            dbgDrawLine(
                transform * gfxm::vec4(vertices[indices[i]], 1.f),
                transform * gfxm::vec4(vertices[indices[i + 1]], 1.f),
                color
            );
            dbgDrawLine(
                transform * gfxm::vec4(vertices[indices[i + 1]], 1.f),
                transform * gfxm::vec4(vertices[indices[i + 2]], 1.f),
                color
            );
            dbgDrawLine(
                transform * gfxm::vec4(vertices[indices[i + 2]], 1.f),
                transform * gfxm::vec4(vertices[indices[i]], 1.f),
                color
            );
        }/*
        for (int i = 0; i < vertices.size(); ++i) {
            dbgDrawSphere(transform * gfxm::vec4(vertices[i], 1.f), .01f, 0xFF0000FF);
        }*/
    }
};
class CollisionConvexShape : public CollisionShape {
    const CollisionConvexMesh*  mesh = 0;
    const gfxm::vec3*           vertices = 0;
    gfxm::aabb                  local_aabb;
    gfxm::mat3                  inertia_tensor;
public:
    CollisionConvexShape()
        : CollisionShape(COLLISION_SHAPE_TYPE::CONVEX_MESH) {
        inertia_tensor = gfxm::mat3(1.f);
    }
    
    void setMesh(const CollisionConvexMesh* mesh) {
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
    const CollisionConvexMesh* getMesh() const {
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


constexpr uint64_t COLLISION_LAYER_DEFAULT = 0x0001;
constexpr uint64_t COLLISION_LAYER_CHARACTER = 0x0002;
constexpr uint64_t COLLISION_LAYER_PROBE = 0x0004;
constexpr uint64_t COLLISION_LAYER_BEACON = 0x0008;
constexpr uint64_t COLLISION_LAYER_PROJECTILE = 0x0010;

constexpr uint64_t COLLISION_MASK_EVERYTHING
    = COLLISION_LAYER_DEFAULT
    | COLLISION_LAYER_CHARACTER
    | COLLISION_LAYER_PROBE
    | COLLISION_LAYER_BEACON
    | COLLISION_LAYER_PROJECTILE;


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

class CollisionNarrowPhase;
class CollisionWorld;
class Collider {
    friend CollisionNarrowPhase;
    friend CollisionWorld;
protected:
    uint32_t id;    // Used to look up cached manifolds
                    // and avoid duplicate pairs (AxB, BxA)
    int dirty_transform_index = -1;
    int flags = 0;
    COLLIDER_TYPE type;
    const CollisionShape* shape = 0;
    CollisionWorld* collision_world = 0;

    gfxm::vec3 center_offset = gfxm::vec3(0, 0, 0);
    gfxm::vec3 position = gfxm::vec3(0, 0, 0);
    gfxm::vec3 prev_pos = gfxm::vec3(0, 0, 0);
    gfxm::quat rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::quat prev_rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::mat4 world_transform = gfxm::mat4(1.f);
    gfxm::aabb world_aabb = gfxm::aabb(gfxm::vec3(-.5f, -.5f, -.5f), gfxm::vec3(.5f, .5f, .5f));

    gfxm::vec3 correction_accum;

    Collider(COLLIDER_TYPE type)
        : type(type) {
        user_data.type = 0;
        user_data.user_ptr = 0;
        tree_elem.collider = this;
    }
public:
    ColliderUserData user_data = { 0 };
    uint64_t collision_mask = COLLISION_LAYER_DEFAULT | COLLISION_LAYER_CHARACTER | COLLISION_LAYER_PROJECTILE;
    uint64_t collision_group = COLLISION_LAYER_DEFAULT;    
    AabbTreeElement tree_elem;

    // DYNAMICS TEST
    float mass = .0f;
    float friction = .6f;
    float gravity_factor = 1.0f;
    gfxm::vec3 mass_center;
    gfxm::vec3 velocity;
    gfxm::vec3 angular_velocity;
    gfxm::mat3 inertia_tensor = gfxm::mat3(1);
    gfxm::mat3 inverse_inertia_tensor = gfxm::mat3(1);
    float sleep_timer = .0f;
    bool is_sleeping = false;
    // ====

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
    void rotate(float angle, const gfxm::vec3& axis) {
        rotation = gfxm::angle_axis(angle, axis) * rotation;
    }
    void rotate(const gfxm::quat& q) {
        rotation = q * rotation;
    }

    void markAsExternallyTransformed();

    gfxm::vec3 getCenterOffset() {
        return center_offset;
    }

    const gfxm::vec3& getPosition() const { return position; }
    const gfxm::quat& getRotation() const { return rotation; }
    gfxm::vec3        getCOM() const { return position + gfxm::to_mat3(rotation) * mass_center; }

    const gfxm::mat4& getTransform() {
        world_transform
            = gfxm::translate(gfxm::mat4(1.f), position)
            * gfxm::to_mat4(rotation);
        return world_transform;
    }
    gfxm::mat4 getPrevTransform() {
        return gfxm::translate(gfxm::mat4(1.f), prev_pos)
            * gfxm::to_mat4(prev_rotation);
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

    void calcInertiaTensor() {
        if (!shape) {
            assert(false);
            return;
        }
        inertia_tensor = shape->calcInertiaTensor(mass);
        inverse_inertia_tensor = gfxm::inverse(inertia_tensor);
    }


    // DYNAMICS TEST
    void impulseAtPoint(const gfxm::vec3& impulse, const gfxm::vec3& point) {
        if (mass < FLT_EPSILON) {
            return;
        }

        velocity += impulse * (1.0f / mass);

        gfxm::vec3 wCOM = getPosition() + gfxm::to_mat3(getRotation()) * mass_center;
        gfxm::vec3 r = point - wCOM;
        gfxm::vec3 torque = gfxm::cross(r, impulse);
        gfxm::mat3 inverse_inertia_tensor_world = getInverseWorldInertiaTensor();
        angular_velocity += inverse_inertia_tensor_world * torque;
    }

    float getInverseMass() {
        return mass > .0f ? (1.0f / mass) : .0f;
    }/*
    gfxm::mat3 getInverseWorldInertiaTensor() {
        return
            gfxm::to_mat3(getTransform()) *
            inverse_inertia_tensor *
            gfxm::to_mat3(gfxm::transpose(getTransform()));
    }*/
    gfxm::mat3 getInverseWorldInertiaTensor() {
        if (mass == .0f) {
            return gfxm::mat3(.0f);
        }
        gfxm::mat3 R = gfxm::to_mat3(getRotation());
        return R * inverse_inertia_tensor * gfxm::transpose(R);
    }
    // ====
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