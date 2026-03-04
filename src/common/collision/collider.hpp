#pragma once

#include "math/gfxm.hpp"
#include <vector>
#include "debug_draw/debug_draw.hpp"
#include "collision/collision_contact_point.hpp"
#include "collision/intersection/sphere_capsule.hpp"
#include "collision/intersection/ray.hpp"

#include "shape/shape.hpp"


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

class phyNarrowPhase;
class phyWorld;
class phyRigidBody {
    friend phyNarrowPhase;
    friend phyWorld;
protected:
    uint32_t id;    // Used to look up cached manifolds
                    // and avoid duplicate pairs (AxB, BxA)
    int dirty_transform_index = -1;
    int flags = 0;
    PHY_COLLIDER_TYPE type;
    const phyShape* shape = 0;
    phyWorld* collision_world = 0;

    gfxm::vec3 center_offset = gfxm::vec3(0, 0, 0);
    gfxm::vec3 position = gfxm::vec3(0, 0, 0);
    gfxm::vec3 prev_pos = gfxm::vec3(0, 0, 0);
    gfxm::quat rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::quat prev_rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::mat4 world_transform = gfxm::mat4(1.f);
    gfxm::aabb world_aabb = gfxm::aabb(gfxm::vec3(-.5f, -.5f, -.5f), gfxm::vec3(.5f, .5f, .5f));

    gfxm::vec3 correction_accum;

    phyRigidBody(PHY_COLLIDER_TYPE type)
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
    float friction = .3f;
    float gravity_factor = 1.0f;
    gfxm::vec3 mass_center;
    gfxm::vec3 velocity;
    gfxm::vec3 angular_velocity;
    gfxm::mat3 inertia_tensor = gfxm::mat3(1);
    gfxm::mat3 inverse_inertia_tensor = gfxm::mat3(1);
    float sleep_timer = .0f;
    bool is_sleeping = false;
    // ====

    phyRigidBody()
    : type(PHY_COLLIDER_TYPE::COLLIDER) {
        tree_elem.collider = this;
    }
    virtual ~phyRigidBody() {}

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

    PHY_COLLIDER_TYPE getType() const {
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
    void setShape(const phyShape* shape) {
        this->shape = shape;
    }
    const phyShape* getShape() const {
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

class phyProbe : public phyRigidBody {
    std::vector<phyRigidBody*> overlapping_colliders;
public:
    phyProbe()
    : phyRigidBody(PHY_COLLIDER_TYPE::PROBE) {
        flags |= COLLIDER_NO_RESPONSE;
    }

    int overlappingColliderCount() const {
        return overlapping_colliders.size();
    }
    phyRigidBody* getOverlappingCollider(int i) {
        return overlapping_colliders[i];
    }

    void _addOverlappingCollider(phyRigidBody* c) {
        overlapping_colliders.push_back(c);
    }
    void _clearOverlappingColliders() {
        overlapping_colliders.clear();
    }
};