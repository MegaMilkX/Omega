#pragma once

#include <assert.h>
#include <vector>
#include <unordered_map>
#include "math/gfxm.hpp"
#include "collision/intersection/boxbox.hpp"
#include "collision/intersection/sphere_box.hpp"
#include "collision/intersection/sphere_capsule.hpp"
#include "collision/intersection/capsule_capsule.hpp"
#include "collision/intersection/capsule_triangle.hpp"

#include "collision/collider.hpp"

#include "debug_draw/debug_draw.hpp"

#include "aabb_tree/aabb_tree.hpp"

constexpr int MAX_CONTACT_POINTS = 2048;

struct RayCastResult {
    gfxm::vec3 position;
    gfxm::vec3 normal;
    Collider* collider;
    float distance;
    bool hasHit;
};

class CollisionWorld {
    std::vector<Collider*> colliders;
    std::vector<ColliderProbe*> probes;
    std::unordered_map<COLLISION_PAIR_TYPE, std::vector<std::pair<Collider*, Collider*>>> potential_pairs;
    std::vector<CollisionManifold> manifolds;
    ContactPoint contact_points[MAX_CONTACT_POINTS];
    int          contact_point_count = 0;

    AabbTree aabb_tree;

    std::vector<Collider*> dirty_transform_array;
    int dirty_transform_count = 0;

    void clearContactPoints() {
        contact_point_count = 0;
    }
    void _setColliderTransformDirty(Collider* collider);
    void _addColliderToDirtyTransformArray(Collider* collider);
    void _removeColliderFromDirtyTransformArray(Collider* collider);
public:
    void addCollider(Collider* collider);
    void removeCollider(Collider* collider);

    RayCastResult rayTest(const gfxm::vec3& from, const gfxm::vec3& to, uint64_t mask = COLLISION_MASK_EVERYTHING);
    void sphereTest(const gfxm::mat4& tr, float radius);

    void debugDraw();

    void update(float dt);

    int dirtyTransformCount() const { return dirty_transform_count; }
    const Collider* const* getDirtyTransformArray() const { return dirty_transform_array.data(); }

    // 
    int addContactPoint(
        const gfxm::vec3& pos_a, const gfxm::vec3& pos_b, 
        const gfxm::vec3& normal_a, const gfxm::vec3& normal_b,
        float depth
    );
    int addContactPoint(const ContactPoint& cp);
    ContactPoint* getContactPointArrayEnd();
};
