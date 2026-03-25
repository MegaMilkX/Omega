#include "collision_world.hpp"

#include <algorithm>
#include <format>

#include "collision_util.hpp"
#include "engine.hpp"
#include "util/timer.hpp"

#include "intersection/gjkepa/gjkepa.hpp"
#include "intersection/sat/sat_convexmesh_triangle.hpp"

#include "shape/sphere.hpp"
#include "shape/box.hpp"
#include "shape/capsule.hpp"
#include "shape/convex_mesh.hpp"
#include "shape/triangle_mesh.hpp"
#include "shape/heightfield.hpp"


template<>
class GJKEPA_SupportGetter<phyTriangleMeshShape> {
    gfxm::mat4 transform;
    const phyTriangleMeshShape* shape;
    int triangle_idx;
public:
    GJKEPA_SupportGetter(
        const gfxm::mat4& transform,
        const phyTriangleMeshShape* shape,
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

template<PHY_SHAPE_TYPE SHAPE_TYPE>
struct SHAPE_TYPE_HELPER {};

#define DEF_COLLISION_SHAPE_HELPER(ENUM, TYPE) \
    template<> \
    struct SHAPE_TYPE_HELPER<ENUM> { \
        typedef TYPE SHAPE_T; \
    };

DEF_COLLISION_SHAPE_HELPER(PHY_SHAPE_TYPE::SPHERE , phySphereShape);
DEF_COLLISION_SHAPE_HELPER(PHY_SHAPE_TYPE::BOX , phyBoxShape);
DEF_COLLISION_SHAPE_HELPER(PHY_SHAPE_TYPE::CAPSULE , phyCapsuleShape);
DEF_COLLISION_SHAPE_HELPER(PHY_SHAPE_TYPE::TRIANGLE_MESH, phyTriangleMeshShape);
DEF_COLLISION_SHAPE_HELPER(PHY_SHAPE_TYPE::CONVEX_MESH, phyConvexMeshShape);


inline static bool aabbOverlap(const gfxm::aabb &a, const gfxm::aabb &b) {
    float ox = gfxm::_min(a.to.x, b.to.x) - gfxm::_max(a.from.x, b.from.x);
    float oy = gfxm::_min(a.to.y, b.to.y) - gfxm::_max(a.from.y, b.from.y);
    float oz = gfxm::_min(a.to.z, b.to.z) - gfxm::_max(a.from.z, b.from.z);
    return !(ox < 0.0f || oy < 0.0f || oz < 0.0f);
}


phyWorld::phyWorld() {
    world_body.mass = .0f;
    world_body.setShape(&empty_shape);
}

bool phyWorld::_isTransformDirty(phyRigidBody* c) const {
    return c->dirty_transform_index < dirty_transform_count;
}
void phyWorld::_setColliderTransformDirty(phyRigidBody* collider) {
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
void phyWorld::_addColliderToDirtyTransformArray(phyRigidBody* collider) {
    collider->dirty_transform_index = dirty_transform_array.size();
    dirty_transform_array.push_back(collider);
}
void phyWorld::_removeColliderFromDirtyTransformArray(phyRigidBody* collider) {
    auto idx = collider->dirty_transform_index;
    auto last_idx = dirty_transform_array.size() - 1;
    assert(last_idx >= 0);
    auto tmp = dirty_transform_array[last_idx];
    dirty_transform_array[idx] = tmp;    
    dirty_transform_array.resize(dirty_transform_array.size() - 1);
    tmp->dirty_transform_index = idx;
    collider->dirty_transform_index = -1;
}

void phyWorld::_broadphase() {    
    // Determine potential collisions
    for (int i = 0; i < dirty_transform_count; ++i) {
        auto a = dirty_transform_array[i];
        // TODO: Actually expand aabb at collider scope, not here every time
        gfxm::aabb box = a->world_aabb;
        box.from -= gfxm::vec3(.01f, .01f, .01f);
        box.to += gfxm::vec3(.01f, .01f, .01f);
        aabb_tree.forEachOverlap(box, [this, &a](phyRigidBody* b) {
            bool layer_test
                = (a->collision_group & b->collision_mask)
                && (b->collision_group & a->collision_mask);
            if (!layer_test) {
                return;
            }
            bool a_skip = a->is_sleeping || a->mass == .0f || a->getFlags() & COLLIDER_STATIC;
            bool b_skip = b->is_sleeping || b->mass == .0f || b->getFlags() & COLLIDER_STATIC;
            if(a_skip && b_skip) {
                return;
            }
            if(_isTransformDirty(b) && a->id >= b->id) {
                return;
            }/*
            if(a == b) {
                return;
            }*/

            PHY_PAIR_TYPE pair_identifier = (PHY_PAIR_TYPE)((int)a->getShape()->getShapeType() | (int)b->getShape()->getShapeType());
            if ((int)a->getShape()->getShapeType() <= (int)b->getShape()->getShapeType()) {
                potential_pairs[pair_identifier].push_back(std::make_pair(a, b));
            } else {
                potential_pairs[pair_identifier].push_back(std::make_pair(b, a));
            }
        });
    }
}

void phyWorld::_broadphase_Naive() {
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

                PHY_PAIR_TYPE pair_identifier = (PHY_PAIR_TYPE)((int)a->getShape()->getShapeType() | (int)b->getShape()->getShapeType());
                if ((int)a->getShape()->getShapeType() <= (int)b->getShape()->getShapeType()) {
                    potential_pairs[pair_identifier].push_back(std::make_pair(a, b));
                } else {
                    potential_pairs[pair_identifier].push_back(std::make_pair(b, a));
                }
            }
        }
    }
}

void phyWorld::_transformContactPoints() {    
    auto shouldProcess = [](const phyManifold& m) -> bool {
        if (m.pointCount() == 0) {
            assert(false);
            return false;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            return false;
        }
        if (m.collider_a->getType() == PHY_COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == PHY_COLLIDER_TYPE::PROBE) {
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

        for (int j = 0; j < m.pointCount(); ++j) {
            phyContactPoint* pt = &m.points[j];

            gfxm::mat4 M4A = m.collider_a->getTransform();
            gfxm::mat4 M4B = m.collider_b->getTransform();
            gfxm::vec3 A = M4A * gfxm::vec4(pt->lcl_point_a, 1.f);
            gfxm::vec3 B = M4B * gfxm::vec4(pt->lcl_point_b, 1.f);
            gfxm::vec3 N = gfxm::normalize(M4A * gfxm::vec4(pt->lcl_normal, .0f));
            gfxm::vec3 diff = B - A;
            float depth = gfxm::_max(.0f, gfxm::dot(-diff, pt->normal_a));
            if(!isnan(depth)) {
                pt->point_a = A;
                pt->point_b = B;
                pt->normal_a = N;
                pt->normal_b = -N;
                pt->depth = depth;
            }
        }
    }
}

void phyWorld::_transformContactPoints2() {     
    auto shouldProcess = [](const phyManifold& m) -> bool {
        if (m.pointCount() == 0) {
            assert(false);
            return false;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            return false;
        }
        if (m.collider_a->getType() == PHY_COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == PHY_COLLIDER_TYPE::PROBE) {
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

        for (int j = 0; j < m.pointCount(); ++j) {
            phyContactPoint* pt = &m.points[j];
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

void phyWorld::_adjustPenetrations(float dt) {
    constexpr int ITERATION_COUNT = 8;
    constexpr float CORRECTION_FRACTION = 1.f / float(ITERATION_COUNT);
    const float SLACK = .01f;

    auto shouldProcess = [](const phyManifold& m) -> bool {
        if (m.pointCount() == 0) {
            assert(false);
            return false;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            return false;
        }
        if (m.collider_a->getType() == PHY_COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == PHY_COLLIDER_TYPE::PROBE) {
            return false;
        }
        if (m.collider_a->getFlags() & COLLIDER_STATIC
            && m.collider_b->getFlags() & COLLIDER_STATIC) {
            return false;
        }
        return true;
    };

    for(int iter = 0; iter < ITERATION_COUNT; ++iter) {
        _transformContactPoints();

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
            /*
            std::sort(&m.points[0], &m.points[0] + m.pointCount(), [](const phyContactPoint& a, const phyContactPoint& b)->bool {
                if (a.type != b.type) {
                    return (int)a.type < (int)b.type;
                }
                return a.depth > b.depth;
            });*/
            for (int j = 0; j < m.pointCount(); ++j) {
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

                gfxm::vec3 N = pt->normal_a;
                //float depth_a = pt->depth - gfxm::dot(pt->normal_b, delta_a);
                //float depth_b = pt->depth - gfxm::dot(pt->normal_a, delta_b);
                float depth = gfxm::_max(.0f, pt->depth);

                if (depth < SLACK) {
                    continue;
                }
                delta_a += N * depth * CORRECTION_FRACTION;
                delta_b += N * depth * CORRECTION_FRACTION;
            }

            if (weight_a > FLT_EPSILON) {
                //m.collider_a->position += delta_a * weight_a;
                m.collider_a->correction_accum -= delta_a * weight_a;
                _setColliderTransformDirty(m.collider_a);
            }
            if (weight_b > FLT_EPSILON) {
                //m.collider_b->position += delta_b * weight_b;
                m.collider_b->correction_accum += delta_b * weight_b;
                _setColliderTransformDirty(m.collider_b);
            }
        }
        /*
        for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
            auto& m = narrow_phase.getManifold(i);
            if(!shouldProcess(m)) {
                continue;
            }

            for (int j = 0; j < m.pointCount(); ++j) {
                phyContactPoint& cp = m.points[j];
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
        }*/

        for (int i = 0; i < this->dirty_transform_count; ++i) {
            phyRigidBody* c = dirty_transform_array[i];
            c->position += c->correction_accum;
            //c->velocity += c->correction_accum;
            c->correction_accum = gfxm::vec3(0,0,0);
        }


    }
}

void phyWorld::_adjustPenetrationsOld() {
    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        if (m.pointCount() == 0) {
            assert(false);
            continue;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            continue;
        }
        if (m.collider_a->getType() == PHY_COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == PHY_COLLIDER_TYPE::PROBE) {
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
        /*
        std::sort(&m.points[0], &m.points[0] + m.pointCount(), [](const phyContactPoint& a, const phyContactPoint& b)->bool {
            if (a.type != b.type) {
                return (int)a.type < (int)b.type;
            }
            return a.depth > b.depth;
        });*/
        for (int j = 0; j < m.pointCount(); ++j) {
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

void phyWorld::_solveImpulses(float dt) {
    const float inv_dt = dt > .0f ? (1.f / dt) : .0f;

    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        phyRigidBody* a = m.collider_a;
        phyRigidBody* b = m.collider_b;
        if (m.pointCount() == 0) {
            assert(false);
            continue;
        }
        if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
            continue;
        }
        if (m.collider_a->getType() == PHY_COLLIDER_TYPE::PROBE
            || m.collider_b->getType() == PHY_COLLIDER_TYPE::PROBE) {
            continue;
        }
        bool a_skip = a->is_sleeping || a->mass == .0f || a->getFlags() & COLLIDER_STATIC;
        bool b_skip = b->is_sleeping || b->mass == .0f || b->getFlags() & COLLIDER_STATIC;
        if(a_skip && b_skip) {
            continue;
        }

        for (int j = 0; j < m.pointCount(); ++j) {
            phyContactPoint* pt = &m.points[j];
            _preStepContact(m.collider_a, m.collider_b, *pt, inv_dt);
        }
    }

    for(int i = 0; i < joints.size(); ++i) {
        auto& joint = joints[i];
        _preStepJoint(joint, dt, inv_dt);
    }


    //LOG_DBG("### Impulse solver ###");
    constexpr int SOLVER_ITERATIONS = 16;
    for (int si = 0; si < SOLVER_ITERATIONS; ++si) {
        for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
            auto& m = narrow_phase.getManifold(i);
            _solveManifoldImpulseIteration(m);
            //_solveManifoldFrictionIteration(m);
        }

        for(int i = 0; i < joints.size(); ++i) {
            auto& joint = joints[i];
            _solveJoint(joint);
        }
    }
}

void phyWorld::_preStepContact(phyRigidBody* bodyA, phyRigidBody* bodyB, phyContactPoint& cp, float inv_dt) {
    const float ALLOWED_PENETRATION = .01f;
#if PHY_ENABLE_POSITION_CORRECTION
    const float BIAS_FACTOR = .2f;
#else
    const float BIAS_FACTOR = .0f;
#endif
    gfxm::vec3 wCOM_A = bodyA->position + gfxm::to_mat3(bodyA->getRotation()) * bodyA->mass_center;
    gfxm::vec3 wCOM_B = bodyB->position + gfxm::to_mat3(bodyB->getRotation()) * bodyB->mass_center;

    gfxm::vec3 N = gfxm::normalize(cp.normal_a);
    gfxm::vec3 rA = cp.point_b - wCOM_A;
    gfxm::vec3 rB = cp.point_a - wCOM_B;
    gfxm::vec3 rAxn = gfxm::cross(rA, N);
    gfxm::vec3 rBxn = gfxm::cross(rB, N);

    float invMassA = bodyA->getInverseMass();
    float invMassB = bodyB->getInverseMass();
    float invMassSum = invMassA + invMassB;
    gfxm::mat3 invInertiaA = bodyA->getInverseWorldInertiaTensor();
    gfxm::mat3 invInertiaB = bodyB->getInverseWorldInertiaTensor();
    /*
    if(dynamic_cast<const phyTriangleMeshShape*>(bodyA->shape)
        && dynamic_cast<const phyConvexMeshShape*>(bodyB->shape)) {
        assert(false);
    }*/

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

    // Reproject friction?
    float reproject_jt_t1 = gfxm::dot(cp.t1, cp.Jt_acc);
    float reproject_jt_t2 = gfxm::dot(cp.t2, cp.Jt_acc);
    cp.Jt_acc = cp.t1 * reproject_jt_t1 + cp.t2 * reproject_jt_t2;
    
    // Remove friction completely at warm start?
    //cp.jt_acc = gfxm::vec3(.0f, .0f, .0f);

    gfxm::vec3 rAxt1 = gfxm::cross(rA, cp.t1);
    gfxm::vec3 rBxt1 = gfxm::cross(rB, cp.t1);
    //float denom_t1 = invMassSum + gfxm::dot(invInertiaA * rAxt1, rAxt1) + gfxm::dot(invInertiaB * rBxt1, rBxt1);
    float denom_t1 = invMassSum
        + gfxm::dot(cp.t1, gfxm::cross(invInertiaA * rAxt1, rA))
        + gfxm::dot(cp.t1, gfxm::cross(invInertiaB * rBxt1, rB));
    cp.mass_tangent1 = denom_t1 > .0f ? (1.f / denom_t1) : .0f;

    gfxm::vec3 rAxt2 = gfxm::cross(rA, cp.t2);
    gfxm::vec3 rBxt2 = gfxm::cross(rB, cp.t2);
    //float denom_t2 = invMassSum + gfxm::dot(invInertiaA * rAxt2, rAxt2) + gfxm::dot(invInertiaB * rBxt2, rBxt2);
    float denom_t2 = invMassSum
        + gfxm::dot(cp.t2, gfxm::cross(invInertiaA * rAxt2, rA))
        + gfxm::dot(cp.t2, gfxm::cross(invInertiaB * rBxt2, rB));
    cp.mass_tangent2 = denom_t2 > .0f ? (1.f / denom_t2) : .0f;

    // Bias
    //cp.bias = -BIAS_FACTOR * gfxm::_min(.0f, -cp.depth + ALLOWED_PENETRATION) * inv_dt;
    //cp.bias = -BIAS_FACTOR * inv_dt * gfxm::_min(.0f, -cp.depth + ALLOWED_PENETRATION);
    float separation = gfxm::dot((cp.point_b - cp.point_a), N);
    // TODO: -BIAS_FACTOR originally, find out why this works, something somewhere is flipped
    cp.bias = -BIAS_FACTOR * inv_dt * gfxm::_min(.0f, separation + ALLOWED_PENETRATION);

    // Initial relative normal velocity (for restitution)
    // And manifold tangential expansion
    // TODO: Maybe should calculate earlier (on contact creation)
    gfxm::vec3 vA = bodyA->velocity + gfxm::cross(bodyA->angular_velocity, rA);
    gfxm::vec3 vB = bodyB->velocity + gfxm::cross(bodyB->angular_velocity, rB);
    gfxm::vec3 vRel = vB - vA;
    cp.vn0 = gfxm::dot(vRel, N);
    cp.vt = vRel - cp.vn0 * N;

#if PHY_ENABLE_ACCUMULATION
    // Apply normal + friction impulse
    {
        const gfxm::vec3 impulse_n = cp.normal_a * cp.Jn_acc + cp.Jt_acc;
        _applyImpulse(bodyA, -impulse_n, -gfxm::cross(rA, impulse_n), invMassA, invInertiaA);
        _applyImpulse(bodyB,  impulse_n,  gfxm::cross(rB, impulse_n), invMassB, invInertiaB);
    }
#endif
}

void phyWorld::_solveManifoldImpulseIteration(phyManifold& m) {
    phyRigidBody* a = m.collider_a;
    phyRigidBody* b = m.collider_b;
    if (m.pointCount() == 0) {
        assert(false);
        return;
    }
    if ((a->getFlags() & COLLIDER_NO_RESPONSE) || (b->getFlags() & COLLIDER_NO_RESPONSE)) {
        return;
    }
    if (a->getType() == PHY_COLLIDER_TYPE::PROBE
        || b->getType() == PHY_COLLIDER_TYPE::PROBE) {
        return;
    }
    bool a_skip = a->is_sleeping || a->mass == .0f || a->getFlags() & COLLIDER_STATIC;
    bool b_skip = b->is_sleeping || b->mass == .0f || b->getFlags() & COLLIDER_STATIC;
    if(a_skip && b_skip) {
        return;
    }

    for (int j = 0; j < m.pointCount(); ++j) {
        phyContactPoint* pt = &m.points[j];
        _solveContactIteration(a, b, *pt);
#if PHY_ENABLE_FRICTION
        _solveContactFrictionIteration(a, b, *pt);
#endif
    }
}

void phyWorld::_solveManifoldFrictionIteration(phyManifold& m) {
    if (m.pointCount() == 0) {
        assert(false);
        return;
    }
    if ((m.collider_a->getFlags() & COLLIDER_NO_RESPONSE) || (m.collider_b->getFlags() & COLLIDER_NO_RESPONSE)) {
        return;
    }
    if (m.collider_a->getType() == PHY_COLLIDER_TYPE::PROBE
        || m.collider_b->getType() == PHY_COLLIDER_TYPE::PROBE) {
        return;
    }
    if (m.collider_a->getFlags() & COLLIDER_STATIC
        && m.collider_b->getFlags() & COLLIDER_STATIC) {
        return;
    }
    phyRigidBody* bodyA = m.collider_a;
    phyRigidBody* bodyB = m.collider_b;
    for (int j = 0; j < m.pointCount(); ++j) {
        phyContactPoint& cp = m.points[j];
        _solveContactFrictionIteration(bodyA, bodyB, cp);
    }
}

void phyWorld::_solveContactIteration(phyRigidBody* bodyA, phyRigidBody* bodyB, phyContactPoint& cp) {
    if(bodyA->mass < FLT_EPSILON && bodyB->mass < FLT_EPSILON) {
        return;
    }

    gfxm::vec3 contactPoint = gfxm::lerp(cp.point_a, cp.point_b, 0.5f);
    gfxm::vec3 collisionNormal = gfxm::normalize(cp.normal_a);

    gfxm::vec3 wCMA = gfxm::to_mat3(bodyA->getRotation()) * bodyA->mass_center;
    gfxm::vec3 wCMB = gfxm::to_mat3(bodyB->getRotation()) * bodyB->mass_center;
    gfxm::vec3 rA = contactPoint - (bodyA->position + wCMA + bodyA->center_offset);
    gfxm::vec3 rB = contactPoint - (bodyB->position + wCMB + bodyB->center_offset);

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

    // Restitution
    /*{
        float e = .25f;
        if(cp.tick_id == tick_id && vn < -1e-2f) {
            float jn_bounce = -(1.f + e) * vn * cp.mass_normal;
            gfxm::vec3 impulse_bounce = jn_bounce * collisionNormal;
            _applyImpulse(bodyA, -impulse_bounce, -gfxm::cross(rA, impulse_bounce), invMassA, invInertiaA);
            _applyImpulse(bodyB,  impulse_bounce,  gfxm::cross(rB, impulse_bounce), invMassB, invInertiaB);
        }
        // Refresh relative velocity
        gfxm::vec3 vA = bodyA->velocity + gfxm::cross(bodyA->angular_velocity, rA);
        gfxm::vec3 vB = bodyB->velocity + gfxm::cross(bodyB->angular_velocity, rB);
        gfxm::vec3 relativeVelocity = vB - vA;
        vn = gfxm::dot(relativeVelocity, collisionNormal);
    }*/

    float jn_delta = (-vn + cp.bias) * cp.mass_normal;
    
    // Accumulate and clamp to non-negative
#if PHY_ENABLE_ACCUMULATION
    float Jn_old = cp.Jn_acc;
    /*cp.Jn_acc += jn_delta;
    cp.Jn_acc = gfxm::_max(.0f, cp.Jn_acc);*/
    cp.Jn_acc = gfxm::_max(.0f, Jn_old + jn_delta);
    jn_delta = cp.Jn_acc - Jn_old;
#else
    jn_delta = gfxm::_max(jn_delta, 0.0f);
#endif

    // Apply normal impulse
    gfxm::vec3 impulse_n = jn_delta * collisionNormal;
    _applyImpulse(bodyA, -impulse_n, -gfxm::cross(rA, impulse_n), invMassA, invInertiaA);
    _applyImpulse(bodyB,  impulse_n,  gfxm::cross(rB, impulse_n), invMassB, invInertiaB);
}

void phyWorld::_solveContactFrictionIteration(phyRigidBody* bodyA, phyRigidBody* bodyB, phyContactPoint& cp) {
    gfxm::vec3 collisionNormal = gfxm::normalize(cp.normal_a);

    gfxm::vec3 wCOM_A = bodyA->position + gfxm::to_mat3(bodyA->getRotation()) * bodyA->mass_center;
    gfxm::vec3 wCOM_B = bodyB->position + gfxm::to_mat3(bodyB->getRotation()) * bodyB->mass_center;
    gfxm::vec3 rA = cp.point_b - wCOM_A;
    gfxm::vec3 rB = cp.point_a - wCOM_B;

    gfxm::vec3 vA = bodyA->velocity + gfxm::cross(bodyA->angular_velocity, rA);
    gfxm::vec3 vB = bodyB->velocity + gfxm::cross(bodyB->angular_velocity, rB);
    gfxm::vec3 relativeVelocity = vB - vA;

    gfxm::mat3 invInertiaA = bodyA->getInverseWorldInertiaTensor();
    gfxm::mat3 invInertiaB = bodyB->getInverseWorldInertiaTensor();

    float invMassA = bodyA->getInverseMass();
    float invMassB = bodyB->getInverseMass();

    // ---- FRICTION IMPULSE ----
    // Recalculate relative velocity again
    const gfxm::vec3 avAxrA = gfxm::cross(bodyA->angular_velocity, rA);
    const gfxm::vec3 avBxrB = gfxm::cross(bodyB->angular_velocity, rB);
    assert(avAxrA.is_valid());
    assert(avBxrB.is_valid());
    vA = bodyA->velocity + avAxrA;
    vB = bodyB->velocity + avBxrB;
    relativeVelocity = vB - vA;
    assert(vA.is_valid());
    assert(vB.is_valid());

    float vt1 = gfxm::dot(relativeVelocity, cp.t1);
    float vt2 = gfxm::dot(relativeVelocity, cp.t2);

    float jt1_delta = (-vt1) * cp.mass_tangent1;
    float jt2_delta = (-vt2) * cp.mass_tangent2;

    gfxm::vec3 jt_delta_vec = jt1_delta * cp.t1 + jt2_delta * cp.t2;
    gfxm::vec3 jt_new_vec   = cp.Jt_acc + jt_delta_vec;

    float frictionA = bodyA->friction;
    float frictionB = bodyB->friction;
    float mu = gfxm::sqrt(frictionA * frictionB);


#if PHY_ENABLE_ACCUMULATION
    // Clamp friction
    float Jt_max = gfxm::_max(.0f, mu * cp.Jn_acc);
    if(jt_new_vec.length2() > .000001f && gfxm::length(jt_new_vec) > Jt_max) {
        jt_new_vec = gfxm::normalize(jt_new_vec) * Jt_max;
    }
    jt_delta_vec = jt_new_vec - cp.Jt_acc;
#else
    float Jt_max = mu * cp.Jn_acc;
    jt_delta_vec = gfxm::clamp(jt_delta_vec, -Jt_max, Jt_max);
#endif
    cp.Jt_acc    = jt_new_vec;

    // Apply friction impulse
    _applyImpulse(bodyA, -jt_delta_vec, gfxm::cross(rA, -jt_delta_vec), invMassA, invInertiaA);
    _applyImpulse(bodyB,  jt_delta_vec, gfxm::cross(rB, jt_delta_vec), invMassB, invInertiaB);
}

static gfxm::mat3 crossMatrix(const gfxm::vec3& r) {
    return gfxm::mat3(
        gfxm::vec3(.0f, -r.z, r.y),
        gfxm::vec3(r.z, .0f, -r.x),
        gfxm::vec3(-r.y, r.x, .0f)
    );
}
void phyWorld::_preStepJoint(phyJoint* joint, float dt, float inv_dt) {
    auto body_a = joint->body_a;
    auto body_b = joint->body_b;

    bool a_skip = body_a->is_sleeping || body_a->mass == .0f || body_a->getFlags() & COLLIDER_STATIC;
    bool b_skip = body_b->is_sleeping || body_b->mass == .0f || body_b->getFlags() & COLLIDER_STATIC;
    if(a_skip && b_skip) {
        return;
    }

    float invMassA = body_a->getInverseMass();
    float invMassB = body_b->getInverseMass();
    gfxm::mat3 invInertiaA = body_a->getInverseWorldInertiaTensor();
    gfxm::mat3 invInertiaB = body_b->getInverseWorldInertiaTensor();

    joint->rA = gfxm::to_mat3(body_a->rotation) * joint->lcl_anchor_a;
    joint->rB = gfxm::to_mat3(body_b->rotation) * joint->lcl_anchor_b;
    joint->world_basis = gfxm::to_mat3(body_a->rotation) * joint->lcl_basis_a;
    
    // Linear bias
    gfxm::vec3 pA = body_a->position + joint->rA;
    gfxm::vec3 pB = body_b->position + joint->rB;
    gfxm::vec3 dp = pB - pA;

    const gfxm::vec3& axisx = joint->world_basis[0];
    const gfxm::vec3& axisy = joint->world_basis[1];
    const gfxm::vec3& axisz = joint->world_basis[2];
    
    //joint->linear_mask = gfxm::vec3(1, 1, 0);

    // Default joint
    const gfxm::vec3 bias = -joint->bias_factor * inv_dt * dp;
    joint->lin_bias = gfxm::vec3(
        gfxm::dot(bias, axisx),
        gfxm::dot(bias, axisy),
        gfxm::dot(bias, axisz)
    ) * joint->linear_mask;
    //joint->bias = -joint->bias_factor * inv_dt * dp;
    
    // Spring?
    /*{
        //F = -stiffness * displacement - damping * relative_vel
        const gfxm::vec3 axis = gfxm::normalize(dp);
        const float k_ = gfxm::dot(axis, invInertiaA * axis)
            + gfxm::dot(axis, invInertiaB * axis);
        const float m_eff = (k_ > .0f) ? 1.f / k_ : .0f;
        const float freq = 1.f;
        const float damping_ratio = .1f;
        const float omega = (2.f * gfxm::pi * freq);
        const float k = m_eff * omega * omega;
        const float c = 2.f * m_eff * damping_ratio * omega;
        float gamma = 1.0f / (c + k * dt);  // CFM
        float beta = k * dt / (c + k * dt); // ERP
        joint->softness = gamma;
        joint->bias = -beta * dp * inv_dt;
    }*/
    
    // Effective mass 3x3
    /*
    gfxm::mat3 rA_cross = crossMatrix(joint->rA);
    gfxm::mat3 rB_cross = crossMatrix(joint->rB);
    gfxm::mat3 K = gfxm::mat3(invMassA + invMassB);
    K += gfxm::mat3(joint->softness);
    K += rA_cross * invInertiaA * gfxm::transpose(rA_cross);
    K += rB_cross * invInertiaB * gfxm::transpose(rB_cross);
    */

    gfxm::mat3 K = gfxm::mat3(invMassA + invMassB + joint->softness);
    for(int i = 0; i < 3; ++i) {
        const gfxm::vec3 rAxNi = gfxm::cross(joint->rA, joint->world_basis[i]);
        const gfxm::vec3 rBxNi = gfxm::cross(joint->rB, joint->world_basis[i]);
        for(int j = 0; j < 3; ++j) {
            const gfxm::vec3 rAxNj = gfxm::cross(joint->rA, joint->world_basis[j]);
            const gfxm::vec3 rBxNj = gfxm::cross(joint->rB, joint->world_basis[j]);
            K[i][j] += gfxm::dot(rAxNi, invInertiaA * rAxNj);
            K[i][j] += gfxm::dot(rBxNi, invInertiaB * rBxNj);
        }
    }
    gfxm::mat3 M(.0f);
    M[0][0] = joint->linear_mask.x;
    M[1][1] = joint->linear_mask.y;
    M[2][2] = joint->linear_mask.z;
    gfxm::mat3 I(1.f);
    gfxm::mat3 invM = I - M;
    K = M * K * M + invM;
    joint->Meff = gfxm::inverse(K);

    // Angular Meff
    {
        gfxm::mat3 K = invInertiaA + invInertiaB;
        gfxm::mat3 R(joint->world_basis[0], joint->world_basis[1], joint->world_basis[2]);
        K = gfxm::transpose(R) * K * R;
        joint->Meff_ang = gfxm::inverse(K);
    }

    //const gfxm::vec3 impulse = joint->J_acc;
    gfxm::vec3 impulse
        = joint->world_basis[0] * joint->J_acc.x
        + joint->world_basis[1] * joint->J_acc.y
        + joint->world_basis[2] * joint->J_acc.z;
    _applyImpulse(body_a, -impulse, -gfxm::cross(joint->rA, impulse), invMassA, invInertiaA);
    _applyImpulse(body_b,  impulse,  gfxm::cross(joint->rB, impulse), invMassB, invInertiaB);
    
    // Accumulated friction
    {
        gfxm::vec3 torque
            = joint->world_basis[0] * joint->Jw_acc.x
            + joint->world_basis[1] * joint->Jw_acc.y
            + joint->world_basis[2] * joint->Jw_acc.z;
        body_a->angular_velocity -= invInertiaA * torque;
        body_b->angular_velocity += invInertiaB * torque;
    }

    // Angular
    gfxm::quat qRel = gfxm::inverse(body_a->rotation) * body_b->rotation;
    gfxm::quat qErr = qRel * inverse(joint->qRel0);
    gfxm::quat nq = gfxm::normalize(qErr);
    gfxm::vec3 axis(nq.x, nq.y, nq.z);
    float angle = nq.w;
    axis *= 2.0f;
    joint->angularBias = axis * angle * joint->bias_factor * inv_dt;
}

void phyWorld::_solveJoint(phyJoint* joint) {
    auto body_a = joint->body_a;
    auto body_b = joint->body_b;

    bool a_skip = body_a->is_sleeping || body_a->mass == .0f || body_a->getFlags() & COLLIDER_STATIC;
    bool b_skip = body_b->is_sleeping || body_b->mass == .0f || body_b->getFlags() & COLLIDER_STATIC;
    if(a_skip && b_skip) {
        return;
    }

    float invMassA = body_a->getInverseMass();
    float invMassB = body_b->getInverseMass();
    gfxm::mat3 invInertiaA = body_a->getInverseWorldInertiaTensor();
    gfxm::mat3 invInertiaB = body_b->getInverseWorldInertiaTensor();
    
    gfxm::vec3 vA = body_a->velocity + gfxm::cross(body_a->angular_velocity, joint->rA);
    gfxm::vec3 vB = body_b->velocity + gfxm::cross(body_b->angular_velocity, joint->rB);
    gfxm::vec3 dV = vB - vA; // Relative velocity

    gfxm::vec3 Jv = gfxm::vec3{
        gfxm::dot(joint->world_basis[0], dV),
        gfxm::dot(joint->world_basis[1], dV),
        gfxm::dot(joint->world_basis[2], dV)
    } * joint->linear_mask;

    gfxm::vec3 rhs = joint->lin_bias - Jv - joint->softness * joint->J_acc;
    gfxm::vec3 dLambda = joint->Meff * rhs;
    joint->J_acc += dLambda;

    gfxm::vec3 impulse
        = joint->world_basis[0] * dLambda.x
        + joint->world_basis[1] * dLambda.y
        + joint->world_basis[2] * dLambda.z;
    _applyImpulse(body_a, -impulse, -gfxm::cross(joint->rA, impulse), invMassA, invInertiaA);
    _applyImpulse(body_b,  impulse,  gfxm::cross(joint->rB, impulse), invMassB, invInertiaB);

    // Angular friction
    {
        gfxm::vec3 wRel = body_b->angular_velocity - body_a->angular_velocity;
        gfxm::vec3 Jw = gfxm::vec3{
            gfxm::dot(joint->world_basis[0], wRel),
            gfxm::dot(joint->world_basis[1], wRel),
            gfxm::dot(joint->world_basis[2], wRel)
        };

        const float mu = .02f;
        float max_torque = mu * gfxm::length(joint->J_acc);
        gfxm::vec3 j = joint->Meff_ang * -Jw;
        gfxm::vec3 Jw_old = joint->Jw_acc;
        joint->Jw_acc += j;
        float len = gfxm::length(joint->Jw_acc);
        if(len > max_torque) {
            joint->Jw_acc = joint->Jw_acc * (max_torque / len);
        }
        j = joint->Jw_acc - Jw_old;

        gfxm::vec3 torque
            = joint->world_basis[0] * j.x
            + joint->world_basis[1] * j.y
            + joint->world_basis[2] * j.z;
        body_a->angular_velocity -= invInertiaA * torque;
        body_b->angular_velocity += invInertiaB * torque;
    }

    //gfxm::vec3 impulse = joint->Meff * (joint->bias - dV - joint->softness * joint->J_acc);
    //gfxm::vec3 impulse = joint->Meff * (joint->bias - dV);

    // Angular constraints
    gfxm::mat3 basisA = gfxm::to_mat3(body_a->rotation) * joint->lcl_basis_a;
    gfxm::vec3 axis_x = basisA[0];
    gfxm::vec3 axis_y = basisA[1];
    gfxm::vec3 axis_z = basisA[2];
    gfxm::vec3 wimpulse;
    gfxm::vec3 dW = body_b->angular_velocity - body_a->angular_velocity;
    if(0) {
        gfxm::vec3 axis = axis_x;
        float Cdot = gfxm::dot(dW, axis) + gfxm::dot(joint->angularBias, axis);
        float k
            = gfxm::dot(axis, invInertiaA * axis)
            + gfxm::dot(axis, invInertiaB * axis);
        float Meff = (k > .0f) ? 1.f / k : .0f;
        float lambda = Meff * -Cdot;

        wimpulse += axis * lambda;
    }
    if(0) {
        gfxm::vec3 axis = axis_y;
        float Cdot = gfxm::dot(dW, axis) + gfxm::dot(joint->angularBias, axis);
        float k
            = gfxm::dot(axis, invInertiaA * axis)
            + gfxm::dot(axis, invInertiaB * axis);
        float Meff = (k > .0f) ? 1.f / k : .0f;
        float lambda = Meff * -Cdot;

        wimpulse += axis * lambda;
    }
    if(0) {
        gfxm::vec3 axis = axis_z;
        float Cdot = gfxm::dot(dW, axis) + gfxm::dot(joint->angularBias, axis);
        float k
            = gfxm::dot(axis, invInertiaA * axis)
            + gfxm::dot(axis, invInertiaB * axis);
        float Meff = (k > .0f) ? 1.f / k : .0f;
        float lambda = Meff * -Cdot;

        wimpulse += axis * lambda;
    }
    if (invMassA > .0f) {
        body_a->angular_velocity -= invInertiaA * wimpulse;
    }
    if (invMassB > .0f) {
        body_b->angular_velocity += invInertiaB * wimpulse;
    }
}

gfxm::vec3 phyWorld::_solveLinearConstraint(phyJoint* joint, const gfxm::vec3& axis, const gfxm::vec3& dV, float bias) {
    float dVn = gfxm::dot(dV, axis);
    float Jaccn = gfxm::dot(joint->J_acc, axis);

    return joint->Meff * axis * (bias - dVn - joint->softness * Jaccn);
}

void phyWorld::_applyImpulse(phyRigidBody* body, const gfxm::vec3 linJ, const gfxm::vec3& angJ, float invMass, const gfxm::mat3& invInertiaWorld) {
    if(invMass <= .0f) return;
    assert(linJ.is_valid());
    assert(angJ.is_valid());
    body->velocity += linJ * invMass;
    body->angular_velocity += invInertiaWorld * angJ;
    assert(body->angular_velocity.is_valid());
    assert(body->velocity.is_valid());
}


void phyWorld::addCollider(phyRigidBody* collider) {
    static uint32_t next_collider_id = 0;
    
    collider->id = next_collider_id++;

    colliders.push_back(collider);
    if (collider->getType() == PHY_COLLIDER_TYPE::PROBE) {
        probes.push_back((phyProbe*)collider);
    }

    collider->tree_elem.aabb = collider->getBoundingAabb();
    aabb_tree.add(&collider->tree_elem);

    collider->collision_world = this;

    _addColliderToDirtyTransformArray(collider);
    _setColliderTransformDirty(collider);
    collider->calcInertiaTensor();
    collider->is_sleeping = true;
}
void phyWorld::removeCollider(phyRigidBody* collider) {
    _removeColliderFromDirtyTransformArray(collider);

    aabb_tree.remove(&collider->tree_elem);

    collider->collision_world = 0;

    for (int i = 0; i < colliders.size(); ++i) {
        if (colliders[i] == collider) {
            colliders.erase(colliders.begin() + i);
            break;
        }
    }
    if (collider->getType() == PHY_COLLIDER_TYPE::PROBE) {
        for (int i = 0; i < probes.size(); ++i) {
            if (probes[i] == (phyProbe*)collider) {
                probes.erase(probes.begin() + i);
                break;
            }
        }
    }
}
void phyWorld::markAsExternallyTransformed(phyRigidBody* collider) {
    _setColliderTransformDirty(collider);
}

void phyWorld::addJoint(phyJoint* joint) {
    assert(joint->body_a || joint->body_b);
    if(joint->body_a == nullptr) {
        joint->body_a = &world_body;
    }
    if(joint->body_b == nullptr) {
        joint->body_b = &world_body;
    }
    joints.push_back(joint);
}
void phyWorld::removeJoint(phyJoint* joint) {
    for (int i = 0; i < joints.size(); ++i) {
        if(joints[i] == joint) {
            joints.erase(joints.begin() + i);
            break;
        }
    }
}

struct CastRayContext {
    RayHitPoint rhp;
    phyRigidBody* closest_collider = 0;
    uint64_t mask = 0;
    bool hasHit = false;
};
struct CastSphereContext {
    SweepContactPoint scp;
    phyRigidBody* closest_collider = 0;
    uint64_t mask = 0;
    bool hasHit = false;
};
static void rayTestCallback(void* context, const gfxm::ray& ray, phyRigidBody* cdr) {
    CastRayContext* ctx = (CastRayContext*)context;

    if ((ctx->mask & cdr->collision_group) == 0) {
        return;
    }
    
    const phyShape* shape = cdr->getShape();
    if (!shape) {
        assert(false);
        return;
    }

    gfxm::mat4 shape_transform = cdr->getShapeTransform();
    gfxm::vec3 shape_pos = shape_transform * gfxm::vec4(0, 0, 0, 1);

    RayHitPoint rhp;
    bool hasHit = false;
    switch (shape->getShapeType()) {
    case PHY_SHAPE_TYPE::SPHERE:
        hasHit = intersectRaySphere(ray, shape_pos, ((const phySphereShape*)shape)->radius, rhp);
        break;
    case PHY_SHAPE_TYPE::BOX:
        hasHit = intersectRayBox(ray, shape_transform, ((const phyBoxShape*)shape)->half_extents, rhp);
        break;
    case PHY_SHAPE_TYPE::CAPSULE:
        hasHit = intersectRayCapsule(
            ray, shape_transform,
            ((const phyCapsuleShape*)shape)->height,
            ((const phyCapsuleShape*)shape)->radius,
            rhp
        );
        break;
    case PHY_SHAPE_TYPE::TRIANGLE_MESH:
        hasHit = intersectRayTriangleMesh(ray, ((const phyTriangleMeshShape*)shape)->getMesh(), rhp);
        break;
    case PHY_SHAPE_TYPE::CONVEX_MESH: {
        const gfxm::vec3 O = gfxm::inverse(shape_transform) * gfxm::vec4(ray.origin, 1.f);
        const gfxm::vec3 D = gfxm::inverse(shape_transform) * gfxm::vec4(ray.direction, .0f);
        const gfxm::ray R(O, D, ray.length);
        hasHit = intersectRayConvexMesh(R, ((const phyConvexMeshShape*)shape)->getMesh(), rhp);
        if(hasHit) {
            rhp.point = shape_transform * gfxm::vec4(rhp.point, 1.f);
            rhp.normal = shape_transform * gfxm::vec4(rhp.normal, .0f);
        }
        break;
    }
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
    const phyConvexMeshShape* shape;
    float radius;
public:
    GJKEPA_SupportGetter(const gfxm::mat4& transform, const phyConvexMeshShape* shape, float radius)
        : transform(transform), shape(shape), radius(radius) {}

    gfxm::vec3 getPosition() const { return transform[3]; }

    gfxm::vec3 operator()(const gfxm::vec3& dir) const {
        return shape->getMinkowskiSupportPoint(dir, transform) + gfxm::normalize(dir) * radius;
    }
};

static void sphereSweepCallback(void* context, const gfxm::vec3& from, const gfxm::vec3& to, float radius, phyRigidBody* cdr) {
    CastSphereContext* ctx = (CastSphereContext*)context;
    if ((ctx->mask & cdr->collision_group) == 0) {
        return;
    }
    const phyShape* shape = cdr->getShape();
    if (!shape) {
        assert(false);
        return;
    }
    gfxm::mat4 shape_transform = cdr->getShapeTransform();
    gfxm::vec3 shape_pos = shape_transform * gfxm::vec4(0, 0, 0, 1);
    
    SweepContactPoint scp;
    bool hasHit = false;
    switch (shape->getShapeType()) {
    case PHY_SHAPE_TYPE::SPHERE:
        hasHit = intersectionSweepSphereSphere(((const phySphereShape*)shape)->radius, shape_pos, from, to, radius, scp);
        break;
    case PHY_SHAPE_TYPE::BOX:
        //hasHit = intersectRayBox(ray, shape_transform, ((const phyBoxShape*)shape)->half_extents, rhp);
        break;
    case PHY_SHAPE_TYPE::CAPSULE:/*
        hasHit = intersectRayCapsule(
            ray, shape_transform,
            ((const phyCapsuleShape*)shape)->height,
            ((const phyCapsuleShape*)shape)->radius,
            rhp
        );*/
        break;
    case PHY_SHAPE_TYPE::TRIANGLE_MESH: {
        gfxm::vec3 F = gfxm::inverse(shape_transform) * gfxm::vec4(from, 1.f);
        gfxm::vec3 T = gfxm::inverse(shape_transform) * gfxm::vec4(to, 1.f);
        hasHit = intersectSweepSphereTriangleMesh(F, T, radius, ((const phyTriangleMeshShape*)shape)->getMesh(), scp);
        //hasHit = intersectRayTriangleMesh(ray, ((const phyTriangleMeshShape*)shape)->getMesh(), rhp);
        if(hasHit) {
            scp.normal = shape_transform * gfxm::vec4(scp.normal, .0f);
            scp.contact = shape_transform * gfxm::vec4(scp.contact, 1.f);
            scp.sweep_contact_pos = shape_transform * gfxm::vec4(scp.sweep_contact_pos, 1.f);
        }
        break;
    }
    case PHY_SHAPE_TYPE::CONVEX_MESH: {
        gfxm::vec3 F = gfxm::inverse(shape_transform) * gfxm::vec4(from, 1.f);
        gfxm::vec3 T = gfxm::inverse(shape_transform) * gfxm::vec4(to, 1.f);
        hasHit = intersectSweptSphereConvexMesh(F, T, radius, ((const phyConvexMeshShape*)shape)->getMesh(), scp);
        if(hasHit) {
            scp.normal = shape_transform * gfxm::vec4(scp.normal, .0f);
            scp.contact = shape_transform * gfxm::vec4(scp.contact, 1.f);
            scp.sweep_contact_pos = shape_transform * gfxm::vec4(scp.sweep_contact_pos, 1.f);
        }
        break;
    }
    case PHY_SHAPE_TYPE::HEIGHTFIELD: {
        gfxm::vec3 F = gfxm::inverse(shape_transform) * gfxm::vec4(from, 1.f);
        gfxm::vec3 T = gfxm::inverse(shape_transform) * gfxm::vec4(to, 1.f);
        hasHit = intersectSweptSphereHeightfield(F, T, radius, ((const phyHeightfieldShape*)shape), scp);
        if(hasHit) {
            scp.normal = shape_transform * gfxm::vec4(scp.normal, .0f);
            scp.contact = shape_transform * gfxm::vec4(scp.contact, 1.f);
            scp.sweep_contact_pos = shape_transform * gfxm::vec4(scp.sweep_contact_pos, 1.f);
        }
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
phyRayCastResult phyWorld::rayTest(const gfxm::vec3& from, const gfxm::vec3& to, uint64_t mask) {
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
        return phyRayCastResult{
            ctx.rhp.point,
            ctx.rhp.normal,
            ctx.closest_collider, ctx.rhp.prop, ctx.rhp.distance, ctx.hasHit
        };
    }
    return phyRayCastResult{
        gfxm::vec3(0,0,0),
        gfxm::vec3(0,0,0),
        0, phySurfaceProp(), .0f, false
    };
}

phySphereSweepResult phyWorld::sphereSweep(const gfxm::vec3& from, const gfxm::vec3& to, float radius, uint64_t mask) {
    CastSphereContext ctx;
    ctx.mask = mask;
    ctx.scp.distance_traveled = INFINITY;
    phySphereSweepResult ssr;
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

void phyWorld::sphereTest(const gfxm::mat4& tr, float radius) {
#if COLLISION_DBG_DRAW_TESTS == 1
    if (dbg_draw_enabled) {
        dbgDrawSphere(tr, radius, DBG_COLOR_RED);
    }
#endif
    // TODO
}

void phyWorld::debugDraw() {
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
        if (col->getType() == PHY_COLLIDER_TYPE::PROBE) {
            color = 0xFF8AA63A;
        } else if (col->is_sleeping) {
            color = 0xFFFF77CC;
        }

        //dbgDrawAabb(col->getBoundingAabb(), 0xFF999999);

        if (shape->getShapeType() == PHY_SHAPE_TYPE::SPHERE) {
            auto shape_sphere = (const phySphereShape*)shape;
            dbgDrawSphere(transform, shape_sphere->radius, color);
        } else if(shape->getShapeType() == PHY_SHAPE_TYPE::BOX) {
            auto shape_box = (const phyBoxShape*)shape;
            auto& e = shape_box->half_extents;
            dbgDrawBox(transform, shape_box->half_extents, color);
        } else if(shape->getShapeType() == PHY_SHAPE_TYPE::CAPSULE) {
            auto shape_capsule = (const phyCapsuleShape*)shape;
            auto height = shape_capsule->height;
            auto radius = shape_capsule->radius;
            dbgDrawCapsule(transform, height, radius, color);
        } else if(shape->getShapeType() == PHY_SHAPE_TYPE::TRIANGLE_MESH) {
            auto mesh = (const phyTriangleMeshShape*)shape;
            //mesh->debugDraw(transform, color);
        } else if (shape->getShapeType() == PHY_SHAPE_TYPE::CONVEX_MESH) {
            auto mesh = (const phyConvexMeshShape*)shape;
            mesh->debugDraw(transform, color);
        }

        if(col->mass > FLT_EPSILON) {
            dbgDrawLine(col->position, col->position + col->velocity, 0xFF00FF00);
            dbgDrawLine(col->position, col->position + col->angular_velocity, 0xFFFF00FF);
        }
    }
#endif

#if COLLISION_DBG_DRAW_CONSTRAINTS
    for (int i = 0; i < joints.size(); ++i) {
        phyJoint* joint = joints[i];
        if(joint->body_a != &world_body) {
            dbgDrawLine(joint->body_a->position, joint->body_a->position + joint->rA, 0xFFFFFFFF);
        }
        dbgDrawLine(joint->body_a->position + joint->rA, joint->body_b->position + joint->rB, 0xFFFFFFFF);
        if(joint->body_b != &world_body) {
            dbgDrawLine(joint->body_b->position + joint->rB, joint->body_b->position, 0xFFFFFFFF);
        }
        dbgDrawSphere(joint->body_a->position + joint->rA, .05f, 0xFFFF0000);
        dbgDrawSphere(joint->body_b->position + joint->rB, .05f, 0xFF00FF00);
    }
#endif

#if COLLISION_DBG_DRAW_CONTACT_POINTS == 1
    if (dbg_draw_enabled) {
        auto project = [](const gfxm::vec3& t1, const gfxm::vec3& t2, const gfxm::vec3& p)->gfxm::vec2{
            return gfxm::vec2(gfxm::dot(t1, p), gfxm::dot(t2, p));
        };

        for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
            auto& m = narrow_phase.getManifold(i);
            // Manifold
            
            if(m.pointCount() > 0) {
                //dbgDrawText(m.points[0].point_a, std::format("MANIFOLD_{:#06x}", m.key).c_str(), 0xFFFFFFFF);
                gfxm::vec3 C;
                for(int j = 0; j < m.pointCount(); ++j) {
                    C += m.points[j].point_a;
                }
                C /= float(m.pointCount());

                const gfxm::vec3 t1 = gfxm::normalize(m.t1);
                const gfxm::vec3 t2 = gfxm::normalize(m.t2);
                const gfxm::vec3 n = gfxm::normalize(gfxm::cross(t1, t2));
                const float d = gfxm::dot(C, n);
                C = n * d;
                const gfxm::rect rc = m.extremes;//gfxm::rect(-1, -1, 1, 1);
                gfxm::vec3 PTS[4] = {
                    C + t1 * rc.min.x + t2 * rc.min.y,
                    C + t1 * rc.max.x + t2 * rc.min.y,
                    C + t1 * rc.max.x + t2 * rc.max.y,
                    C + t1 * rc.min.x + t2 * rc.max.y
                };
                dbgDrawLine(PTS[0], PTS[1], 0xFF00FFFF);
                dbgDrawLine(PTS[1], PTS[2], 0xFF00FFFF);
                dbgDrawLine(PTS[2], PTS[3], 0xFF00FFFF);
                dbgDrawLine(PTS[3], PTS[0], 0xFF00FFFF);

                if(0) {
                    gfxm::vec3 C;
                    for(int j = 0; j < 4; ++j) {
                        C += PTS[j];
                    }
                    C /= float(4);
                    dbgDrawText(C, std::format("S: {:.3f}", m.hull.area2()), 0xFFFFFFFF);
                }
                /*
                const gfxm::vec3 AT = C + t1 + t2;
                dbgDrawText(
                    (PTS[0] + PTS[1] + PTS[2] + PTS[3]) * .25f,
                    std::format("[{}, {}, {}]", (m.key >> 10) & 0x7F, (m.key >> 5) & 0x7F, m.key & 0x7F),
                    0xFFFFFFFF
                );
                dbgDrawArrow(
                    (PTS[0] + PTS[1] + PTS[2] + PTS[3]) * .25f, m.initial_normal, 0xFFFF00FF
                );*/
                //dbgDrawLine(m.points[0].point_a, m.points[0].point_a + m.t1, 0xFF00FFFF);
                //dbgDrawLine(m.points[0].point_a, m.points[0].point_a + m.t2, 0xFF00FFFF);
            }
            for(int j = 0; j < m.pointCount(); ++j) {
                dbgDrawLine(m.points[j].point_a, m.points[(j + 1) % m.pointCount()].point_a, 0xFF00FF00);
            }

            // Points
            for (int j = 0; j < m.pointCount(); ++j) {
                uint32_t COLOR_A = 0xFF0000FF;
                uint32_t COLOR_B = 0xFFFFFF00;
                if(m.points[j].tick_id != tick_id) {
                    COLOR_A = 0xFF00FF00;
                    COLOR_B = 0xFFFF00FF;
                }/*
                const gfxm::vec3 A = m.points[j].point_a;
                const gfxm::vec3 B = m.points[j].point_b;
                const gfxm::vec3 NA = m.points[j].normal_a;
                const gfxm::vec3 NB = m.points[j].normal_b;
                float depth = m.points[j].depth;
                */

                const gfxm::vec3 A = m.collider_a->getTransform() * gfxm::vec4(m.points[j].lcl_point_a, 1.f);
                const gfxm::vec3 B = m.collider_b->getTransform() * gfxm::vec4(m.points[j].lcl_point_b, 1.f);
                //const gfxm::vec3 NA = gfxm::to_mat3(m.collider_a->getRotation()) * m.points[j].lcl_normal;
                //const gfxm::vec3 NB = -NA;
                const gfxm::vec3 NA = m.points[j].normal_a;
                const gfxm::vec3 NB = m.points[j].normal_b;
                float depth = m.points[j].depth;

                dbgDrawArrow(A, NA, COLOR_A);
                dbgDrawArrow(B, NB, COLOR_B);
                dbgDrawSphere(A, .02f, COLOR_A);
                dbgDrawSphere(B, .02f, COLOR_B);
                //dbgDrawCross(gfxm::translate(gfxm::mat4(1.0f), m.points[j].position), .5f, 0xFF0000FF);

                gfxm::vec3 P = gfxm::lerp(m.points[j].point_a, m.points[j].point_b, .5f);
                //dbgDrawText(P, std::format("vn: {:.3f}", m.points[j].dbg_vn), 0xFFFFFFFF);
                //dbgDrawText(P, std::format("tick: {}", m.points[j].tick_id), 0xFFFFFFFF);
                //dbgDrawText(P, std::format("{}", j), 0xFFFFFFFF);
            }
        }
    }
#endif

#if COLLISION_DBG_DRAW_AABB_TREE
    if(dbg_draw_enabled) {
        aabb_tree.debugDraw();
    }
#endif
}

void phyWorld::update(float dt, float time_step, int max_steps) {
    timer timer_;
    timer_.start();

    dt_accum += dt;
    int step = 0;
    int n_steps = int(dt_accum / time_step);
    if(max_steps > 0) {
        n_steps = std::min(n_steps, max_steps);
    }
    while (step < n_steps) {
        updateInternal(time_step);
        ++step;
    }
    dt_accum -= time_step * float(step);

    engineGetStats().collision_time = timer_.stop();
    //
    debugDraw();
}
void phyWorld::update_variableDt(float dt) {
    timer timer_;
    timer_.start();

    updateInternal(dt);

    engineGetStats().collision_time = timer_.stop();
    //
    debugDraw();
}
bool dbg_stepPhysics = true;
void phyWorld::updateInternal(float dt) {
    if (dbg_stepPhysics) {
        //dbg_stepPhysics = false;
    } else {
        return;
    }
    ++tick_id;

    // Clear per-frame data
    //clearContactPoints();

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
        const phyShape* shape = collider->getShape();
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

    // Find contact points to preserve
    if(1) {
        int contact_points_confirmed = 0;
        const float SEPARATION_THRESHOLD = 2e-2f;
        const float SHEARING_THRESHOLD = 5e-2f;
        for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
            phyManifold& M = narrow_phase.getManifold(i);
            int write_index = 0;
            for (int j = 0; j < M.pointCount(); ++j) {
                phyContactPoint cp = M.points[j];
                const gfxm::vec3 wA = M.collider_a->getTransform() * gfxm::vec4(cp.lcl_point_a, 1.f);
                gfxm::vec3 wB       = M.collider_b->getTransform() * gfxm::vec4(cp.lcl_point_b, 1.f);
                const gfxm::vec3 wN = gfxm::normalize(gfxm::to_mat3(M.collider_a->getRotation()) * cp.lcl_normal);
                float separation = gfxm::dot(wN, wB - wA);

                if (tick_id - cp.tick_id > 3) {
                    //continue;
                }
                if (separation > SEPARATION_THRESHOLD) {
                    continue;
                }
                gfxm::vec3 shearing = (wB - wA) - separation * wN;
                if(shearing.length2() > SHEARING_THRESHOLD * SHEARING_THRESHOLD) {
                    continue;
                }
                
                wB = wA + wN * separation;

                assert(wN.is_valid());
            
                cp.point_a = wA;
                cp.point_b = wB;
                cp.normal_a = wN;
                cp.normal_b = -wN;
                cp.depth = -separation;// < .0f ? .0f : -separation;
                
                M.points[write_index] = cp;
                ++write_index;
                ++contact_points_confirmed;
            }
            M.points.resize(write_index);
            narrow_phase.rebuildManifold(&M);
        }
        //LOG_DBG("Confirmed contacts: " << contact_points_confirmed);
    }
    
    /*
    LOG_DBG("=== Manifolds ===");
    LOG_DBG("Manifolds back: " << narrow_phase.oldManifoldCount());
    LOG_DBG("Manifolds front: " << narrow_phase.manifoldCount());
    */

    // Check for actual collisions
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::SPHERE_SPHERE].size(); ++i) {
        auto a = potential_pairs[PHY_PAIR_TYPE::SPHERE_SPHERE][i].first;
        auto b = potential_pairs[PHY_PAIR_TYPE::SPHERE_SPHERE][i].second;
        auto a_type = a->getType();
        auto b_type = b->getType();
        auto sa = (const phySphereShape*)a->getShape();
        auto sb = (const phySphereShape*)b->getShape();
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

            phyManifold* manifold = narrow_phase.getManifold(a, b, normal_a);
            narrow_phase.addContact(manifold, pt_a, pt_b, normal_a, normal_b, distance);
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::BOX_BOX].size(); ++i) {
        auto a = potential_pairs[PHY_PAIR_TYPE::BOX_BOX][i].first;
        auto b = potential_pairs[PHY_PAIR_TYPE::BOX_BOX][i].second;
        auto sa = (const phyBoxShape*)a->getShape();
        auto sb = (const phyBoxShape*)b->getShape();
        const gfxm::mat4& tr_a = a->getTransform();
        const gfxm::mat4& tr_b = b->getTransform();
        /*
        phyContactPoint cp;
        if (intersectBoxBox(sa->half_extents, tr_a, sb->half_extents, tr_b, cp)) {
            phyManifold* manifold = narrow_phase.createManifold(a, b);
            narrow_phase.addContact(manifold, cp);
        }*/
        GJKEPA_SupportGetter<phyBoxShape> a_support(tr_a, sa);
        GJKEPA_SupportGetter<phyBoxShape> b_support(tr_b, sb);
        GJK_Simplex simplex;
        EPA_Result result;
        if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
            phyManifold* manifold = narrow_phase.getManifold(a, b, 0);
            narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::SPHERE_BOX].size(); ++i) {
        auto a = potential_pairs[PHY_PAIR_TYPE::SPHERE_BOX][i].first;
        auto b = potential_pairs[PHY_PAIR_TYPE::SPHERE_BOX][i].second;
        auto sa = (const phySphereShape*)a->getShape();
        auto sb = (const phyBoxShape*)b->getShape();
        gfxm::mat4 box_transform = b->getShapeTransform();
        gfxm::vec3 sphere_pos = a->getShapeTransform() * gfxm::vec4(0, 0, 0, 1);        
        phyContactPoint cp;
        if (intersectionSphereBox(sa->radius, sphere_pos, sb->half_extents, box_transform, cp)) {
            phyManifold* manifold = narrow_phase.getManifold(a, b, 0);
            narrow_phase.addContact(manifold, cp);
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::CAPSULE_SPHERE].size(); ++i) {
        auto a = potential_pairs[PHY_PAIR_TYPE::CAPSULE_SPHERE][i].first;
        auto b = potential_pairs[PHY_PAIR_TYPE::CAPSULE_SPHERE][i].second;
        auto sa = (const phySphereShape*)a->getShape();
        auto sb = (const phyCapsuleShape*)b->getShape();
        gfxm::mat4 capsule_transform = b->getShapeTransform();
        gfxm::vec3 sphere_pos = a->getShapeTransform() * gfxm::vec4(0, 0, 0, 1);
        phyContactPoint cp;
        if (intersectionSphereCapsule(
            sa->radius, sphere_pos, sb->radius, sb->height, capsule_transform, cp
        )) {
            phyManifold* manifold = narrow_phase.getManifold(a, b, 0);
            narrow_phase.addContact(manifold, cp);
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::CAPSULE_BOX].size(); ++i) {
        auto a = potential_pairs[PHY_PAIR_TYPE::CAPSULE_BOX][i].first;
        auto b = potential_pairs[PHY_PAIR_TYPE::CAPSULE_BOX][i].second;
        auto sa = (const phyBoxShape*)a->getShape();
        auto sb = (const phyCapsuleShape*)b->getShape();
        gfxm::mat4 a_transform = a->getShapeTransform();
        gfxm::mat4 b_transform = b->getShapeTransform();

        GJKEPA_SupportGetter<phyBoxShape> a_support(a_transform, sa);
        GJKEPA_SupportGetter<phyCapsuleShape> b_support(b_transform, sb);
        GJK_Simplex simplex;
        EPA_Result result;
        if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
            phyManifold* manifold = narrow_phase.getManifold(a, b, 0);
            narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::CAPSULE_CAPSULE].size(); ++i) {
        auto a = potential_pairs[PHY_PAIR_TYPE::CAPSULE_CAPSULE][i].first;
        auto b = potential_pairs[PHY_PAIR_TYPE::CAPSULE_CAPSULE][i].second;
        auto sa = (const phyCapsuleShape*)a->getShape();
        auto sb = (const phyCapsuleShape*)b->getShape();
        
        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);

        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();
        phyContactPoint cp;
        if (intersectCapsuleCapsule(
            sa->radius, sa->height, transform_a, sb->radius, sb->height, transform_b, cp
        )) {
            phyManifold* manifold = narrow_phase.getManifold(a, b, 0);
            narrow_phase.addContact(manifold, cp);
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::SPHERE_TRIANGLEMESH].size(); ++i) {
        phyRigidBody* a = potential_pairs[PHY_PAIR_TYPE::SPHERE_TRIANGLEMESH][i].first;
        phyRigidBody* b = potential_pairs[PHY_PAIR_TYPE::SPHERE_TRIANGLEMESH][i].second;
        auto sa = (const phySphereShape*)a->getShape();
        auto sb = (const phyTriangleMeshShape*)b->getShape();

        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        phyManifold* manifold = 0;
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
            phyContactPoint cp;
            if (intersectSphereTriangle(
                sa->radius, transform_a[3], A, B, C, cp
            )) {
                phyManifold* manifold = narrow_phase.getManifold(a, b, cp.normal_a);
                fixEdgeCollisionNormal(cp, tri, transform_b, sb->getMesh());
                narrow_phase.addContact(manifold, cp);
            }
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::BOX_TRIANGLEMESH].size(); ++i) {
        phyRigidBody* a = potential_pairs[PHY_PAIR_TYPE::BOX_TRIANGLEMESH][i].first;
        phyRigidBody* b = potential_pairs[PHY_PAIR_TYPE::BOX_TRIANGLEMESH][i].second;
        auto sa = (const phyBoxShape*)a->getShape();
        auto sb = (const phyTriangleMeshShape*)b->getShape();
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        int triangles[128];
        int tri_count = 0;
        tri_count = sb->getMesh()->findPotentialTrianglesAabb(a->getBoundingAabb(), triangles, 128);

        for (int j = 0; j < tri_count; ++j) {
            int tri = triangles[j];

            GJKEPA_SupportGetter<phyBoxShape> a_support(transform_a, sa);
            GJKEPA_SupportGetter<phyTriangleMeshShape> b_support(transform_b, sb, tri);

            GJK_Simplex simplex;
            EPA_Result result;
            if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
                phyManifold* manifold = narrow_phase.getManifold(a, b, result.normal);
                narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
            }
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::CAPSULE_TRIANGLEMESH].size(); ++i) {
        phyRigidBody* a = potential_pairs[PHY_PAIR_TYPE::CAPSULE_TRIANGLEMESH][i].first;
        phyRigidBody* b = potential_pairs[PHY_PAIR_TYPE::CAPSULE_TRIANGLEMESH][i].second;
        auto sa = (const phyCapsuleShape*)a->getShape();
        auto sb = (const phyTriangleMeshShape*)b->getShape();

        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);
        
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        phyManifold* manifold = 0;
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
            phyContactPoint cp;
            if (intersectCapsuleTriangle2(
                sa->radius, sa->height, transform_a, A, B, C, cp
            )) {
                // NOTE: This can cause bad behavior when neighboring surfaces are not connected by an edge
                //fixEdgeCollisionNormal(cp, tri, transform_b, sb->getMesh());

                phyManifold* manifold = narrow_phase.getManifold(a, b, cp.normal_a);
                narrow_phase.addContact(manifold, cp);
            }
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::CAPSULE_CONVEX_MESH].size(); ++i) {
        phyRigidBody* a = potential_pairs[PHY_PAIR_TYPE::CAPSULE_CONVEX_MESH][i].first;
        phyRigidBody* b = potential_pairs[PHY_PAIR_TYPE::CAPSULE_CONVEX_MESH][i].second;
        auto sa = (const phyCapsuleShape*)a->getShape();
        auto sb = (const phyConvexMeshShape*)b->getShape();

        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);
        
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        GJKEPA_SupportGetter<phyCapsuleShape> a_support(transform_a, sa);
        GJKEPA_SupportGetter<phyConvexMeshShape> b_support(transform_b, sb);

        GJK_Simplex simplex;
        EPA_Result result;
        if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
            phyManifold* manifold = narrow_phase.getManifold(a, b, 0);
            narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
            sb->debugDraw(transform_b, 0xFF0000FF);
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::TRIANGLE_MESH_CONVEX_MESH].size(); ++i) {
        phyRigidBody* a = potential_pairs[PHY_PAIR_TYPE::TRIANGLE_MESH_CONVEX_MESH][i].first;
        phyRigidBody* b = potential_pairs[PHY_PAIR_TYPE::TRIANGLE_MESH_CONVEX_MESH][i].second;
        auto sa = (const phyTriangleMeshShape*)a->getShape();
        auto sb = (const phyConvexMeshShape*)b->getShape();
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        int triangles[128];
        int tri_count = 0;
        tri_count = sa->getMesh()->findPotentialTrianglesAabb(b->getBoundingAabb(), triangles, 128);

        for (int j = 0; j < tri_count; ++j) {
            int tri = triangles[j];

            const gfxm::vec3* vertices = sa->getMesh()->getVertexData();
            const uint32_t* indices = sa->getMesh()->getIndexData();
            const gfxm::vec3 tri_points[3] = {
                transform_a * gfxm::vec4(vertices[indices[tri * 3]], 1.0f),
                transform_a * gfxm::vec4(vertices[indices[tri * 3 + 1]], 1.0f),
                transform_a * gfxm::vec4(vertices[indices[tri * 3 + 2]], 1.0f)
            };
#if COLLISION_DBG_DRAW_CONTACT_POINTS == 1
            gfxm::vec3 A = tri_points[0], B = tri_points[1], C = tri_points[2];
            if (dbg_draw_enabled) {
                dbgDrawLine(A, B, DBG_COLOR_RED);
                dbgDrawLine(B, C, DBG_COLOR_GREEN);
                dbgDrawLine(C, A, DBG_COLOR_BLUE);
            }
#endif
            /*
            phyContactPoint cp;
            if (SAT_ConvexMeshTriangle(transform_b, sb->getMesh(), tri_points, cp)) {
                phyManifold* manifold = narrow_phase.getManifold(a, b, cp.normal_a);
                narrow_phase.addContact(manifold, cp.point_a, cp.point_b, cp.normal_a, cp.normal_b, cp.depth);
            }*/
            GJKEPA_SupportGetter<phyTriangleMeshShape> a_support(transform_a, sa, tri);
            GJKEPA_SupportGetter<phyConvexMeshShape> b_support(transform_b, sb);            

            GJK_Simplex simplex;
            EPA_Result result;
            if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
                phyManifold* manifold = narrow_phase.getManifold(a, b, result.normal);
                narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
            }
        }
    }
    /*
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::TRIANGLE_MESH_CONVEX_MESH].size(); ++i) {
        phyRigidBody* a = potential_pairs[PHY_PAIR_TYPE::TRIANGLE_MESH_CONVEX_MESH][i].first;
        phyRigidBody* b = potential_pairs[PHY_PAIR_TYPE::TRIANGLE_MESH_CONVEX_MESH][i].second;
        auto sa = (const phyTriangleMeshShape*)a->getShape();
        auto sb = (const phyConvexMeshShape*)b->getShape();

        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);
        
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        int triangles[128];
        int tri_count = 0;
        tri_count = sa->getMesh()->findPotentialTrianglesAabb(b->getBoundingAabb(), triangles, 128);

        for (int j = 0; j < tri_count; ++j) {
            int tri = triangles[j];

#if COLLISION_DBG_DRAW_CONTACT_POINTS == 1
            const gfxm::vec3* vertices = sa->getMesh()->getVertexData();
            const uint32_t* indices = sa->getMesh()->getIndexData();
            gfxm::vec3 A, B, C;
            A = transform_a * gfxm::vec4(vertices[indices[tri * 3]], 1.0f);
            B = transform_a * gfxm::vec4(vertices[indices[tri * 3 + 1]], 1.0f);
            C = transform_a * gfxm::vec4(vertices[indices[tri * 3 + 2]], 1.0f);
            if (dbg_draw_enabled) {
                dbgDrawLine(A, B, DBG_COLOR_RED);
                dbgDrawLine(B, C, DBG_COLOR_GREEN);
                dbgDrawLine(C, A, DBG_COLOR_BLUE);
            }
#endif
            GJKEPA_SupportGetter<phyTriangleMeshShape> a_support(transform_a, sa, tri);
            GJKEPA_SupportGetter<phyConvexMeshShape> b_support(transform_b, sb);            

            GJK_Simplex simplex;
            EPA_Result result;
            if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
                phyManifold* manifold = narrow_phase.getManifold(a, b, result.normal);
                narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
            }
        }
    }*/
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::CONVEX_MESH_CONVEX_MESH].size(); ++i) {
        phyRigidBody* a = potential_pairs[PHY_PAIR_TYPE::CONVEX_MESH_CONVEX_MESH][i].first;
        phyRigidBody* b = potential_pairs[PHY_PAIR_TYPE::CONVEX_MESH_CONVEX_MESH][i].second;
        auto sa = (const phyConvexMeshShape*)a->getShape();
        auto sb = (const phyConvexMeshShape*)b->getShape();

        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);

        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        GJKEPA_SupportGetter<phyConvexMeshShape> a_support(transform_a, sa);
        GJKEPA_SupportGetter<phyConvexMeshShape> b_support(transform_b, sb);

        GJK_Simplex simplex;
        EPA_Result result;
        if (GJKEPA_T(a_support, b_support, simplex, epa_ctx, result)) {
            phyManifold* manifold = narrow_phase.getManifold(a, b, result.normal);
            narrow_phase.addContact(manifold, result.contact_a, result.contact_b, result.normal, -result.normal, result.depth);
        }
    }
    for (int i = 0; i < potential_pairs[PHY_PAIR_TYPE::SPHERE_HEIGHTFIELD].size(); ++i) {
        phyRigidBody* a = potential_pairs[PHY_PAIR_TYPE::SPHERE_HEIGHTFIELD][i].first;
        phyRigidBody* b = potential_pairs[PHY_PAIR_TYPE::SPHERE_HEIGHTFIELD][i].second;
        auto sphere = (const phySphereShape*)a->getShape();
        auto heightfield = (const phyHeightfieldShape*)b->getShape();
        const gfxm::mat4 transform_a = a->getShapeTransform();
        const gfxm::mat4 transform_b = b->getShapeTransform();

        const gfxm::mat4 inv_b = gfxm::inverse(transform_b);
        const gfxm::vec3 sphere_pos = inv_b * transform_a[3];

        float minx = sphere_pos.x - sphere->radius;
        float maxx = sphere_pos.x + sphere->radius;
        float minz = sphere_pos.z - sphere->radius;
        float maxz = sphere_pos.z + sphere->radius;
        float miny = sphere_pos.y - sphere->radius;
        float maxy = sphere_pos.y + sphere->radius;

        float cell_width = heightfield->getCellWidth();
        float cell_depth = heightfield->getCellDepth();

        int sminx = gfxm::clamp(minx / cell_width, .0f, heightfield->getSampleCountX() - 1);
        int smaxx = gfxm::clamp(1 + maxx / cell_width, .0f, heightfield->getSampleCountX() - 1);
        int sminz = gfxm::clamp(minz / cell_depth, .0f, heightfield->getSampleCountZ() - 1);
        int smaxz = gfxm::clamp(1 + maxz / cell_depth, .0f, heightfield->getSampleCountZ() - 1);

        if (sminx == smaxx || sminz == smaxz) {
            continue;
        }

        {
            gfxm::vec3 p0 = transform_b * gfxm::vec4(minx, .0f, minz, 1.f);
            gfxm::vec3 p1 = transform_b * gfxm::vec4(maxx, .0f, minz, 1.f);
            gfxm::vec3 p2 = transform_b * gfxm::vec4(maxx, .0f, maxz, 1.f);
            gfxm::vec3 p3 = transform_b * gfxm::vec4(minx, .0f, maxz, 1.f);

            dbgDrawLine(p0, p1, 0xFFFF00FF);
            dbgDrawLine(p1, p2, 0xFFFF00FF);
            dbgDrawLine(p2, p3, 0xFFFF00FF);
            dbgDrawLine(p3, p0, 0xFFFF00FF);
        }
        {
            gfxm::vec3 p0 = transform_b * gfxm::vec4(sminx * cell_width, .0f, sminz * cell_depth, 1.f);
            gfxm::vec3 p1 = transform_b * gfxm::vec4(smaxx * cell_width, .0f, sminz * cell_depth, 1.f);
            gfxm::vec3 p2 = transform_b * gfxm::vec4(smaxx * cell_width, .0f, smaxz * cell_depth, 1.f);
            gfxm::vec3 p3 = transform_b * gfxm::vec4(sminx * cell_width, .0f, smaxz * cell_depth, 1.f);

            dbgDrawLine(p0, p1, 0xFF0000FF);
            dbgDrawLine(p1, p2, 0xFF0000FF);
            dbgDrawLine(p2, p3, 0xFF0000FF);
            dbgDrawLine(p3, p0, 0xFF0000FF);
        }

        int scount_x = heightfield->getSampleCountX();
        const float* samples = heightfield->getData();
        for (int z = sminz; z < smaxz; ++z) {
            for (int x = sminx; x < smaxx; ++x) {
                float h0 = samples[x + z * scount_x];
                float h1 = samples[x + (z + 1) * scount_x];
                float h2 = samples[x + 1 + (z + 1) * scount_x];
                float h3 = samples[x + 1 + z * scount_x];

                float hmin = gfxm::_min(gfxm::_min(h0, h1), gfxm::_min(h2, h3));
                float hmax = gfxm::_max(gfxm::_max(h0, h1), gfxm::_max(h2, h3));

                if ((hmin < miny && hmax < miny)
                    || (hmin > maxy && hmax > maxy)
                    ) {
                    continue;
                }

                {
                    gfxm::vec3 p0 = transform_b * gfxm::vec4(x * cell_width, h0, z * cell_depth, 1.f);
                    gfxm::vec3 p1 = transform_b * gfxm::vec4(x * cell_width, h1, (z + 1) * cell_depth, 1.f);
                    gfxm::vec3 p2 = transform_b * gfxm::vec4((x + 1) * cell_width, h2, (z + 1) * cell_depth, 1.f);
                    gfxm::vec3 p3 = transform_b * gfxm::vec4((x + 1) * cell_width, h3, z * cell_depth, 1.f);

                    dbgDrawLine(p0, p1, 0xFFFFFFFF);
                    dbgDrawLine(p1, p2, 0xFFFFFFFF);
                    dbgDrawLine(p2, p3, 0xFFFFFFFF);
                    dbgDrawLine(p3, p0, 0xFFFFFFFF);
                    dbgDrawLine(p2, p0, 0xFFFFFFFF);
                }

                gfxm::vec3 p0(x * cell_width, h0, z * cell_depth);
                gfxm::vec3 p1(x * cell_width, h1, (z + 1) * cell_depth);
                gfxm::vec3 p2((x + 1) * cell_width, h2, (z + 1) * cell_depth);
                gfxm::vec3 p3((x + 1) * cell_width, h3, z * cell_depth);
                phyContactPoint cp;
                if (intersectSphereTriangle(
                    sphere->radius, sphere_pos, p0, p1, p2, cp
                )) {
                    cp.normal_a = transform_b * gfxm::vec4(cp.normal_a, .0f);
                    cp.normal_b = transform_b * gfxm::vec4(cp.normal_b, .0f);
                    cp.point_a = transform_b * gfxm::vec4(cp.point_a, 1.f);
                    cp.point_b = transform_b * gfxm::vec4(cp.point_b, 1.f);
                    phyManifold* manifold = narrow_phase.getManifold(a, b, cp.normal_a);
                    narrow_phase.addContact(manifold, cp);
                }
                if (intersectSphereTriangle(
                    sphere->radius, sphere_pos, p2, p3, p0, cp
                )) {
                    cp.normal_a = transform_b * gfxm::vec4(cp.normal_a, .0f);
                    cp.normal_b = transform_b * gfxm::vec4(cp.normal_b, .0f);
                    cp.point_a = transform_b * gfxm::vec4(cp.point_a, 1.f);
                    cp.point_b = transform_b * gfxm::vec4(cp.point_b, 1.f);
                    phyManifold* manifold = narrow_phase.getManifold(a, b, cp.normal_a);
                    narrow_phase.addContact(manifold, cp);
                }
            }
        }
    }

    // Poke awake colliding bodies
    /*
    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        m.collider_a->is_sleeping = false;
        m.collider_b->is_sleeping = false;
    }*/

    // Fill overlapping collider arrays for probe objects
    for (int i = 0; i < narrow_phase.manifoldCount(); ++i) {
        auto& m = narrow_phase.getManifold(i);
        if (m.collider_a->getType() == PHY_COLLIDER_TYPE::PROBE) {
            ((phyProbe*)m.collider_a)->_addOverlappingCollider(m.collider_b);
        }
        if (m.collider_b->getType() == PHY_COLLIDER_TYPE::PROBE) {
            ((phyProbe*)m.collider_b)->_addOverlappingCollider(m.collider_a);
        }
    }

    // Apply forces
    for (int i = 0; i < colliders.size(); ++i) {
        auto& collider = colliders[i];
        if (collider->is_sleeping) {
            continue;
        }/*
        if(collider->mass < FLT_EPSILON) {
            continue;
        }*/
        if(collider->getInverseMass() == .0f) {
            continue;
        }
        collider->velocity += gravity * collider->gravity_factor * dt;
    }

    // Resolve penetration (impulse)
    _solveImpulses(dt);

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
            if (collider->is_sleeping) {
                continue;
            }

            float vmag2 = collider->velocity.length2();
            float avmag2 = collider->angular_velocity.length2();

            if (vmag2 > FLT_EPSILON) {
                collider->position += collider->velocity * dt;

                _setColliderTransformDirty(collider);
            }

            if (avmag2 > FLT_EPSILON) {
                gfxm::mat3 rot_from = gfxm::to_mat3(collider->getRotation());
                gfxm::vec3 wCOM = collider->position + rot_from * collider->mass_center;

                float angle = gfxm::length(collider->angular_velocity) * dt;
                gfxm::vec3 axis = gfxm::normalize(collider->angular_velocity);
                gfxm::quat dq = gfxm::angle_axis(angle, axis);
                collider->rotation = dq * collider->rotation;
                collider->rotation = gfxm::normalize(collider->rotation);
                assert(collider->rotation.is_valid());

                gfxm::mat3 rot_to = gfxm::to_mat3(collider->getRotation());
                collider->position = wCOM - rot_to * collider->mass_center;

                _setColliderTransformDirty(collider);
            }
        }
    }

    // Transform contact points
    //_transformContactPoints();

#if PHY_ENABLE_POSITION_CORRECTION_BLUNT
    // Resolve penetration (adjustment)
    _adjustPenetrations(dt);
#endif
    
    // Damping
    if(1){
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

    const float SLEEP_THRESHOLD_SEC = 5.f;
    for (int i = 0; i < colliders.size(); ++i) {
        auto& collider = colliders[i];
        const float EPS = 1e-3f;
        if (collider->velocity.length2() < EPS
            && collider->angular_velocity.length2() < EPS
        ) {
            collider->sleep_timer += dt;
            if(collider->sleep_timer > SLEEP_THRESHOLD_SEC) {
                collider->is_sleeping = true;
                collider->velocity = gfxm::vec3(.0f, .0f, .0f);
                collider->angular_velocity = gfxm::vec3(.0f, .0f, .0f);
            }
        } else {
            collider->is_sleeping = false;
            collider->sleep_timer = .0f;
        }
    }
}

