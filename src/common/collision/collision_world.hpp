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

constexpr int MAX_CONTACT_POINTS = 4096;
constexpr int MAX_MANIFOLDS = 4096;

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

inline uint16_t makeNormalKey(const gfxm::vec3& N) {
    float phi = atan2f(N.z, N.x);
    float theta = acosf(gfxm::clamp(N.y, -1.f, 1.f));
    if(phi < .0f) phi += 2.0f * gfxm::pi;
    int t = int(theta / gfxm::pi * 31.f + .5f);
    int p = int(phi / (2.0f * gfxm::pi) * 63.f + .5f);
    return t + (p << 8);
};

// 0xBBBBBB AAAAAA NNNN
typedef uint64_t manifold_key_t;
#define MAKE_MANIFOLD_KEY(A, B, NORMAL_KEY) \
    manifold_key_t(uint64_t(NORMAL_KEY) | uint64_t(std::min(A, B)) << 16 | (uint64_t(std::max(A, B)) << 40))


template<typename T, int CAPACITY>
struct CollisionArray {
    T _data[CAPACITY];
    int _count = 0;

    CollisionArray() {
        memset(&_data[0], 0, CAPACITY * sizeof(T));
    }
    CollisionArray(const std::initializer_list<T>& list) {
        for (auto& e : list) {
            push_back(e);
        }
    }

    constexpr int capacity() const { return CAPACITY; }

    void clear() {
        _count = 0;
    }
    int count() const {
        return _count;
    }
    T* data() {
        return _data;
    }
    void push_back(const T& item) {
        assert(_count < CAPACITY);
        _data[_count++] = item;
    }
    void erase(int at) {
        assert(_count > 0);
        assert(at < _count && at >= 0);
        _data[at] = data[_count - 1];
        --_count;
    }

    T& operator[](int i) {
        return _data[i];
    }
    const T& operator[](int i) const {
        return _data[i];
    }
};

struct NarrowPhaseData {
    std::unordered_map<manifold_key_t, int> pair_manifold_table;
    CollisionArray<CollisionManifold, MAX_MANIFOLDS> manifolds;
    CollisionArray<ContactPoint, MAX_CONTACT_POINTS> contact_points;
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
        front->contact_points.clear();/*
        for (int i = 0; i < back->manifolds.count(); ++i) {
            back->manifolds[i].old_points = back->manifolds[i].points;
            back->manifolds[i].old_point_count = back->manifolds[i].point_count;
        }*/
    }

    CollisionManifold* createManifold(Collider* a, Collider* b, const gfxm::vec3& N, bool use_cached = true) {
        return createManifold(a, b, makeNormalKey(N), use_cached);
    }
    CollisionManifold* createManifold(Collider* a, Collider* b, uint16_t normal_key, bool use_cached = true) {
        manifold_key_t key = MAKE_MANIFOLD_KEY(a->id, b->id, normal_key);
        
        CollisionManifold* manifold = createNewManifold(key);
        manifold->points = &front->contact_points[front->contact_points.count()];
        manifold->collider_a = a;
        manifold->collider_b = b;
        manifold->point_count = 0;
        manifold->old_points = 0;
        manifold->old_point_count = 0;
        if(use_cached) {
            CollisionManifold* manifold_cached = findCachedManifold(key);
            if(manifold_cached) {
                manifold->old_points = manifold_cached->points;
                manifold->old_point_count = manifold_cached->point_count;
                for (int i = 0; i < manifold_cached->preserved_point_count; ++i) {
                    front->contact_points.push_back(manifold->old_points[i]);
                }
                manifold->point_count = manifold_cached->preserved_point_count;
            }
        }
        return manifold;
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
    }

    void rebuildManifold(CollisionManifold* m) {
        if (m->point_count == 0) {
            return;
        }
        primeManifold(m, m->points[0]);
        for (int i = 1; i < m->point_count; ++i) {
            auto& cp = m->points[i];
            auto project = [](const gfxm::vec3& t1, const gfxm::vec3& t2, const gfxm::vec3& p)->gfxm::vec2{
                return gfxm::vec2(gfxm::dot(t1, p), gfxm::dot(t2, p));
            };
            gfxm::vec2 p2 = project(m->t1, m->t2, cp.point_a);
            if(p2.x < m->extremes.min.x) { m->iminx = i; }
            if(p2.y < m->extremes.min.y) { m->iminy = i; }
            if(p2.x < m->extremes.max.x) { m->imaxx = i; }
            if(p2.y < m->extremes.max.y) { m->imaxy = i; }
            gfxm::expand(m->extremes, p2);
        }
    }

    void addContact2(CollisionManifold* m, ContactPoint& cp) {
        auto project = [](const gfxm::vec3& t1, const gfxm::vec3& t2, const gfxm::vec3& p)->gfxm::vec2{
            return gfxm::vec2(gfxm::dot(t1, p), gfxm::dot(t2, p));
        };
        gfxm::vec2 p2 = project(m->t1, m->t2, cp.point_a);
        if (m->point_count < 4) {
            if(p2.x < m->extremes.min.x) { m->iminx = m->point_count; }
            if(p2.y < m->extremes.min.y) { m->iminy = m->point_count; }
            if(p2.x < m->extremes.max.x) { m->imaxx = m->point_count; }
            if(p2.y < m->extremes.max.y) { m->imaxy = m->point_count; }
            gfxm::expand(m->extremes, p2);
            front->contact_points.push_back(cp);
            ++m->point_count;
        }
        if(p2.x <= m->extremes.min.x) {
            m->points[m->iminx] = cp;
            gfxm::expand(m->extremes, p2);
        }
        if(p2.y <= m->extremes.min.y) {
            m->points[m->iminy] = cp;
            gfxm::expand(m->extremes, p2);
        }
        if(p2.x <= m->extremes.max.x) {
            m->points[m->imaxx] = cp;
            gfxm::expand(m->extremes, p2);
        }
        if(p2.y <= m->extremes.max.y) {
            m->points[m->imaxy] = cp;
            gfxm::expand(m->extremes, p2);
        }
    }

    void addContact(CollisionManifold* manifold, ContactPoint& cp) {
        if (!cp.normal_a.is_valid()) {
            //return;
        }

        if (manifold->point_count == 0) {
            primeManifold(manifold, cp);
        }

        cp.lcl_point_a = gfxm::inverse(manifold->collider_a->getTransform()) * gfxm::vec4(cp.point_a, 1.f);
        cp.lcl_point_b = gfxm::inverse(manifold->collider_b->getTransform()) * gfxm::vec4(cp.point_b, 1.f);
        cp.lcl_normal = gfxm::inverse(gfxm::to_mat3(manifold->collider_a->getRotation())) * cp.normal_a;

        cp.tick_id = tick_id;

        addContact2(manifold, cp);

        /*
        constexpr float EPS_POSITION = 1e-2f;
        constexpr float EPS_NORMAL = .02f;
        for (int i = 0; i < manifold->preserved_point_count; ++i) {
            ContactPoint& oldcp = manifold->points[i];
            const gfxm::vec3& A = gfxm::lerp(oldcp.point_a, oldcp.point_b, .5f);
            const gfxm::vec3& B = gfxm::lerp(cp.point_a, cp.point_b, .5f);
            if (gfxm::length(B - A) > EPS_POSITION) {
                continue;
            }
            if (1.0f - gfxm::dot(oldcp.normal_a, cp.normal_a) > EPS_NORMAL) {
                continue;
            }

            return;
        }

        if (manifold->point_count >= 8) {
            return;
        }

        cp.lcl_point_a = gfxm::inverse(manifold->collider_a->getTransform()) * gfxm::vec4(cp.point_a, 1.f);
        cp.lcl_point_b = gfxm::inverse(manifold->collider_b->getTransform()) * gfxm::vec4(cp.point_b, 1.f);
        cp.lcl_normal = gfxm::inverse(gfxm::to_mat3(manifold->collider_a->getRotation())) * cp.normal_a;

        cp.tick_id = tick_id;
        front->contact_points.push_back(cp);
        ++manifold->point_count;*/
    }
    /*
    void addContact(CollisionManifold* manifold, ContactPoint& cp) {
        if (!cp.normal_a.is_valid()) {
            //return;
        }

        if (manifold->point_count >= 8) {
            return;
        }

        constexpr float EPS_POSITION = 1e-2f;
        constexpr float EPS_NORMAL = .02f;
        for (int i = 0; i < manifold->old_point_count; ++i) {
            const ContactPoint& oldcp = manifold->old_points[i];
            const gfxm::vec3& A = gfxm::lerp(oldcp.point_a, oldcp.point_b, .5f);
            const gfxm::vec3& B = gfxm::lerp(cp.point_a, cp.point_b, .5f);
            if (gfxm::length(B - A) > EPS_POSITION) {
                continue;
            }
            if (1.0f - gfxm::dot(oldcp.normal_a, cp.normal_a) > EPS_NORMAL) {
                continue;
            }

            cp.lcl_point_a = gfxm::inverse(manifold->collider_a->getTransform()) * gfxm::vec4(cp.point_a, 1.f);
            cp.lcl_point_b = gfxm::inverse(manifold->collider_b->getTransform()) * gfxm::vec4(cp.point_b, 1.f);
            cp.lcl_normal = gfxm::inverse(manifold->collider_a->getTransform()) * gfxm::vec4(cp.normal_a, .0f);
            cp.jn_acc   = oldcp.jn_acc;
            cp.jt_acc   = oldcp.jt_acc;
            cp.mass_normal  = oldcp.mass_normal;
            cp.mass_tangent1 = oldcp.mass_tangent1;
            cp.mass_tangent2 = oldcp.mass_tangent2;
            cp.t1       = oldcp.t1;
            cp.t2       = oldcp.t2;
            cp.vn0      = oldcp.vn0;
            cp.bounceApplied = oldcp.bounceApplied;
            cp.tick_id = oldcp.tick_id;

            front->contact_points.push_back(cp);
            ++manifold->point_count;
            return;
        }

        cp.lcl_point_a = gfxm::inverse(manifold->collider_a->getTransform()) * gfxm::vec4(cp.point_a, 1.f);
        cp.lcl_point_b = gfxm::inverse(manifold->collider_b->getTransform()) * gfxm::vec4(cp.point_b, 1.f);
        cp.lcl_normal = gfxm::inverse(manifold->collider_a->getTransform()) * gfxm::vec4(cp.normal_a, .0f);

        cp.tick_id = tick_id;
        front->contact_points.push_back(cp);
        ++manifold->point_count;
    }*/
    void removeContact(CollisionManifold* manifold, int at) {
        // TODO:
        LOG_ERR("removeContact() not implemented");
    }

};

class CollisionWorld {
    std::vector<Collider*> colliders;
    std::vector<ColliderProbe*> probes;
    std::unordered_map<COLLISION_PAIR_TYPE, std::vector<std::pair<Collider*, Collider*>>> potential_pairs;
    /*std::vector<CollisionManifold> manifolds;
    ContactPoint contact_points[MAX_CONTACT_POINTS];
    int          contact_point_count = 0;*/
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
        // Not clearing contact points like this anymore
        //contact_point_count = 0;
    }
    bool _isTransformDirty(Collider* collider) const;
    void _setColliderTransformDirty(Collider* collider);
    void _addColliderToDirtyTransformArray(Collider* collider);
    void _removeColliderFromDirtyTransformArray(Collider* collider);

    void _broadphase();
    void _broadphase_Naive();

    void _transformContactPoints();
    void _transformContactPoints2();
    void _adjustPenetrations();
    void _adjustPenetrationsOld();
    // Physics experiments
    void _preStepContact2(Collider* bodyA, Collider* bodyB, ContactPoint& cp, float dt);
    void _applyCollisionIteration(Collider* bodyA, Collider* bodyB, ContactPoint& cp, float restitution, float dt);
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
