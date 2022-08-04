#pragma once

#include <assert.h>
#include <vector>
#include <unordered_map>
#include "math/gfxm.hpp"
#include "collision/intersection/boxbox.hpp"
#include "collision/intersection/sphere_box.hpp"

#include "collision/collider.hpp"

#include "debug_draw/debug_draw.hpp"

constexpr int MAX_CONTACT_POINTS = 2048;

class CollisionWorld {
    std::vector<Collider*> colliders;
    std::vector<ColliderProbe*> probes;
    std::unordered_map<COLLISION_PAIR_TYPE, std::vector<std::pair<Collider*, Collider*>>> potential_pairs;
    std::vector<CollisionManifold> manifolds;
    ContactPoint contact_points[MAX_CONTACT_POINTS];
    int          contact_point_count = 0;

    void clearContactPoints() {
        contact_point_count = 0;
    }
public:
    void addCollider(Collider* collider);
    void removeCollider(Collider* collider);

    void castSphere(const gfxm::mat4& tr, float radius);

    void debugDraw();

    void update();

    // 
    int addContactPoint(const gfxm::vec3& pos, const gfxm::vec3& normal, float depth);
    ContactPoint* getContactPointArrayEnd();
};
