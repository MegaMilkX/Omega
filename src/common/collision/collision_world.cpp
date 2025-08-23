#include "collision_world.hpp"

#include <algorithm>
#include <format>

#include "collision_util.hpp"
#include "engine.hpp"
#include "util/timer.hpp"

#include "intersection/gjkepa/gjkepa.hpp"

template<>
class GJKEPA_SupportGetter<CollisionTriangleMeshShape> {
    gfxm::mat4 transform;
    const CollisionTriangleMeshShape* shape;
    int triangle_idx;
public:
    GJKEPA_SupportGetter(
        const gfxm::mat4& transform,
        const CollisionTriangleMeshShape* shape,
        int triangle_idx
    ) 
        : transform(transform),
        shape(shape),
        triangle_idx(triangle_idx)
    {}

    gfxm::vec3 getPosition() const { return transform[3]; }

    gfxm::vec3 operator()(const gfxm::vec3& dir) const {
        return shape->getMinkowskiSupportPoint(dir, transform, triangle_idx);
    }
};

template<COLLISION_SHAPE_TYPE SHAPE_TYPE>
struct SHAPE_TYPE_HELPER {};

#define DEF_COLLISION_SHAPE_HELPER(ENUM, TYPE) \
    template<> \
    struct SHAPE_TYPE_HELPER<ENUM> { \
        typedef TYPE SHAPE_T; \
    };

DEF_COLLISION_SHAPE_HELPER(COLLISION_SHAPE_TYPE::SPHERE , CollisionSphereShape);
DEF_COLLISION_SHAPE_HELPER(COLLISION_SHAPE_TYPE::BOX , CollisionBoxShape);
DEF_COLLISION_SHAPE_HELPER(COLLISION_SHAPE_TYPE::CAPSULE , CollisionCapsuleShape);
DEF_COLLISION_SHAPE_HELPER(COLLISION_SHAPE_TYPE::TRIANGLE_MESH, CollisionTriangleMeshShape);
DEF_COLLISION_SHAPE_HELPER(COLLISION_SHAPE_TYPE::CONVEX_MESH, CollisionConvexShape);

template<
    COLLISION_SHAPE_TYPE SHAPE_TYPE_A,
    COLLISION_SHAPE_TYPE SHAPE_TYPE_B
>
void foo() {
    typedef typename SHAPE_TYPE_HELPER<SHAPE_TYPE_A>::SHAPE_T shape_a_t;
    typedef typename SHAPE_TYPE_HELPER<SHAPE_TYPE_B>::SHAPE_T shape_b_t;
    /*
    auto& pairs = potential_pairs[(COLLISION_PAIR_TYPE)(SHAPE_TYPE_A | SHAPE_TYPE_B)];

    Collider* collider_a = pairs[i].first;
    Collider* collider_b = pairs[i].second;

    auto shape_a = (const shape_a_t*)a->getShape();
    auto shape_b = (const shape_b_t*)b->getShape();
    const gfxm::mat4 transform_a = a->getShapeTransform();
    const gfxm::mat4 transform_b = b->getShapeTransform();
    */

}


void bar() {
    foo<COLLISION_SHAPE_TYPE::BOX, COLLISION_SHAPE_TYPE::TRIANGLE_MESH>();


}


inline static bool aabbOverlap(const gfxm::aabb &a, const gfxm::aabb &b) {
    float ox = gfxm::_min(a.to.x, b.to.x) - gfxm::_max(a.from.x, b.from.x);
    float oy = gfxm::_min(a.to.y, b.to.y) - gfxm::_max(a.from.y, b.from.y);
    float oz = gfxm::_min(a.to.z, b.to.z) - gfxm::_max(a.from.z, b.from.z);
    return !(ox < 0.0f || oy < 0.0f || oz < 0.0f);
}


bool CollisionWorld::_isTransformDirty(Collider* c) const {
    return c->dirty_transform_index < dirty_transform_count;
}
void CollisionWorld::_setColliderTransformDirty(Collider* collider) {
    auto cur_idx = collider->dirty_transform_index;
    if (cur_idx < dirty_transform_count) {
        return;
    }
    auto tgt_idx = dirty_transform_count;
    auto other = dirty_transform_array[tgt_idx];
    dirty_transform_array[tgt_idx] = collider;
    dirty_transform_array[cur_idx] = other;
    collider->dirty_transform_index = tgt_idx;
    other->dirty_transform_index = cur_idx;
    ++dirty_transform_count;
}
void CollisionWorld::_addColliderToDirtyTransformArray(Collider* collider) {
    collider->dirty_transform_index = dirty_transform_array.size();
    dirty_transform_array.push_back(collider);
}
void CollisionWorld::_removeColliderFromDirtyTransformArray(Collider* collider) {
    auto idx = collider->dirty_transform_index;
    auto last_idx = dirty_transform_array.size() - 1;
    assert(last_idx >= 0);
    auto tmp = dirty_transform_array[last_idx];
    dirty_transform_array[idx] = tmp;    
    dirty_transform_array.resize(dirty_transform_array.size() - 1);
    tmp->dirty_transform_index = idx;
    collider->dirty_transform_index = -1;
}

void CollisionWorld::_broadphase() {    
    // Determine potential collisions
    for (int i = 0; i < dirty_transform_count; ++i) {
        auto a = dirty_transform_array[i];

        aabb_tree.forEachOverlap(a->world_aabb, [this, &a](Collider* b) {
            bool layer_test
                = (a->collision_group & b->collision_mask)
                && (b->collision_group & a->collision_mask);
            if (!layer_test) {
                return;
            }
            if(_isTransformDirty(b) && a->id >= b->id) {
                return;
            }/*
            if(a == b) {
                return;
            }*/

            COLLISION_PAIR_TYPE pair_identifier = (COLLISION_PAIR_TYPE)((int)a->getShape()->getShapeType() | (int)b->getShape()->getShapeType());
            if ((int)a->getShape()->getShapeType() <= (int)b->getShape()->getShapeType()) {
                potential_pairs[pair_identifier].push_back(std::make_pair(a, b));
            } else {
                potential_pairs[pair_identifier].push_back(std::make_pair(b, a));
            }
        });
    }
}

void CollisionWorld::_broadphase_Naive() {
    // Determine potential collisions
    // everything vs everything
    for (int i = 0; i < colliders.size(); ++i) {
        for (int j = i + 1; j < colliders.size(); ++j) {
            auto a = colliders[i];
            auto b = colliders[j];
        
            for (int j = 0; j < colliders.size(); ++j) {
                auto b = colliders[j];
                if(a == b) {
                    continue;
                }
                bool layer_test
                    = (a->collision_group & b->collision_mask)
                    && (b->collision_group & a->collision_mask);
                if (!layer_test) {
                    continue;
                }
            
                if (!aabbOverlap(a->world_aabb, b->world_aabb)) {
                    continue;
                }

                COLLISION_PAIR_TYPE pair_identifier = (COLLISION_PAIR_TYPE)((int)a->getShape()->getShapeType() | (int)b->getShape()->getShapeType());
                if ((int)a->getShape()->getShapeType() <= (int)b->getShape()->getShapeType()) {
                    potential_pairs[pair_identifier].push_back(std::make_pair(a, b));
                } else {
                    potential_pairs[pair_identifier].push_back(std::make_pair(b, a));
                }
            }
        }
    }
}

void CollisionWorld::_transformContactPoints() {    
    auto shouldProcess = [](const CollisionManifold& m) -> bool {
        if (m.point_count == 0) {
            assert(false);
            return false;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            return false;
        }
        if (m.collider_a->getType() == COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == COLLIDER_TYPE::PROBE) {
            return false;
        }
        if (m.collider_a->getFlags() & COLLIDER_STATIC
            && m.collider_b->getFlags() & COLLIDER_STATIC) {
            return false;
        }
        return true;
    };
    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        if(!shouldProcess(m)) {
            continue;
        }

        for (int j = 0; j < m.point_count; ++j) {
            ContactPoint* pt = &m.points[j];

            gfxm::mat4 M4A = m.collider_a->getTransform();
            gfxm::mat4 M4B = m.collider_b->getTransform();
            gfxm::vec3 A = M4A * gfxm::vec4(pt->lcl_point_a, 1.f);
            gfxm::vec3 B = M4B * gfxm::vec4(pt->lcl_point_b, 1.f);
            gfxm::vec3 diff = B - A;
            float depth = gfxm::_max(.0f, gfxm::dot(-diff, pt->normal_a));
            if(!isnan(depth)) {
                pt->point_a = A;
                pt->point_b = B;
                pt->depth = depth;
            }
        }
    }
}

void CollisionWorld::_transformContactPoints2() {     
    auto shouldProcess = [](const CollisionManifold& m) -> bool {
        if (m.point_count == 0) {
            assert(false);
            return false;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            return false;
        }
        if (m.collider_a->getType() == COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == COLLIDER_TYPE::PROBE) {
            return false;
        }
        if (m.collider_a->getFlags() & COLLIDER_STATIC
            && m.collider_b->getFlags() & COLLIDER_STATIC) {
            return false;
        }
        return true;
    };
    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        if(!shouldProcess(m)) {
            continue;
        }

        for (int j = 0; j < m.point_count; ++j) {
            ContactPoint* pt = &m.points[j];
            if(pt->depth < FLT_EPSILON) {
                continue;
            }

            gfxm::vec3 dA = m.collider_a->position - m.collider_a->prev_pos;
            gfxm::vec3 dB = m.collider_b->position - m.collider_b->prev_pos;
            float offs = gfxm::dot(dB - dA, pt->normal_a);
            // TODO: CHECK
            pt->depth += offs;
        }
    }
}

void CollisionWorld::_adjustPenetrations() {
    constexpr int ITERATION_COUNT = 8;
    constexpr float CORRECTION_FRACTION = 1.f / float(ITERATION_COUNT);
    const float SLACK = .001f;

    auto shouldProcess = [](const CollisionManifold& m) -> bool {
        if (m.point_count == 0) {
            assert(false);
            return false;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            return false;
        }
        if (m.collider_a->getType() == COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == COLLIDER_TYPE::PROBE) {
            return false;
        }
        if (m.collider_a->getFlags() & COLLIDER_STATIC
            && m.collider_b->getFlags() & COLLIDER_STATIC) {
            return false;
        }
        return true;
    };

    for(int iter = 0; iter < ITERATION_COUNT; ++iter) {
        for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
            auto& m = narrow_phase.getManifold(i);
            if(!shouldProcess(m)) {
                continue;
            }

            float weight_a = 1.f;
            float weight_b = 1.f;
            if (m.collider_a->getFlags() & COLLIDER_STATIC) {
                weight_a = .0f;
            }
            if (m.collider_b->getFlags() & COLLIDER_STATIC) {
                weight_b = .0f;
            }
            weight_a = weight_a / (weight_a + weight_b);
            weight_b = weight_b / (weight_a + weight_b);

            gfxm::vec3 delta_a;
            gfxm::vec3 delta_b;
            std::sort(m.points, m.points + m.point_count, [](const ContactPoint& a, const ContactPoint& b)->bool {
                if (a.type != b.type) {
                    return (int)a.type < (int)b.type;
                }
                return a.depth > b.depth;
            });
            for (int j = 0; j < m.point_count; ++j) {
                auto pt = &m.points[j];

                if (isnan(pt->depth)) {
                    assert(false);
                    continue;
                }
                if (!pt->normal_a.is_valid()) {
                    //assert(false);
                    continue;
                }
                if (!pt->normal_b.is_valid()) {
                    //assert(false);
                    continue;
                }

                gfxm::vec3 Na = pt->normal_a;
                gfxm::vec3 Nb = pt->normal_b;
                //float depth_a = pt->depth - gfxm::dot(pt->normal_b, delta_a);
                //float depth_b = pt->depth - gfxm::dot(pt->normal_a, delta_b);
                float depth_a = gfxm::_max(.0f, pt->depth - SLACK);
                float depth_b = gfxm::_max(.0f, pt->depth - SLACK);

                if (depth_a < .0f && depth_b < .0f) {
                    continue;
                }
                float correction_a = gfxm::_max(depth_a, 0.0f);
                delta_a += Nb * depth_a * CORRECTION_FRACTION;
                delta_b += Na * depth_b * CORRECTION_FRACTION;
            }

            if (weight_a > FLT_EPSILON) {
                //m.collider_a->position += delta_a * weight_a;
                m.collider_a->correction_accum += delta_a * weight_a;
                _setColliderTransformDirty(m.collider_a);
            }
            if (weight_b > FLT_EPSILON) {
                //m.collider_b->position += delta_b * weight_b;
                m.collider_b->correction_accum += delta_b * weight_b;
                _setColliderTransformDirty(m.collider_b);
            }
        }

        for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
            auto& m = narrow_phase.getManifold(i);
            if(!shouldProcess(m)) {
                continue;
            }

            for (int j = 0; j < m.point_count; ++j) {
                ContactPoint& cp = m.points[j];
                if (!cp.normal_a.is_valid()) {
                    //assert(false);
                    continue;
                }
                if (!cp.normal_b.is_valid()) {
                    //assert(false);
                    continue;
                }

                cp.depth = cp.depth
                    + dot(m.collider_a->correction_accum, cp.normal_a)
                    - dot(m.collider_b->correction_accum, cp.normal_a);

                if (isnan(cp.depth)) {
                    assert(false);
                }
            }
        }
    }


    for (int i = 0; i < this->dirty_transform_count; ++i) {
        Collider* c = dirty_transform_array[i];
        c->position += c->correction_accum;
        c->correction_accum = gfxm::vec3(0,0,0);
    }
}

void CollisionWorld::_adjustPenetrationsOld() {
    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        if (m.point_count == 0) {
            assert(false);
            continue;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            continue;
        }
        if (m.collider_a->getType() == COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == COLLIDER_TYPE::PROBE) {
            continue;
        }
        if (m.collider_a->getFlags() & COLLIDER_STATIC
            && m.collider_b->getFlags() & COLLIDER_STATIC) {
            continue;
        }
        float weight_a = 1.f, weight_b = 1.f;
        if (m.collider_a->getFlags() & COLLIDER_STATIC) {
            weight_a = .0f;
        }
        if (m.collider_b->getFlags() & COLLIDER_STATIC) {
            weight_b = .0f;
        }
        weight_a = weight_a / (weight_a + weight_b);
        weight_b = weight_b / (weight_a + weight_b);

        gfxm::vec3 delta_a;
        gfxm::vec3 delta_b;
        std::sort(m.points, m.points + m.point_count, [](const ContactPoint& a, const ContactPoint& b)->bool {
            if (a.type != b.type) {
                return (int)a.type < (int)b.type;
            }
            return a.depth > b.depth;
        });
        for (int j = 0; j < m.point_count; ++j) {
            auto pt = &m.points[j];
            
            if (isnan(pt->depth)) {
                assert(false);
                continue;
            }
            if (!pt->normal_a.is_valid()) {
                //assert(false);
                continue;
            }
            if (!pt->normal_b.is_valid()) {
                //assert(false);
                continue;
            }

            gfxm::vec3 Nb = pt->normal_b;
            gfxm::vec3 Na = pt->normal_a;
            float depth_a = pt->depth - gfxm::dot(pt->normal_b, delta_a);
            float depth_b = pt->depth - gfxm::dot(pt->normal_a, delta_b);

            if (depth_a < .0f && depth_b < .0f) {
                continue;
            }

            delta_a += Nb * depth_a;
            delta_b += Na * depth_b;
        }

        if (weight_a > FLT_EPSILON) {
            m.collider_a->position += delta_a * weight_a;
            _setColliderTransformDirty(m.collider_a);
        }
        if (weight_b > FLT_EPSILON) {
            m.collider_b->position += delta_b * weight_b;
            _setColliderTransformDirty(m.collider_b);
        }
    }
}

void CollisionWorld::_preStepContact2(Collider* bodyA, Collider* bodyB, ContactPoint& cp, float dt) {
    const float ALLOWED_PENETRATION = .01f;
    const float BIAS_FACTOR = .2f;
    const float inv_dt = dt > .0f ? (1.f / dt) : .0f;

    gfxm::vec3 N = gfxm::normalize(cp.normal_a);
    gfxm::vec3 contactPoint = gfxm::lerp(cp.point_a, cp.point_b, .5f);
    gfxm::vec3 rA = contactPoint - (bodyA->position + bodyA->center_offset);
    gfxm::vec3 rB = contactPoint - (bodyB->position + bodyB->center_offset);
    gfxm::vec3 rAxn = gfxm::cross(rA, N);
    gfxm::vec3 rBxn = gfxm::cross(rB, N);

    float invMassA = bodyA->getInverseMass();
    float invMassB = bodyB->getInverseMass();
    float invMassSum = invMassA + invMassB;
    gfxm::mat3 invInertiaA = bodyA->getInverseWorldInertiaTensor();
    gfxm::mat3 invInertiaB = bodyB->getInverseWorldInertiaTensor();

    // Precompute normal mass, tangent mass and bias
    float rn1 = gfxm::dot(rA, N);
    float rn2 = gfxm::dot(rB, N);

    // Mass along normal
    //float angularFactor = gfxm::dot(invInertiaA * rAxn, rAxn) + gfxm::dot(invInertiaB * rBxn, rBxn);
    float angularFactor = .0f;
    if(bodyA->mass > .0f) {
        angularFactor += gfxm::dot(gfxm::cross(invInertiaA * rAxn, rA), N);
    }
    if(bodyB->mass > .0f) {
        angularFactor += gfxm::dot(gfxm::cross(invInertiaB * rBxn, rB), N);
    }
    float denom_n = invMassSum + angularFactor;
    cp.mass_normal = denom_n > .0f ? (1.0f / denom_n) : 1.f;

    // Mass along tangents
    if(fabsf(N.x) < 0.57735f) {
        cp.t1 = gfxm::normalize(gfxm::cross(N, gfxm::vec3(1,0,0)));
    } else {
        cp.t1 = gfxm::normalize(gfxm::cross(N, gfxm::vec3(0,1,0)));
    }
    cp.t2 = gfxm::cross(N, cp.t1);

    gfxm::vec3 rAxt1 = gfxm::cross(rA, cp.t1);
    gfxm::vec3 rBxt1 = gfxm::cross(rB, cp.t1);
    float denom_t1 = invMassSum + gfxm::dot(invInertiaA * rAxt1, rAxt1) + gfxm::dot(invInertiaB * rBxt1, rBxt1);
    cp.mass_tangent1 = denom_t1 > .0f ? (1.f / denom_t1) : .0f;

    gfxm::vec3 rAxt2 = gfxm::cross(rA, cp.t2);
    gfxm::vec3 rBxt2 = gfxm::cross(rB, cp.t2);
    float denom_t2 = invMassSum + gfxm::dot(invInertiaA * rAxt2, rAxt2) + gfxm::dot(invInertiaB * rBxt2, rBxt2);
    cp.mass_tangent2 = denom_t2 > .0f ? (1.f / denom_t2) : .0f;

    // Bias
    cp.bias = -BIAS_FACTOR * gfxm::_min(.0f, -cp.depth + ALLOWED_PENETRATION) * inv_dt;    

    // Initial relative normal velocity (for restitution)
    /*
    gfxm::vec3 vA = bodyA->velocity + gfxm::cross(bodyA->angular_velocity, rA);
    gfxm::vec3 vB = bodyB->velocity + gfxm::cross(bodyB->angular_velocity, rB);
    cp.vn0 = gfxm::dot(vB - vA, N);
    */
    // Apply normal + friction impulse
    {
        const gfxm::vec3 impulse_n = cp.normal_a * cp.jn_acc + cp.jt_acc;
        _applyImpulse(bodyA, -impulse_n, -gfxm::cross(rA, impulse_n), invMassA, invInertiaA);
        _applyImpulse(bodyB,  impulse_n,  gfxm::cross(rB, impulse_n), invMassB, invInertiaB);
    }
}

void CollisionWorld::_applyCollisionIteration(Collider* bodyA, Collider* bodyB, ContactPoint& cp, float restitution, float dt) {
    if(bodyA->mass < FLT_EPSILON && bodyB->mass < FLT_EPSILON) {
        return;
    }

    gfxm::vec3 contactPoint = gfxm::lerp(cp.point_a, cp.point_b, 0.5f);
    gfxm::vec3 collisionNormal = gfxm::normalize(cp.normal_a);

    gfxm::vec3 rA = contactPoint - (bodyA->position + bodyA->center_offset);
    gfxm::vec3 rB = contactPoint - (bodyB->position + bodyB->center_offset);

    gfxm::vec3 vA = bodyA->velocity + gfxm::cross(bodyA->angular_velocity, rA);
    gfxm::vec3 vB = bodyB->velocity + gfxm::cross(bodyB->angular_velocity, rB);
    gfxm::vec3 relativeVelocity = vB - vA;
    
    gfxm::mat3 invInertiaA = bodyA->getInverseWorldInertiaTensor();
    gfxm::mat3 invInertiaB = bodyB->getInverseWorldInertiaTensor();

    float invMassA = bodyA->getInverseMass();
    float invMassB = bodyB->getInverseMass();

    //dbgDrawLine(contactPoint, contactPoint + collisionNormal, 0xFF0000FF);
    //dbgDrawLine(contactPoint, contactPoint + relativeVelocity, 0xFF00FF00);

    // ---- NORMAL IMPULSE ----
    float vn = gfxm::dot(relativeVelocity, collisionNormal);
    cp.dbg_vn = vn;

    // Restitution applied only once or based on initial rel speed
#if 0
    float e_use = (!cp.bounceApplied && -cp.vn0 > 1.0f) ? restitution : 0.0f;
    if (fabs(cp.vn0) < 0.5f) e_use = 0.0f;
    if(e_use > 0.0f) cp.bounceApplied = true;
#else
    float e_use = .0f;
#endif

    float jn_delta = (-(vn + cp.bias + e_use * cp.vn0)) * cp.mass_normal;

    // Accumulate and clamp to non-negative
    float jn0 = cp.jn_acc;
    //cp.jn_acc = jn0 + jn_delta;
    cp.jn_acc = gfxm::_max(jn0 + jn_delta, 0.0f);
    jn_delta = cp.jn_acc - jn0;

    // Apply normal impulse
    gfxm::vec3 impulse_n = jn_delta * collisionNormal;// * (1.f / dt) * (1.f / 16.f);
    _applyImpulse(bodyA, -impulse_n, -gfxm::cross(rA, impulse_n), invMassA, invInertiaA);
    _applyImpulse(bodyB,  impulse_n,  gfxm::cross(rB, impulse_n), invMassB, invInertiaB);

    /*
    std::string strout = std::format(
        "vn: {:.3f}, bias: {:.3f}, mass_normal: {:.3f}, jn_delta: {:.3f}, velocity: [{:.3f}, {:.3f}, {:.3f}]", 
        vn, cp.bias, cp.mass_normal, jn_delta, bodyA->velocity.x, bodyA->velocity.y, bodyA->velocity.z
    );
    LOG(strout);*/

    //return; // SKIPPING FRICTION FOR DEBUG

    // ---- FRICTION IMPULSE ----
    // Recalculate relative velocity again
    vA = bodyA->velocity + gfxm::cross(bodyA->angular_velocity, rA);
    vB = bodyB->velocity + gfxm::cross(bodyB->angular_velocity, rB);
    relativeVelocity = vB - vA;

    float vt1 = gfxm::dot(relativeVelocity, cp.t1);
    float vt2 = gfxm::dot(relativeVelocity, cp.t2);

    float jt1_delta = (-vt1) * cp.mass_tangent1;
    float jt2_delta = (-vt2) * cp.mass_tangent2;

    gfxm::vec3 jt_delta_vec = jt1_delta * cp.t1 + jt2_delta * cp.t2;
    gfxm::vec3 jt_new_vec   = cp.jt_acc + jt_delta_vec;

    //float mu = 0.4f; // friction coefficient
    float frictionA = bodyA->friction;
    float frictionB = bodyB->friction;
    float mu = gfxm::sqrt(frictionA * frictionB);

    // Clamp friction
    float jt_max = mu * cp.jn_acc;
    gfxm::vec3 oldTangentImpulse = cp.jt_acc;
    float jt_len = gfxm::length(jt_new_vec);
    if(jt_len > jt_max) {
        jt_new_vec *= jt_max / jt_len;
    }

    jt_delta_vec = jt_new_vec - cp.jt_acc;
    cp.jt_acc    = jt_new_vec;

    // Apply friction impulse
    _applyImpulse(bodyA, -jt_delta_vec, -gfxm::cross(rA, jt_delta_vec), invMassA, invInertiaA);
    _applyImpulse(bodyB,  jt_delta_vec,  gfxm::cross(rB, jt_delta_vec), invMassB, invInertiaB);
}

void CollisionWorld::_applyImpulse(Collider* body, const gfxm::vec3 linJ, const gfxm::vec3& angJ, float invMass, const gfxm::mat3& invInertiaWorld) {
    if(invMass <= .0f) return;
    body->velocity += linJ * invMass;
    body->angular_velocity += invInertiaWorld * angJ;
    assert(body->angular_velocity.is_valid());
    assert(body->velocity.is_valid());
}


void CollisionWorld::addCollider(Collider* collider) {
    static uint32_t next_collider_id = 0;
    
    collider->id = next_collider_id++;

    colliders.push_back(collider);
    if (collider->getType() == COLLIDER_TYPE::PROBE) {
        probes.push_back((ColliderProbe*)collider);
    }

    collider->tree_elem.aabb = collider->getBoundingAabb();
    aabb_tree.add(&collider->tree_elem);

    collider->collision_world = this;

    _addColliderToDirtyTransformArray(collider);
    _setColliderTransformDirty(collider);
    collider->calcInertiaTensor();
}
void CollisionWorld::removeCollider(Collider* collider) {
    _removeColliderFromDirtyTransformArray(collider);

    aabb_tree.remove(&collider->tree_elem);

    collider->collision_world = 0;

    for (int i = 0; i < colliders.size(); ++i) {
        if (colliders[i] == collider) {
            colliders.erase(colliders.begin() + i);
            break;
        }
    }
    if (collider->getType() == COLLIDER_TYPE::PROBE) {
        for (int i = 0; i < probes.size(); ++i) {
            if (probes[i] == (ColliderProbe*)collider) {
                probes.erase(probes.begin() + i);
                break;
            }
        }
    }
}
void CollisionWorld::markAsExternallyTransformed(Collider* collider) {
    _setColliderTransformDirty(collider);
}

struct CastRayContext {
    RayHitPoint rhp;
    Collider* closest_collider = 0;
    uint64_t mask = 0;
    bool hasHit = false;
};
struct CastSphereContext {
    SweepContactPoint scp;
    Collider* closest_collider = 0;
    uint64_t mask = 0;
    bool hasHit = false;
};
static void rayTestCallback(void* context, const gfxm::ray& ray, Collider* cdr) {
    CastRayContext* ctx = (CastRayContext*)context;

    if ((ctx->mask & cdr->collision_group) == 0) {
        return;
    }
    
    const CollisionShape* shape = cdr->getShape();
    if (!shape) {
        assert(false);
        return;
    }

    gfxm::mat4 shape_transform = cdr->getShapeTransform();
    gfxm::vec3 shape_pos = shape_transform * gfxm::vec4(0, 0, 0, 1);

    RayHitPoint rhp;
    bool hasHit = false;
    switch (shape->getShapeType()) {
    case COLLISION_SHAPE_TYPE::SPHERE:
        hasHit = intersectRaySphere(ray, shape_pos, ((const CollisionSphereShape*)shape)->radius, rhp);
        break;
    case COLLISION_SHAPE_TYPE::BOX:
        hasHit = intersectRayBox(ray, shape_transform, ((const CollisionBoxShape*)shape)->half_extents, rhp);
        break;
    case COLLISION_SHAPE_TYPE::CAPSULE:
        hasHit = intersectRayCapsule(
            ray, shape_transform,
            ((const CollisionCapsuleShape*)shape)->height,
            ((const CollisionCapsuleShape*)shape)->radius,
            rhp
        );
        break;
    case COLLISION_SHAPE_TYPE::TRIANGLE_MESH:
        hasHit = intersectRayTriangleMesh(ray, ((const CollisionTriangleMeshShape*)shape)->getMesh(), rhp);
        break;
    };
    if (hasHit) {
        ctx->hasHit = true;
        if (ctx->rhp.distance > rhp.distance) {
            ctx->rhp = rhp;
            ctx->closest_collider = cdr;
        }
    }
}


struct POINT_SUPPORT_TAG {};
struct INFLATED_CONVEX_TAG {};

template<>
class GJKEPA_SupportGetter<POINT_SUPPORT_TAG> {
    gfxm::vec3 point;
public:
    GJKEPA_SupportGetter(const gfxm::vec3& pt)
        : point(pt) {}

    gfxm::vec3 getPosition() const { return point; }

    gfxm::vec3 operator()(const gfxm::vec3& dir) const {
        return point;
    }
};
template<>
class GJKEPA_SupportGetter<INFLATED_CONVEX_TAG> {
    gfxm::mat4 transform;
    const CollisionConvexShape* shape;
    float radius;
public:
    GJKEPA_SupportGetter(const gfxm::mat4& transform, const CollisionConvexShape* shape, float radius)
        : transform(transform), shape(shape), radius(radius) {}

    gfxm::vec3 getPosition() const { return transform[3]; }

    gfxm::vec3 operator()(const gfxm::vec3& dir) const {
        return shape->getMinkowskiSupportPoint(dir, transform) + gfxm::normalize(dir) * radius;
    }
};

static void sphereSweepCallback(void* context, const gfxm::vec3& from, const gfxm::vec3& to, float radius, Collider* cdr) {
    CastSphereContext* ctx = (CastSphereContext*)context;
    if ((ctx->mask & cdr->collision_group) == 0) {
        return;
    }
    const CollisionShape* shape = cdr->getShape();
    if (!shape) {
        assert(false);
        return;
    }
    gfxm::mat4 shape_transform = cdr->getShapeTransform();
    gfxm::vec3 shape_pos = shape_transform * gfxm::vec4(0, 0, 0, 1);
    
    SweepContactPoint scp;
    bool hasHit = false;
    switch (shape->getShapeType()) {
    case COLLISION_SHAPE_TYPE::SPHERE:
        hasHit = intersectionSweepSphereSphere(((const CollisionSphereShape*)shape)->radius, shape_pos, from, to, radius, scp);
        break;
    case COLLISION_SHAPE_TYPE::BOX:
        //hasHit = intersectRayBox(ray, shape_transform, ((const CollisionBoxShape*)shape)->half_extents, rhp);
        break;
    case COLLISION_SHAPE_TYPE::CAPSULE:/*
        hasHit = intersectRayCapsule(
            ray, shape_transform,
            ((const CollisionCapsuleShape*)shape)->height,
            ((const CollisionCapsuleShape*)shape)->radius,
            rhp
        );*/
        break;
    case COLLISION_SHAPE_TYPE::TRIANGLE_MESH: {
        gfxm::vec3 F = gfxm::inverse(shape_transform) * gfxm::vec4(from, 1.f);
        gfxm::vec3 T = gfxm::inverse(shape_transform) * gfxm::vec4(to, 1.f);
        hasHit = intersectSweepSphereTriangleMesh(F, T, radius, ((const CollisionTriangleMeshShape*)shape)->getMesh(), scp);
        //hasHit = intersectRayTriangleMesh(ray, ((const CollisionTriangleMeshShape*)shape)->getMesh(), rhp);
        if(hasHit) {
            scp.normal = shape_transform * gfxm::vec4(scp.normal, .0f);
            scp.contact = shape_transform * gfxm::vec4(scp.contact, 1.f);
            scp.sweep_contact_pos = shape_transform * gfxm::vec4(scp.sweep_contact_pos, 1.f);
        }
        break;
    }
    case COLLISION_SHAPE_TYPE::CONVEX_MESH: {
        gfxm::vec3 F = gfxm::inverse(shape_transform) * gfxm::vec4(from, 1.f);
        gfxm::vec3 T = gfxm::inverse(shape_transform) * gfxm::vec4(to, 1.f);
        hasHit = intersectSweptSphereConvexMesh(F, T, radius, ((const CollisionConvexShape*)shape)->getMesh(), scp);
        if(hasHit) {
            scp.normal = shape_transform * gfxm::vec4(scp.normal, .0f);
            scp.contact = shape_transform * gfxm::vec4(scp.contact, 1.f);
            scp.sweep_contact_pos = shape_transform * gfxm::vec4(scp.sweep_contact_pos, 1.f);
        }
        /*
        CollisionSphereShape sphere_shape;
        sphere_shape.radius = radius;
        const float EPS = .05f;

        float t = .0f;
        const gfxm::vec3 V = to - from;
        const float Vlen = gfxm::length(V);
        while(true) {
            gfxm::vec3 Pfrom = from + gfxm::normalize(V) * t;
            GJKEPA_SupportGetter<POINT_SUPPORT_TAG> sphere_support(Pfrom);
            GJKEPA_SupportGetter<INFLATED_CONVEX_TAG> convex_support(shape_transform, (CollisionConvexShape*)shape, radius);

            GJK_Simplex simplex;
            gfxm::vec3 closestA;
            gfxm::vec3 closestB;

            // Distance from sphere suraface at t to shape surface
            float dist = FLT_MAX;
            if(GJK_distance_T(sphere_support, convex_support, simplex, closestA, closestB, dist)) {
                EPA_Context epa_ctx;
                EPA_Result epa = EPA_T(sphere_support, convex_support, epa_ctx, simplex);
                dist = -epa.depth;
                closestA = epa.contact_a;
                closestB = epa.contact_b;
            }

            // Normal from point on sphere to point on shape
            gfxm::vec3 N = gfxm::normalize(closestB - closestA);

            if(dist <= EPS) {
                hasHit = true;
                scp.normal = -N;
                scp.distance_traveled = t;
                scp.sweep_contact_pos = Pfrom;
                scp.type = CONTACT_POINT_TYPE::DEFAULT;
                scp.contact = closestB;
                break;
            }

            float NdotV = gfxm::dot(N, V);
            float t_step = dist / NdotV;
            t += t_step;

            if(NdotV < .0f) {
                break;
            }
            if(t > Vlen) {
                break;
            }
        }
        */
        break;
    }
    };

    if (hasHit) {
        ctx->hasHit = true;
        if (ctx->scp.distance_traveled > scp.distance_traveled) {
            ctx->scp = scp;
            ctx->closest_collider = cdr;
        }
    }
}
RayCastResult CollisionWorld::rayTest(const gfxm::vec3& from, const gfxm::vec3& to, uint64_t mask) {
    CastRayContext ctx;
    ctx.mask = mask;
    ctx.rhp.distance = INFINITY;
    aabb_tree.rayTest(gfxm::ray(from, to - from), &ctx, &rayTestCallback);

#if COLLISION_DBG_DRAW_TESTS == 1
    if (dbg_draw_enabled) {
        dbgDrawLine(from, to, DBG_COLOR_RED);
    }
#endif
    if (ctx.hasHit) {
        //dbgDrawSphere(ctx.rhp.point, 0.1f, 0xFFFF00FF);
        //dbgDrawArrow(ctx.rhp.point, ctx.rhp.normal, 0xFFFF00FF);
        return RayCastResult{
            ctx.rhp.point,
            ctx.rhp.normal,
            ctx.closest_collider, ctx.rhp.prop, ctx.rhp.distance, ctx.hasHit
        };
    }
    return RayCastResult{
        gfxm::vec3(0,0,0),
        gfxm::vec3(0,0,0),
        0, CollisionSurfaceProp(), .0f, false
    };
}

SphereSweepResult CollisionWorld::sphereSweep(const gfxm::vec3& from, const gfxm::vec3& to, float radius, uint64_t mask) {
    CastSphereContext ctx;
    ctx.mask = mask;
    ctx.scp.distance_traveled = INFINITY;
    SphereSweepResult ssr;
    ssr.sphere_pos = to;
    aabb_tree.sphereSweep(from, to, radius, &ctx, &sphereSweepCallback);
    
#if COLLISION_DBG_DRAW_TESTS == 1
    if (dbg_draw_enabled) {
        dbgDrawSphere(from, radius, DBG_COLOR_RED);
        dbgDrawSphere(to, radius, DBG_COLOR_RED);
        dbgDrawLine(from, to, DBG_COLOR_WHITE);
    }
#endif
    if (ctx.hasHit) {
        ssr.collider = ctx.closest_collider;
        ssr.prop = ctx.scp.prop;
        ssr.contact = ctx.scp.contact;
        ssr.distance = ctx.scp.distance_traveled;
        ssr.hasHit = ctx.hasHit;
        ssr.normal = ctx.scp.normal;
        ssr.sphere_pos = ctx.scp.sweep_contact_pos;
#if COLLISION_DBG_DRAW_TESTS == 1
        if (dbg_draw_enabled) {
            dbgDrawSphere(ssr.sphere_pos, radius, 0xFF00FF99);
            dbgDrawLine(from, ssr.sphere_pos, DBG_COLOR_WHITE);
            dbgDrawArrow(ssr.contact, ssr.normal, DBG_COLOR_BLUE | DBG_COLOR_GREEN);
        }
#endif
    }
    return ssr;
}

void CollisionWorld::sphereTest(const gfxm::mat4& tr, float radius) {
#if COLLISION_DBG_DRAW_TESTS == 1
    if (dbg_draw_enabled) {
        dbgDrawSphere(tr, radius, DBG_COLOR_RED);
    }
#endif
    // TODO
}

void CollisionWorld::debugDraw() {
    if (!dbg_draw_enabled) {
        return;
    }
#if COLLISION_DBG_DRAW_COLLIDERS == 1
    for (int k = 0; k < colliders.size(); ++k) {
        auto& col = colliders[k];
        assert(col->getShape());
        auto shape = col->getShape();
        gfxm::mat4 transform = col->getShapeTransform();

        uint32_t color = 0xFFFFFFFF;
        if (col->getType() == COLLIDER_TYPE::PROBE) {
            color = 0xFF8AA63A;
        }

        dbgDrawAabb(col->getBoundingAabb(), 0xFF999999);

        if (shape->getShapeType() == COLLISION_SHAPE_TYPE::SPHERE) {
            auto shape_sphere = (const CollisionSphereShape*)shape;
            dbgDrawSphere(transform, shape_sphere->radius, color);
        } else if(shape->getShapeType() == COLLISION_SHAPE_TYPE::BOX) {
            auto shape_box = (const CollisionBoxShape*)shape;
            auto& e = shape_box->half_extents;
            dbgDrawBox(transform, shape_box->half_extents, color);
        } else if(shape->getShapeType() == COLLISION_SHAPE_TYPE::CAPSULE) {
            auto shape_capsule = (const CollisionCapsuleShape*)shape;
            auto height = shape_capsule->height;
            auto radius = shape_capsule->radius;
            dbgDrawCapsule(transform, height, radius, color);
        } else if(shape->getShapeType() == COLLISION_SHAPE_TYPE::TRIANGLE_MESH) {
            auto mesh = (const CollisionTriangleMeshShape*)shape;
            //mesh->debugDraw(transform, color);
        } else if (shape->getShapeType() == COLLISION_SHAPE_TYPE::CONVEX_MESH) {
            auto mesh = (const CollisionConvexShape*)shape;
            mesh->debugDraw(transform, color);
        }

        if(col->mass > FLT_EPSILON) {
            dbgDrawLine(col->position, col->position + col->velocity, 0xFF00FF00);
            dbgDrawLine(col->position, col->position + col->angular_velocity, 0xFFFF00FF);
        }
    }
#endif

#if COLLISION_DBG_DRAW_CONTACT_POINTS == 1
    if (dbg_draw_enabled) {
        for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
            auto& m = narrow_phase.getManifold(i);
            for (int j = 0; j < m.point_count; ++j) {
                uint32_t COLOR_A = 0xFF0000FF;
                uint32_t COLOR_B = 0xFFFFFF00;
                if(m.points[j].tick_id != tick_id) {
                    COLOR_A = 0xFF00FF00;
                    COLOR_B = 0xFFFF00FF;
                }
                dbgDrawArrow(m.points[j].point_a, m.points[j].normal_a * m.points[j].depth, COLOR_A);
                dbgDrawArrow(m.points[j].point_b, m.points[j].normal_b * m.points[j].depth, COLOR_B);
                dbgDrawSphere(m.points[j].point_a, .02f, COLOR_A);
                dbgDrawSphere(m.points[j].point_b, .02f, COLOR_B);
                //dbgDrawCross(gfxm::translate(gfxm::mat4(1.0f), m.points[j].position), .5f, 0xFF0000FF);

                gfxm::vec3 P = gfxm::lerp(m.points[j].point_a, m.points[j].point_b, .5f);
                //dbgDrawText(P, std::format("vn: {:.3f}", m.points[j].dbg_vn), 0xFFFFFFFF);
            }
        }
    }
#endif

#if COLLISION_DBG_DRAW_AABB_TREE == 1
    if(dbg_draw_enabled) {
        aabb_tree.debugDraw();
    }
#endif
}

void CollisionWorld::update(float dt, float time_step) {
    timer timer_;
    timer_.start();

    dt_accum += dt;
    while (dt_accum >= time_step) {
        updateInternal(time_step);
        dt_accum -= time_step;
    }

    engineGetStats().collision_time = timer_.stop();
    //
    debugDraw();
}
void CollisionWorld::update_variableDt(float dt) {
    timer timer_;
    timer_.start();

    updateInternal(dt);

    engineGetStats().collision_time = timer_.stop();
    //
    debugDraw();
}
void CollisionWorld::updateInternal(float dt) {
    ++tick_id;

    // Clear per-frame data
    //manifolds.clear();
    clearContactPoints();
    for (auto& it : potential_pairs) {
        it.second.clear();
    }
    for (int i = 0; i < probes.size(); ++i) {
        probes[i]->_clearOverlappingColliders();
    }

    // Update bounds data
    //for (int i = 0; i < colliders.size(); ++i) {
    //    auto collider = colliders[i];
    for (int i = 0; i < dirty_transform_count; ++i) {
        auto collider = dirty_transform_array[i];
        const CollisionShape* shape = collider->getShape();
        if (!shape) {
            assert(false);
            continue;
        }
        gfxm::mat4 transform = collider->getShapeTransform();
        if (!transform.is_valid()) {
            assert(false);
            continue;
        }
        collider->world_aabb = shape->calcWorldAabb(transform);
    }

    // Update aabb tree
    //for (int i = 0; i < colliders.size(); ++i) {
    //    auto collider = colliders[i];
    for (int i = 0; i < dirty_transform_count; ++i) {
        auto collider = dirty_transform_array[i];
        collider->tree_elem.aabb = collider->getBoundingAabb();
        aabb_tree.remove(&collider->tree_elem);
        aabb_tree.add(&collider->tree_elem);
    }
    aabb_tree.update();

    _broadphase();

    // Clear dirty array
    clearDirtyTransformArray();

    // Check for actual collisions
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::SPHERE_SPHERE].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::SPHERE_SPHERE][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::SPHERE_SPHERE][i].second;
        auto a_type = a->getType();
        auto b_type = b->getType();
        auto sa = (const CollisionSphereShape*)a->getShape();
        auto sb = (const CollisionSphereShape*)b->getShape();
        gfxm::vec3 a_pos = a->getShapeTransform() * gfxm::vec4(0, 0, 0, 1);
        gfxm::vec3 b_pos = b->getShapeTransform() * gfxm::vec4(0, 0, 0, 1);
        
        gfxm::vec3 vec_a = b_pos - a_pos;
        float center_distance = gfxm::length(vec_a);
        float radius_distance = sa->radius + sb->radius;
        float distance = center_distance - radius_distance;
        if (distance <= FLT_EPSILON) {
            gfxm::vec3 normal_a = vec_a / center_distance;
            gfxm::vec3 normal_b = -normal_a;
            gfxm::vec3 pt_a = normal_a * sa->radius + a_pos;
            gfxm::vec3 pt_b = normal_b * sb->radius + b_pos;

            CollisionManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, pt_a, pt_b, normal_a, normal_b, distance);
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::BOX_BOX].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::BOX_BOX][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::BOX_BOX][i].second;
        auto sa = (const CollisionBoxShape*)a->getShape();
        auto sb = (const CollisionBoxShape*)b->getShape();
        const gfxm::mat4& tr_a = a->getTransform();
        const gfxm::mat4& tr_b = b->getTransform();
        /*
        ContactPoint cp;
        if (intersectBoxBox(sa->half_extents, tr_a, sb->half_extents, tr_b, cp)) {
            CollisionManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, cp);
        }*/
        GJKEPA_SupportGetter<CollisionBoxShape> a_support(tr_a, sa);
        GJKEPA_SupportGetter<CollisionBoxShape> b_support(tr_b, sb);
        GJK_Simplex simplex;
        EPA_Result result;
        if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
            CollisionManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::SPHERE_BOX].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::SPHERE_BOX][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::SPHERE_BOX][i].second;
        auto sa = (const CollisionSphereShape*)a->getShape();
        auto sb = (const CollisionBoxShape*)b->getShape();
        gfxm::mat4 box_transform = b->getShapeTransform();
        gfxm::vec3 sphere_pos = a->getShapeTransform() * gfxm::vec4(0, 0, 0, 1);        
        ContactPoint cp;
        if (intersectionSphereBox(sa->radius, sphere_pos, sb->half_extents, box_transform, cp)) {
            CollisionManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, cp);
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_SPHERE].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_SPHERE][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_SPHERE][i].second;
        auto sa = (const CollisionSphereShape*)a->getShape();
        auto sb = (const CollisionCapsuleShape*)b->getShape();
        gfxm::mat4 capsule_transform = b->getShapeTransform();
        gfxm::vec3 sphere_pos = a->getShapeTransform() * gfxm::vec4(0, 0, 0, 1);
        ContactPoint cp;
        if (intersectionSphereCapsule(
            sa->radius, sphere_pos, sb->radius, sb->height, capsule_transform, cp
        )) {
            CollisionManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, cp);
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_BOX].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_BOX][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_BOX][i].second;
        auto sa = (const CollisionBoxShape*)a->getShape();
        auto sb = (const CollisionCapsuleShape*)b->getShape();
        gfxm::mat4 a_transform = a->getShapeTransform();
        gfxm::mat4 b_transform = b->getShapeTransform();

        GJKEPA_SupportGetter<CollisionBoxShape> a_support(a_transform, sa);
        GJKEPA_SupportGetter<CollisionCapsuleShape> b_support(b_transform, sb);
        GJK_Simplex simplex;
        EPA_Result result;
        if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
            CollisionManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CAPSULE].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CAPSULE][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CAPSULE][i].second;
        auto sa = (const CollisionCapsuleShape*)a->getShape();
        auto sb = (const CollisionCapsuleShape*)b->getShape();
        
        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);

        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();
        ContactPoint cp;
        if (intersectCapsuleCapsule(
            sa->radius, sa->height, transform_a, sb->radius, sb->height, transform_b, cp
        )) {
            CollisionManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, cp);
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::SPHERE_TRIANGLEMESH].size(); ++i) {
        Collider* a = potential_pairs[COLLISION_PAIR_TYPE::SPHERE_TRIANGLEMESH][i].first;
        Collider* b = potential_pairs[COLLISION_PAIR_TYPE::SPHERE_TRIANGLEMESH][i].second;
        auto sa = (const CollisionSphereShape*)a->getShape();
        auto sb = (const CollisionTriangleMeshShape*)b->getShape();

        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        CollisionManifold* manifold = 0;
        int triangles[128];
        int tri_count = 0;
        tri_count = sb->getMesh()->findPotentialTrianglesAabb(a->getBoundingAabb(), triangles, 128);
        for (int j = 0; j < tri_count; ++j) {
            int tri = triangles[j];
            const gfxm::vec3* vertices = sb->getMesh()->getVertexData();
            const uint32_t* indices = sb->getMesh()->getIndexData();
            gfxm::vec3 A, B, C;
            A = transform_b * gfxm::vec4(vertices[indices[tri * 3]], 1.0f);
            B = transform_b * gfxm::vec4(vertices[indices[tri * 3 + 1]], 1.0f);
            C = transform_b * gfxm::vec4(vertices[indices[tri * 3 + 2]], 1.0f);
#if COLLISION_DBG_DRAW_CONTACT_POINTS == 1
            if (dbg_draw_enabled) {
                dbgDrawLine(A, B, DBG_COLOR_RED);
                dbgDrawLine(B, C, DBG_COLOR_GREEN);
                dbgDrawLine(C, A, DBG_COLOR_BLUE);
            }
#endif
            ContactPoint cp;
            if (intersectSphereTriangle(
                sa->radius, transform_a[3], A, B, C, cp
            )) {
                if (!manifold) {
                    manifold = narrow_phase.createManifold(a, b);
                }
                narrow_phase.addContact(manifold, cp);
            }
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::BOX_TRIANGLEMESH].size(); ++i) {
        Collider* a = potential_pairs[COLLISION_PAIR_TYPE::BOX_TRIANGLEMESH][i].first;
        Collider* b = potential_pairs[COLLISION_PAIR_TYPE::BOX_TRIANGLEMESH][i].second;
        auto sa = (const CollisionBoxShape*)a->getShape();
        auto sb = (const CollisionTriangleMeshShape*)b->getShape();
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        CollisionManifold* manifold = 0;
        int triangles[128];
        int tri_count = 0;
        tri_count = sb->getMesh()->findPotentialTrianglesAabb(a->getBoundingAabb(), triangles, 128);

        for (int j = 0; j < tri_count; ++j) {
            int tri = triangles[j];

            GJKEPA_SupportGetter<CollisionBoxShape> a_support(transform_a, sa);
            GJKEPA_SupportGetter<CollisionTriangleMeshShape> b_support(transform_b, sb, tri);

            GJK_Simplex simplex;
            EPA_Result result;
            if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
                if (!manifold) {
                    manifold = narrow_phase.createManifold(a, b);
                }
                narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
            }
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_TRIANGLEMESH].size(); ++i) {
        Collider* a = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_TRIANGLEMESH][i].first;
        Collider* b = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_TRIANGLEMESH][i].second;
        auto sa = (const CollisionCapsuleShape*)a->getShape();
        auto sb = (const CollisionTriangleMeshShape*)b->getShape();

        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);
        
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        CollisionManifold* manifold = 0;
        int triangles[128];
        int tri_count = 0;
        tri_count = sb->getMesh()->findPotentialTrianglesAabb(a->getBoundingAabb(), triangles, 128);
        
        for (int j = 0; j < tri_count; ++j) {
            int tri = triangles[j];
            const gfxm::vec3* vertices = sb->getMesh()->getVertexData();
            const uint32_t* indices = sb->getMesh()->getIndexData();
            gfxm::vec3 A, B, C;
            A = transform_b * gfxm::vec4(vertices[indices[tri * 3]], 1.0f);
            B = transform_b * gfxm::vec4(vertices[indices[tri * 3 + 1]], 1.0f);
            C = transform_b * gfxm::vec4(vertices[indices[tri * 3 + 2]], 1.0f);
#if COLLISION_DBG_DRAW_CONTACT_POINTS == 1
            if (dbg_draw_enabled) {
                dbgDrawLine(A, B, DBG_COLOR_RED);
                dbgDrawLine(B, C, DBG_COLOR_GREEN);
                dbgDrawLine(C, A, DBG_COLOR_BLUE);
            }
#endif
            ContactPoint cp;
            if (intersectCapsuleTriangle2(
                sa->radius, sa->height, transform_a, A, B, C, cp
            )) {
                // NOTE: This can cause bad behavior when neighboring surfaces are not connected by an edge
                //fixEdgeCollisionNormal(cp, tri, transform_b, sb->getMesh());

                if (!manifold) {
                    manifold = narrow_phase.createManifold(a, b);
                }
                narrow_phase.addContact(manifold, cp);
            }
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CONVEX_MESH].size(); ++i) {
        Collider* a = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CONVEX_MESH][i].first;
        Collider* b = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CONVEX_MESH][i].second;
        auto sa = (const CollisionCapsuleShape*)a->getShape();
        auto sb = (const CollisionConvexShape*)b->getShape();

        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);
        
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        GJKEPA_SupportGetter<CollisionCapsuleShape> a_support(transform_a, sa);
        GJKEPA_SupportGetter<CollisionConvexShape> b_support(transform_b, sb);

        GJK_Simplex simplex;
        EPA_Result result;
        if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
            CollisionManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
            sb->debugDraw(transform_b, 0xFF0000FF);
        }
    }

    // Fill overlapping collider arrays for probe objects
    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        if (m.collider_a->getType() == COLLIDER_TYPE::PROBE) {
            ((ColliderProbe*)m.collider_a)->_addOverlappingCollider(m.collider_b);
        }
        if (m.collider_b->getType() == COLLIDER_TYPE::PROBE) {
            ((ColliderProbe*)m.collider_b)->_addOverlappingCollider(m.collider_a);
        }
    }

    // Apply forces
    for (int i = 0; i < colliders.size(); ++i) {
        auto& collider = colliders[i];
        if(collider->mass < FLT_EPSILON) {
            continue;
        }
        collider->velocity += gfxm::vec3(.0f, -9.8f, .0f) * collider->gravity_factor * dt;
    }

    // Resolve penetration (impulse)
    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        if (m.point_count == 0) {
            assert(false);
            continue;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            continue;
        }
        if (m.collider_a->getType() == COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == COLLIDER_TYPE::PROBE) {
            continue;
        }
        if (m.collider_a->getFlags() & COLLIDER_STATIC
            && m.collider_b->getFlags() & COLLIDER_STATIC) {
            continue;
        }

        for (int j = 0; j < m.point_count; ++j) {
            ContactPoint* pt = &m.points[j];
            _preStepContact2(m.collider_a, m.collider_b, *pt, dt);
            //_warmStartContact(m.collider_a, m.collider_b, *pt);
        }
    }

    //LOG_DBG("### Impulse solver ###");
    constexpr int SOLVER_ITERATIONS = 16;
    for (int si = 0; si < SOLVER_ITERATIONS; ++si) {
        for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
            auto& m = narrow_phase.getManifold(i);
            if (m.point_count == 0) {
                assert(false);
                continue;
            }
            if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
                continue;
            }
            if (m.collider_a->getType() == COLLIDER_TYPE::PROBE
                || m.collider_b->getType() == COLLIDER_TYPE::PROBE) {
                continue;
            }
            if (m.collider_a->getFlags() & COLLIDER_STATIC
                && m.collider_b->getFlags() & COLLIDER_STATIC) {
                continue;
            }

            for (int j = 0; j < m.point_count; ++j) {
                ContactPoint* pt = &m.points[j];
                _applyCollisionIteration(m.collider_a, m.collider_b, *pt, .0f, dt);
            }
        }
    }

    // Store previous position
    for (int i = 0; i < colliders.size(); ++i) {
        auto c = colliders[i];
        c->prev_pos = c->position;
        c->prev_rotation = c->rotation;
    }

    // Integrate velocities
    {
        for (int i = 0; i < colliders.size(); ++i) {
            auto& collider = colliders[i];
            if (FLT_EPSILON > collider->velocity.length2()) {
                continue;
            }
            
            collider->position += collider->velocity * dt;

            _setColliderTransformDirty(collider);
        }
        for (int i = 0; i < colliders.size(); ++i) {
            auto& collider = colliders[i];
            if (FLT_EPSILON > collider->angular_velocity.length2()) {
                continue;
            }

            float angle = gfxm::length(collider->angular_velocity) * dt;
            gfxm::vec3 axis = gfxm::normalize(collider->angular_velocity);
            gfxm::quat dq = gfxm::angle_axis(angle, axis);
            collider->rotation = dq * collider->rotation;
            collider->rotation = gfxm::normalize(collider->rotation);
            assert(collider->rotation.is_valid());

            _setColliderTransformDirty(collider);
        }
    }

    // Transform contact points
    _transformContactPoints();

    // Resolve penetration (adjustment)
    _adjustPenetrations();

    // Damping
    {
        for (int i = 0; i < colliders.size(); ++i) {
            auto& collider = colliders[i];
            if (FLT_EPSILON > collider->velocity.length2()) {
                continue;
            }
            collider->velocity *= powf(.98f, dt);
        }

        for (int i = 0; i < colliders.size(); ++i) {
            auto& collider = colliders[i];
            if (FLT_EPSILON > collider->angular_velocity.length2()) {
                continue;
            }
            collider->angular_velocity *= powf(.98f, dt);
        }
    }
}

int CollisionWorld::addContactPoint(
    const gfxm::vec3& pos_a, const gfxm::vec3& pos_b,
    const gfxm::vec3& normal_a, const gfxm::vec3& normal_b,
    float depth
) {
    ContactPoint cp;
    cp.point_a = pos_a;
    cp.point_b = pos_b;
    cp.normal_a = normal_a;
    cp.normal_b = normal_b;
    cp.depth = depth;
    return addContactPoint(cp);
}
int CollisionWorld::addContactPoint(const ContactPoint& cp) {
    /*assert(contact_point_count < MAX_CONTACT_POINTS);
    if (contact_point_count >= MAX_CONTACT_POINTS) {
        LOG_ERR("Contact point overflow");
        return 0;
    }
    auto& cp_ref = contact_points[contact_point_count];
    cp_ref = cp;
    contact_point_count++;
    return 1;*/
    return 0;
}
ContactPoint* CollisionWorld::getContactPointArrayEnd() {
    //return &front->contact_points[contact_point_count];
    return 0;
}