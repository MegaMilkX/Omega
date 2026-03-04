#pragma once

#include <assert.h>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include "math/gfxm.hpp"
#include "collision/common.hpp"
#include "collision/narrow_phase.hpp"
#include "collision/intersection/box_box.hpp"
#include "collision/intersection/sphere_box.hpp"
#include "collision/intersection/sphere_capsule.hpp"
#include "collision/intersection/capsule_capsule.hpp"
#include "collision/intersection/capsule_triangle.hpp"
#include "collision/intersection/gjkepa/epa_types.hpp"
#include "collision/shape/empty.hpp"

#include "collision/collider.hpp"

#include "debug_draw/debug_draw.hpp"

#include "aabb_tree/aabb_tree.hpp"

#define PHY_ENABLE_ACCUMULATION 1
#define PHY_ENABLE_POSITION_CORRECTION 1
#define PHY_ENABLE_POSITION_CORRECTION_BLUNT 0
#define PHY_ENABLE_WARM_STARTING 1
#define PHY_ENABLE_FRICTION 1

#define COLLISION_DBG_DRAW_AABB_TREE 0
#define COLLISION_DBG_DRAW_COLLIDERS 1
#define COLLISION_DBG_DRAW_CONTACT_POINTS 1
#define COLLISION_DBG_DRAW_CONSTRAINTS 1
#define COLLISION_DBG_DRAW_TESTS 1

struct phyRayCastResult {
    gfxm::vec3 position;
    gfxm::vec3 normal;
    phyRigidBody* collider = 0;
    phySurfaceProp prop;
    float distance = .0f;
    bool hasHit = false;
};
struct phySphereSweepResult {
    gfxm::vec3 contact;
    gfxm::vec3 normal;
    gfxm::vec3 sphere_pos;
    phyRigidBody* collider = 0;
    phySurfaceProp prop;
    float distance = .0f;
    bool hasHit = false;
};

class phyJoint {
public:
    phyJoint(phyRigidBody* a, phyRigidBody* b, const gfxm::vec3& joint_pt, const gfxm::mat3& joint_basis)
    : body_a(a), body_b(b) {
        gfxm::vec3 pos_a = gfxm::vec3(0, 0, 0);
        gfxm::vec3 pos_b = gfxm::vec3(0, 0, 0);
        gfxm::quat rot_a = gfxm::quat(0, 0, 0, 1);
        gfxm::quat rot_b = gfxm::quat(0, 0, 0, 1);
        if (a) {
            pos_a = a->getPosition();
            rot_a = a->getRotation();
        }
        if (b) {
            pos_b = b->getPosition();
            rot_b = b->getRotation();
        }
        lcl_anchor_a = gfxm::translate(gfxm::mat4(1.f), -pos_a) * gfxm::to_mat4(gfxm::inverse(rot_a)) * gfxm::vec4(joint_pt, 1.f);
        lcl_basis_a = gfxm::to_mat3(gfxm::inverse(rot_a)) * joint_basis;
        lcl_anchor_b = gfxm::translate(gfxm::mat4(1.f), -pos_b) * gfxm::to_mat4(gfxm::inverse(rot_b)) * gfxm::vec4(joint_pt, 1.f);
        lcl_basis_b = gfxm::to_mat3(gfxm::inverse(rot_b)) * joint_basis;
        qRel0 = gfxm::inverse(rot_a) * rot_b;

        rA = gfxm::to_mat3(rot_a) * lcl_anchor_a;
        rB = gfxm::to_mat3(rot_b) * lcl_anchor_b;
    }
    phyRigidBody* body_a = nullptr;
    phyRigidBody* body_b = nullptr;
    gfxm::vec3 lcl_anchor_a;
    gfxm::vec3 lcl_anchor_b;
    gfxm::mat3 lcl_basis_a;
    gfxm::mat3 lcl_basis_b;
    gfxm::quat qRel0;
    gfxm::vec3 angularBias;

    gfxm::mat3 Meff; // Effective mass
    gfxm::vec3 rA;
    gfxm::vec3 rB;
    gfxm::vec3 lin_bias;
    gfxm::vec3 bias;
    gfxm::vec3 J_acc; // Accumulated impulse
    gfxm::vec3 Jw_acc;
    gfxm::mat3 Meff_ang;

    gfxm::mat3 world_basis;

    gfxm::vec3 linear_mask = gfxm::vec3(1, 1, 1);

    float bias_factor = .2f;
    float softness = .0f;
};

class phyWorld {
    std::vector<phyRigidBody*> colliders;
    std::vector<phyJoint*> joints;
    std::vector<phyProbe*> probes;
    std::unordered_map<PHY_PAIR_TYPE, std::vector<std::pair<phyRigidBody*, phyRigidBody*>>> potential_pairs;
    phyNarrowPhase narrow_phase;

    phyEmptyShape empty_shape;
    phyRigidBody world_body;

    float dt_accum = .0f;
    int tick_id = 0;

    EPA_Context epa_ctx;

    AabbTree aabb_tree;

    std::vector<phyRigidBody*> dirty_transform_array;
    int dirty_transform_count = 0;

    bool dbg_draw_enabled = false;

    void clearContactPoints() {
        narrow_phase.flipBuffers(tick_id);
    }
    bool _isTransformDirty(phyRigidBody* collider) const;
    void _setColliderTransformDirty(phyRigidBody* collider);
    void _addColliderToDirtyTransformArray(phyRigidBody* collider);
    void _removeColliderFromDirtyTransformArray(phyRigidBody* collider);

    void _broadphase();
    void _broadphase_Naive();

    void _transformContactPoints();
    void _transformContactPoints2();
    void _adjustPenetrations(float dt);
    void _adjustPenetrationsOld();
    // Physics experiments
    void _solveImpulses(float dt);
    void _preStepContact(phyRigidBody* bodyA, phyRigidBody* bodyB, phyContactPoint& cp, float inv_dt);
    void _solveManifoldImpulseIteration(phyManifold& m);
    void _solveManifoldFrictionIteration(phyManifold& m);
    void _solveContactIteration(phyRigidBody* bodyA, phyRigidBody* bodyB, phyContactPoint& cp);
    void _solveContactFrictionIteration(phyRigidBody* bodyA, phyRigidBody* bodyB, phyContactPoint& cp);
    
    void _preStepJoint(phyJoint* joint, float dt, float inv_dt);
    void _solveJoint(phyJoint* joint);
    gfxm::vec3 _solveLinearConstraint(phyJoint* joint, const gfxm::vec3& axis, const gfxm::vec3& dV, float bias);
    
    void _applyImpulse(phyRigidBody* body, const gfxm::vec3 linJ, const gfxm::vec3& angJ, float invMass, const gfxm::mat3& invInertiaWorld);

public:
    gfxm::vec3 gravity = gfxm::vec3(.0f, -9.8f, .0f);

    phyWorld();
    phyWorld(const phyWorld&) = delete;
    phyWorld& operator=(const phyWorld&) = delete;

    void addCollider(phyRigidBody* collider);
    void removeCollider(phyRigidBody* collider);
    void markAsExternallyTransformed(phyRigidBody* collider);

    void addJoint(phyJoint* joint);
    void removeJoint(phyJoint* joint);

    phyRayCastResult rayTest(const gfxm::vec3& from, const gfxm::vec3& to, uint64_t mask = COLLISION_MASK_EVERYTHING);
    phySphereSweepResult sphereSweep(const gfxm::vec3& from, const gfxm::vec3& to, float radius, uint64_t mask = COLLISION_MASK_EVERYTHING);
    void sphereTest(const gfxm::mat4& tr, float radius);

    void debugDraw();

    void update(float dt, float time_step, int max_steps);
    void update_variableDt(float dt);
    void updateInternal(float dt);

    int dirtyTransformCount() const { return dirty_transform_count; }
    const phyRigidBody* const* getDirtyTransformArray() const { return dirty_transform_array.data(); }
    void clearDirtyTransformArray() { dirty_transform_count = 0; }

    void enableDbgDraw(bool enable) { dbg_draw_enabled = enable; }
};
