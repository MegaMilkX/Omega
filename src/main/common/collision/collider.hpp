#pragma once

#include "common/math/gfxm.hpp"


enum class COLLISION_SHAPE_TYPE {
    UNKNOWN,
    SPHERE = 0b0001,
    BOX = 0b0010,
    CAPSULE = 0b0100
};
enum class COLLISION_PAIR_TYPE {
    UNKNOWN,
    SPHERE_SPHERE = (int)COLLISION_SHAPE_TYPE::SPHERE,                                          // 0b0001
    BOX_BOX = (int)COLLISION_SHAPE_TYPE::BOX,                                                   // 0b0010
    SPHERE_BOX = (int)COLLISION_SHAPE_TYPE::SPHERE | (int)COLLISION_SHAPE_TYPE::BOX,            // 0b0011
    CAPSULE_CAPSULE = (int)COLLISION_SHAPE_TYPE::CAPSULE,                                       // 0b0100
    CAPSULE_SPHERE = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::SPHERE,    // 0b0101
    CAPSULE_BOX = (int)COLLISION_SHAPE_TYPE::CAPSULE | (int)COLLISION_SHAPE_TYPE::BOX,          // 0b0110
};

enum class COLLIDER_TYPE {
    COLLIDER,
    PROBE
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
};
class CollisionSphereShape : public CollisionShape {
public:
    CollisionSphereShape()
        : CollisionShape(COLLISION_SHAPE_TYPE::SPHERE) {}
    float radius = .5f;
};
class CollisionBoxShape : public CollisionShape {
public:
    CollisionBoxShape()
        : CollisionShape(COLLISION_SHAPE_TYPE::BOX) {}
    gfxm::vec3 half_extents = gfxm::vec3(.5f, .5f, .5f);
};
class CollisionCapsuleShape : public CollisionShape {
public:
    CollisionCapsuleShape()
        : CollisionShape(COLLISION_SHAPE_TYPE::CAPSULE) {}
    float height = 1.0f;
    float radius = 0.5f;
};


class Collider {
protected:
    COLLIDER_TYPE type;
    const CollisionShape* shape = 0;
    void* user_ptr = 0;

    Collider(COLLIDER_TYPE type)
        : type(type) {}
public:
    gfxm::vec3 position = gfxm::vec3(0, 0, 0);
    gfxm::quat rotation = gfxm::quat(0, 0, 0, 1);

    Collider()
    : type(COLLIDER_TYPE::COLLIDER) {}
    virtual ~Collider() {}

    COLLIDER_TYPE getType() const {
        return type;
    }

    void setShape(const CollisionShape* shape) {
        this->shape = shape;
    }
    const CollisionShape* getShape() const {
        return shape;
    }

    void setUserPtr(void* udata) {
        user_ptr = udata;
    }
    void* getUserPtr() const {
        return user_ptr;
    }
};

class ColliderProbe : public Collider {
    std::vector<Collider*> overlapping_colliders;
public:
    ColliderProbe()
    : Collider(COLLIDER_TYPE::PROBE) {

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