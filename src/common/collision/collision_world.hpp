#pragma once

#include <assert.h>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include "math/gfxm.hpp"
#include "collision/common.hpp"
#include "collision/collision_manifold.hpp"
#include "collision/intersection/box_box.hpp"
#include "collision/intersection/sphere_box.hpp"
#include "collision/intersection/sphere_capsule.hpp"
#include "collision/intersection/capsule_capsule.hpp"
#include "collision/intersection/capsule_triangle.hpp"
#include "collision/intersection/gjkepa/epa_types.hpp"

#include "collision/collider.hpp"

#include "debug_draw/debug_draw.hpp"

#include "aabb_tree/aabb_tree.hpp"

#define COLLISION_DBG_DRAW_AABB_TREE 0
#define COLLISION_DBG_DRAW_COLLIDERS 1
#define COLLISION_DBG_DRAW_CONTACT_POINTS 1
#define COLLISION_DBG_DRAW_TESTS 1

constexpr int MAX_MANIFOLDS = 8192;

struct RayCastResult {
    gfxm::vec3 position;
    gfxm::vec3 normal;
    Collider* collider = 0;
    CollisionSurfaceProp prop;
    float distance = .0f;
    bool hasHit = false;
};
struct SphereSweepResult {
    gfxm::vec3 contact;
    gfxm::vec3 normal;
    gfxm::vec3 sphere_pos;
    Collider* collider = 0;
    CollisionSurfaceProp prop;
    float distance = .0f;
    bool hasHit = false;
};
/*
inline uint16_t makeNormalKey(const gfxm::vec3& N) {
    float phi = atan2f(N.z, N.x);
    float theta = acosf(gfxm::clamp(N.y, -1.f, 1.f));
    if(phi < .0f) phi += 2.0f * gfxm::pi;
    int t = int(theta / gfxm::pi * 31.f + .5f);
    int p = int(phi / (2.0f * gfxm::pi) * 63.f + .5f);
    return t + (p << 8);
}*/

inline uint16_t makeNormalKey(const gfxm::vec3& N) {
    float u = gfxm::dot(N, gfxm::vec3(.0f, 1.f, .0f));
    float v = gfxm::dot(N, gfxm::vec3(1.f, .0f, .0f));
    float w = gfxm::dot(N, gfxm::vec3(.0f, .0f, 1.f));
    u = (u + 1.f) * .5f;
    v = (v + 1.f) * .5f;
    w = (w + 1.f) * .5f;
    int iu = u * 8.f;
    int iv = v * 8.f;
    int iw = w * 8.f;
    return (uint16_t(iu) << 10) + (uint16_t(iv) << 5) + uint16_t(w);
}
/*
inline uint16_t makeNormalKey(const gfxm::vec3& n) {
    // Normalize
    gfxm::vec3 N = gfxm::normalize(n);

    // Octahedral projection
    float ax = fabsf(N.x);
    float ay = fabsf(N.y);
    float az = fabsf(N.z);
    float invL1 = 1.0f / (ax + ay + az);

    float u = N.x * invL1;
    float v = N.z * invL1;
    if (N.y < 0.0f) {
        float oldU = u;
        u = (1.0f - fabsf(v)) * (u >= 0.0f ? 1.0f : -1.0f);
        v = (1.0f - fabsf(oldU)) * (v >= 0.0f ? 1.0f : -1.0f);
    }

    // Quantize to 8 bits each
    int iu = int((u * 0.5f + 0.5f) * 255.0f + 0.5f);
    int iv = int((v * 0.5f + 0.5f) * 255.0f + 0.5f);

    uint16_t k = uint16_t(iu | (iv << 8));
    if(k == 0x7f7f) k = 0x8080;
    return k;
}*/

// 0xBBBBBB AAAAAA NNNN
typedef uint64_t manifold_key_t;
#define MAKE_MANIFOLD_KEY(A, B, NORMAL_KEY) \
    manifold_key_t(uint64_t(NORMAL_KEY) | uint64_t(std::min(A, B)) << 16 | (uint64_t(std::max(A, B)) << 40))


struct NarrowPhaseData {
    std::unordered_map<manifold_key_t, int> pair_manifold_table;
    CollisionArray<CollisionManifold, MAX_MANIFOLDS> manifolds;
};

class CollisionNarrowPhase {
    NarrowPhaseData buffers[2];
    NarrowPhaseData* front = &buffers[0];
    NarrowPhaseData* back = &buffers[1];
    int tick_id = 0;

    CollisionManifold* findCachedManifold(manifold_key_t key) {
        auto it = back->pair_manifold_table.find(key);
        if (it == back->pair_manifold_table.end()) {
            return 0;
        }
        return &back->manifolds[it->second];
    }
    CollisionManifold* createNewManifold(manifold_key_t key) {
        auto it_second =
            front->pair_manifold_table.insert(std::make_pair(key, front->manifolds.count())).first;
        front->manifolds.push_back(CollisionManifold());
        return &front->manifolds[front->manifolds.count() - 1];
    }

public:
    int manifoldCount() const {
        return front->manifolds.count();
    }
    CollisionManifold& getManifold(int at) {
        return front->manifolds[at];
    }

    int oldManifoldCount() const {
        return back->manifolds.count();
    }
    CollisionManifold& getOldManifold(int at) {
        return back->manifolds[at];
    }

    void flipBuffers(int tick_id) {
        this->tick_id = tick_id;
        
        std::swap(front, back);
        front->pair_manifold_table.clear();
        front->manifolds.clear();
    }

    CollisionManifold* getManifold(Collider* a, Collider* b, const gfxm::vec3& N, bool use_cached = true) {
        return getManifold(a, b, makeNormalKey(N), use_cached);
    }
    CollisionManifold* getManifold(Collider* a, Collider* b, uint16_t normal_key, bool use_cached = true) {
        manifold_key_t key = MAKE_MANIFOLD_KEY(a->id, b->id, normal_key);

        auto it = front->pair_manifold_table.find(key);
        if (it == front->pair_manifold_table.end()) {
            it = front->pair_manifold_table.insert(std::make_pair(key, front->manifolds.count())).first;
            front->manifolds.push_back(CollisionManifold(key));
        }

        CollisionManifold* M = &front->manifolds[it->second];
        M->collider_a = a;
        M->collider_b = b;
        if (use_cached) {
            CollisionManifold* Mcached = findCachedManifold(key);
            if(Mcached) {
                *M = *Mcached;
            }
        }

        return M;
    }

    void addContact(
        CollisionManifold* manifold,
        const gfxm::vec3& pt_a,
        const gfxm::vec3& pt_b,
        const gfxm::vec3& normal_a,
        const gfxm::vec3& normal_b,
        float distance
    ) {
        ContactPoint cp;
        cp.point_a = pt_a;
        cp.point_b = pt_b;
        cp.normal_a = normal_a;
        cp.normal_b = normal_b;
        cp.depth = distance;
        addContact(manifold, cp);
    }

    void primeManifold(CollisionManifold* manifold, ContactPoint& cp) {
        auto buildTangentBasis = [](const gfxm::vec3& N, gfxm::vec3& t1, gfxm::vec3& t2) {
            gfxm::vec3 ref = (fabsf(N.x) > .9f) ? gfxm::vec3(.0f, 1.f, .0f) : gfxm::vec3(1.f, .0f, .0f);
            t1 = gfxm::normalize(gfxm::cross(N, ref));
            t2 = gfxm::cross(N, t1);
        };
        buildTangentBasis(cp.normal_a, manifold->t1, manifold->t2);
        auto project = [](const gfxm::vec3& t1, const gfxm::vec3& t2, const gfxm::vec3& p)->gfxm::vec2{
            return gfxm::vec2(gfxm::dot(t1, p), gfxm::dot(t2, p));
        };
        gfxm::vec2 p2 = project(manifold->t1, manifold->t2, cp.point_a);
        manifold->extremes.min = p2;
        manifold->extremes.max = p2;
    }
    void removeManifold(int i) {
        auto removed_key = front->manifolds[i].key;
        std::swap(front->manifolds[i], front->manifolds[front->manifolds.count() - 1]);
        front->manifolds.resize(front->manifolds.count() - 1);
        front->pair_manifold_table[front->manifolds[i].key] = i;
        front->pair_manifold_table.erase(removed_key);
    }
    void rebuildManifold(CollisionManifold* m) {
        if (m->pointCount() == 0) {
            return;
        }
        primeManifold(m, m->points[0]);
        for (int i = 1; i < m->pointCount(); ++i) {
            auto& cp = m->points[i];
            auto project = [](const gfxm::vec3& t1, const gfxm::vec3& t2, const gfxm::vec3& p)->gfxm::vec2{
                return gfxm::vec2(gfxm::dot(t1, p), gfxm::dot(t2, p));
            };
            gfxm::vec2 p2 = project(m->t1, m->t2, cp.point_a);
            if(p2.x < m->extremes.min.x) { m->iminx = i; }
            if(p2.y < m->extremes.min.y) { m->iminy = i; }
            if(p2.x > m->extremes.max.x) { m->imaxx = i; }
            if(p2.y > m->extremes.max.y) { m->imaxy = i; }
            gfxm::expand(m->extremes, p2);
        }
    }

    void addContact3(CollisionManifold* m, ContactPoint& cp) {
        float EPS_DIST = .02f;
        for (int i = 0; i < m->pointCount(); ++i) {
            ContactPoint& oldcp = m->points[i];
            if (gfxm::length(oldcp.point_a - cp.point_a) < EPS_DIST
                || gfxm::length(oldcp.point_b - cp.point_b) < EPS_DIST)
            {
                cp.bias = oldcp.bias;
                cp.dbg_vn = oldcp.dbg_vn;
                cp.Jn_acc = oldcp.Jn_acc;
                cp.Jt_acc = oldcp.Jt_acc;
                cp.mass_normal = oldcp.mass_normal;
                cp.mass_tangent1 = oldcp.mass_tangent1;
                cp.mass_tangent2 = oldcp.mass_tangent2;
                cp.tick_id = oldcp.tick_id;
                m->points[i] = cp;
                return;
            }
        }

        if (m->pointCount() < m->points.capacity()) {
            m->points.push_back(cp);
            return;
        }

        std::sort(&m->points[0], &m->points[0] + m->pointCount(), [](const ContactPoint& p1, const ContactPoint& p2)->bool {
            return p1.depth < p2.depth;
        });

        if (cp.depth > m->points[0].depth) {
            m->points[0] = cp;
            return;
        }
    }
    void addContact2(CollisionManifold* m, ContactPoint& cp) {
        auto project = [](const gfxm::vec3& t1, const gfxm::vec3& t2, const gfxm::vec3& p)->gfxm::vec2{
            return gfxm::vec2(gfxm::dot(t1, p), gfxm::dot(t2, p));
        };
        gfxm::vec2 p2 = project(m->t1, m->t2, cp.point_a);

        bool exceeds_depth = m->pointCount() == 0;
        float min_depth = cp.depth;
        int min_depth_idx = 0;
        for (int i = 0; i < m->pointCount(); ++i) {
            if (m->points[i].depth < min_depth) {
                min_depth = m->points[i].depth;
                min_depth_idx = i;
                exceeds_depth = true;
            }
        }
        if (exceeds_depth && m->pointCount() == 4) {
            m->points[min_depth_idx] = cp;
            gfxm::expand(m->extremes, p2);
            return;
        }

        if (m->pointCount() < m->points.capacity()) {
            if(p2.x < m->extremes.min.x) { m->iminx = m->pointCount(); }
            if(p2.y < m->extremes.min.y) { m->iminy = m->pointCount(); }
            if(p2.x > m->extremes.max.x) { m->imaxx = m->pointCount(); }
            if(p2.y > m->extremes.max.y) { m->imaxy = m->pointCount(); }
            gfxm::expand(m->extremes, p2);
            m->points.push_back(cp);
        }
        if(p2.x <= m->extremes.min.x) {
            m->points[m->iminx] = cp;
            gfxm::expand(m->extremes, p2);
        }
        if(p2.y <= m->extremes.min.y) {
            m->points[m->iminy] = cp;
            gfxm::expand(m->extremes, p2);
        }
        if(p2.x >= m->extremes.max.x) {
            m->points[m->imaxx] = cp;
            gfxm::expand(m->extremes, p2);
        }
        if(p2.y >= m->extremes.max.y) {
            m->points[m->imaxy] = cp;
            gfxm::expand(m->extremes, p2);
        }
    }

    void addContact(CollisionManifold* manifold, ContactPoint& cp) {
        if (!cp.normal_a.is_valid()) {
            //return;
        }

        if (manifold->pointCount() == 0) {
            primeManifold(manifold, cp);
        }

        cp.lcl_point_a = gfxm::inverse(manifold->collider_a->getTransform()) * gfxm::vec4(cp.point_a, 1.f);
        cp.lcl_point_b = gfxm::inverse(manifold->collider_b->getTransform()) * gfxm::vec4(cp.point_b, 1.f);
        cp.lcl_normal = gfxm::inverse(gfxm::to_mat3(manifold->collider_a->getRotation())) * cp.normal_a;

        cp.tick_id = tick_id;

        addContact3(manifold, cp);
    }
};

class CollisionWorld {
    std::vector<Collider*> colliders;
    std::vector<ColliderProbe*> probes;
    std::unordered_map<COLLISION_PAIR_TYPE, std::vector<std::pair<Collider*, Collider*>>> potential_pairs;
    CollisionNarrowPhase narrow_phase;

    float dt_accum = .0f;
    int tick_id = 0;

    EPA_Context epa_ctx;

    AabbTree aabb_tree;

    std::vector<Collider*> dirty_transform_array;
    int dirty_transform_count = 0;

    bool dbg_draw_enabled = false;

    void clearContactPoints() {
        narrow_phase.flipBuffers(tick_id);
    }
    bool _isTransformDirty(Collider* collider) const;
    void _setColliderTransformDirty(Collider* collider);
    void _addColliderToDirtyTransformArray(Collider* collider);
    void _removeColliderFromDirtyTransformArray(Collider* collider);

    void _broadphase();
    void _broadphase_Naive();

    void _transformContactPoints();
    void _transformContactPoints2();
    void _adjustPenetrations(float dt);
    void _adjustPenetrationsOld();
    // Physics experiments
    void _solveImpulses(float dt);
    void _preStepContact(Collider* bodyA, Collider* bodyB, ContactPoint& cp, float dt);
    void _solveManifoldImpulseIteration(CollisionManifold& m);
    void _solveManifoldFrictionIteration(CollisionManifold& m);
    void _solveContactIteration(Collider* bodyA, Collider* bodyB, ContactPoint& cp);
    void _applyImpulse(Collider* body, const gfxm::vec3 linJ, const gfxm::vec3& angJ, float invMass, const gfxm::mat3& invInertiaWorld);

public:
    gfxm::vec3 gravity = gfxm::vec3(.0f, -9.8f, .0f);

    void addCollider(Collider* collider);
    void removeCollider(Collider* collider);
    void markAsExternallyTransformed(Collider* collider);

    RayCastResult rayTest(const gfxm::vec3& from, const gfxm::vec3& to, uint64_t mask = COLLISION_MASK_EVERYTHING);
    SphereSweepResult sphereSweep(const gfxm::vec3& from, const gfxm::vec3& to, float radius, uint64_t mask = COLLISION_MASK_EVERYTHING);
    void sphereTest(const gfxm::mat4& tr, float radius);

    void debugDraw();

    void update(float dt, float time_step, int max_steps);
    void update_variableDt(float dt);
    void updateInternal(float dt);

    int dirtyTransformCount() const { return dirty_transform_count; }
    const Collider* const* getDirtyTransformArray() const { return dirty_transform_array.data(); }
    void clearDirtyTransformArray() { dirty_transform_count = 0; }

    // 
    int addContactPoint(
        const gfxm::vec3& pos_a, const gfxm::vec3& pos_b, 
        const gfxm::vec3& normal_a, const gfxm::vec3& normal_b,
        float depth
    );
    int addContactPoint(const ContactPoint& cp);
    ContactPoint* getContactPointArrayEnd();

    void enableDbgDraw(bool enable) { dbg_draw_enabled = enable; }
};
