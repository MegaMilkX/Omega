#include "collision_world.hpp"

#include <algorithm>

#include "collision_util.hpp"
#include "engine.hpp"
#include "util/timer.hpp"



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


void CollisionWorld::addCollider(Collider* collider) {
    colliders.push_back(collider);
    if (collider->getType() == COLLIDER_TYPE::PROBE) {
        probes.push_back((ColliderProbe*)collider);
    }

    collider->tree_elem.aabb = collider->getBoundingAabb();
    aabb_tree.add(&collider->tree_elem);

    collider->collision_world = this;

    _addColliderToDirtyTransformArray(collider);
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
    case COLLISION_SHAPE_TYPE::TRIANGLE_MESH:
        hasHit = intersectSweepSphereTriangleMesh(from, to, radius, ((const CollisionTriangleMeshShape*)shape)->getMesh(), scp);
        //hasHit = intersectRayTriangleMesh(ray, ((const CollisionTriangleMeshShape*)shape)->getMesh(), rhp);
        break;
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
            ctx.closest_collider, ctx.rhp.distance, ctx.hasHit
        };
    }
    return RayCastResult{
        gfxm::vec3(0,0,0),
        gfxm::vec3(0,0,0),
        0, .0f, false
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
        }
    }
#endif

#if COLLISION_DBG_DRAW_CONTACT_POINTS == 1
    if (dbg_draw_enabled) {
        for (int i = 0; i < manifolds.size(); ++i) {
            auto& m = manifolds[i];
            for (int j = 0; j < m.point_count; ++j) {
                dbgDrawArrow(m.points[j].point_a, m.points[j].normal_a * m.points[j].depth, 0xFF0000FF);
                dbgDrawArrow(m.points[j].point_b, m.points[j].normal_b * m.points[j].depth, 0xFFFFFF00);
                dbgDrawSphere(m.points[j].point_a, .02f, 0xFF0000FF);
                dbgDrawSphere(m.points[j].point_b, .02f, 0xFFFFFF00);
                //dbgDrawCross(gfxm::translate(gfxm::mat4(1.0f), m.points[j].position), .5f, 0xFF0000FF);
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

void CollisionWorld::update(float dt) {
    timer timer_;
    timer_.start();

    // Clear per-frame data
    dirty_transform_count = 0;
    manifolds.clear();
    clearContactPoints();
    for (auto& it : potential_pairs) {
        it.second.clear();
    }
    for (int i = 0; i < probes.size(); ++i) {
        probes[i]->_clearOverlappingColliders();
    }

    // Update bounds data
    for (int i = 0; i < colliders.size(); ++i) {
        auto collider = colliders[i];
        const CollisionShape* shape = collider->getShape();
        if (!shape) {
            assert(false);
            continue;
        }
        gfxm::mat4 transform = collider->getShapeTransform();
        collider->world_aabb = shape->calcWorldAabb(transform);
    }

    // Update aabb tree
    for (int i = 0; i < colliders.size(); ++i) {
        auto collider = colliders[i];
        collider->tree_elem.aabb = collider->getBoundingAabb();
        aabb_tree.remove(&collider->tree_elem);
        aabb_tree.add(&collider->tree_elem);
    }
    aabb_tree.update();

    // Determine potential collisions
    for (int i = 0; i < colliders.size(); ++i) {
        for (int j = i + 1; j < colliders.size(); ++j) {
            auto a = colliders[i];
            auto b = colliders[j];
            bool layer_test
                = (a->collision_group & b->collision_mask)
                && (b->collision_group & a->collision_mask);
            if (!layer_test) {
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

            ContactPoint* cp_ptr = &contact_points[contact_point_count];
            int cp_count = 1;
            addContactPoint(pt_a, pt_b, normal_a, normal_b, distance);

            CollisionManifold manifold;
            manifold.points = cp_ptr;
            manifold.point_count = cp_count;
            manifold.depth = distance;
            manifold.normal = normal_a;
            manifold.collider_a = a;
            manifold.collider_b = b;
            manifolds.push_back(manifold);

        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::BOX_BOX].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::BOX_BOX][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::BOX_BOX][i].second;
        auto sa = (const CollisionBoxShape*)a->getShape();
        auto sb = (const CollisionBoxShape*)b->getShape();
        gfxm::vec3 side1 = sa->half_extents * 2.0f;
        gfxm::vec3 side2 = sb->half_extents * 2.0f;
        gfxm::mat3 R1_ = gfxm::to_mat3(a->rotation);
        gfxm::mat3 R2_ = gfxm::to_mat3(b->rotation);
        gfxm::vec3 a_pos = a->getShapeTransform() * gfxm::vec4(0, 0, 0, 1);
        gfxm::vec3 b_pos = b->getShapeTransform() * gfxm::vec4(0, 0, 0, 1);

        CollisionManifold manifold;
        btVector3 p1;
        btVector3 p2;
        btVector3 bt_side1;
        btVector3 bt_side2;
        p1.setValue(a_pos.x, a_pos.y, a_pos.z);
        p2.setValue(b_pos.x, b_pos.y, b_pos.z);
        bt_side1.setValue(side1.x, side1.y, side1.z);
        bt_side2.setValue(side2.x, side2.y, side2.z);
        btVector3 normal;
        btScalar depth;
        int ret_code = 0;
        int max_contacts = 8;
        dMatrix3 R1;
        dMatrix3 R2;
        R1[0] = R1_[0][0]; R1[1] = R1_[0][1]; R1[2] = R1_[0][2]; R1[3] = .0f;
        R1[4] = R1_[1][0]; R1[5] = R1_[1][1]; R1[6] = R1_[1][2]; R1[7] = .0f;
        R1[8] = R1_[2][0]; R1[9] = R1_[2][1]; R1[10] = R1_[2][2]; R1[11] = .0f;
        R2[0] = R2_[0][0]; R2[1] = R2_[0][1]; R2[2] = R2_[0][2]; R2[3] = .0f;
        R2[4] = R2_[1][0]; R2[5] = R2_[1][1]; R2[6] = R2_[1][2]; R2[7] = .0f;
        R2[8] = R2_[2][0]; R2[9] = R2_[2][1]; R2[10] = R2_[2][2]; R2[11] = .0f;
        dBoxBox2(p1, R1, bt_side1, p2, R2, bt_side2, normal, &depth, &ret_code, max_contacts, this, &manifold);

        if (ret_code != 0) {
            gfxm::vec3 normal_(normal.getX(), normal.getY(), normal.getZ());
            manifold.normal = normal_;
            manifold.depth = depth;
            manifold.collider_a = a;
            manifold.collider_b = b;
            manifolds.push_back(manifold);
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
            ContactPoint* cp_ptr = &contact_points[contact_point_count];
            int cp_num = addContactPoint(cp);
            CollisionManifold manifold;
            manifold.collider_a = a;
            manifold.collider_b = b;
            manifold.points = cp_ptr;
            manifold.point_count = cp_num;
            manifolds.push_back(manifold);
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
            ContactPoint* cp_ptr = &contact_points[contact_point_count];
            int cp_num = addContactPoint(cp);
            CollisionManifold manifold;
            manifold.collider_a = a;
            manifold.collider_b = b;
            manifold.points = cp_ptr;
            manifold.point_count = cp_num;
            manifolds.push_back(manifold);
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CAPSULE].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CAPSULE][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_CAPSULE][i].second;
        auto sa = (const CollisionCapsuleShape*)a->getShape();
        auto sb = (const CollisionCapsuleShape*)b->getShape();
        
        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);

        gfxm::mat4 transform_a = a->getShapeTransform();
        gfxm::mat4 transform_b = b->getShapeTransform();
        ContactPoint cp;
        if (intersectCapsuleCapsule(
            sa->radius, sa->height, transform_a, sb->radius, sb->height, transform_b, cp
        )) {
            ContactPoint* cp_ptr = &contact_points[contact_point_count];
            int cp_num = addContactPoint(cp);
            CollisionManifold manifold;
            manifold.collider_a = a;
            manifold.collider_b = b;
            manifold.points = cp_ptr;
            manifold.point_count = cp_num;
            manifolds.push_back(manifold);
        }
    }
    for (int i = 0; i < potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_TRIANGLEMESH].size(); ++i) {
        auto a = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_TRIANGLEMESH][i].first;
        auto b = potential_pairs[COLLISION_PAIR_TYPE::CAPSULE_TRIANGLEMESH][i].second;
        auto sa = (const CollisionCapsuleShape*)a->getShape();
        auto sb = (const CollisionTriangleMeshShape*)b->getShape();

        //dbgDrawLine(a->position, b->position, DBG_COLOR_RED);
        
        gfxm::mat4 transform_a = a->getShapeTransform();
        gfxm::mat4 transform_b = b->getShapeTransform();

        CollisionManifold manifold;
        manifold.collider_a = a;
        manifold.collider_b = b;
        ContactPoint* cp_ptr = &contact_points[contact_point_count];
        manifold.points = cp_ptr;
        manifold.point_count = 0;
        int triangles[128];
        int tri_count = 0;
        tri_count = sb->getMesh()->findPotentialTrianglesAabb(a->getBoundingAabb(), triangles, 128);
        
        for (int i = 0; i < tri_count; ++i) {
            int tri = triangles[i];
            gfxm::vec3 A, B, C;
            A = transform_b * gfxm::vec4(sb->getMesh()->getVertexData()[sb->getMesh()->getIndexData()[tri * 3]], 1.0f);
            B = transform_b * gfxm::vec4(sb->getMesh()->getVertexData()[sb->getMesh()->getIndexData()[tri * 3 + 1]], 1.0f);
            C = transform_b * gfxm::vec4(sb->getMesh()->getVertexData()[sb->getMesh()->getIndexData()[tri * 3 + 2]], 1.0f);
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
                manifold.point_count += addContactPoint(cp);
            }
        }
        if (manifold.point_count > 0) {
            manifolds.push_back(manifold);
        }
    }

    // Fill overlapping collider arrays for probe objects
    for (int i = 0; i < manifolds.size(); ++i) {
        auto& m = manifolds[i];
        if (m.collider_a->getType() == COLLIDER_TYPE::PROBE) {
            ((ColliderProbe*)m.collider_a)->_addOverlappingCollider(m.collider_b);
        }
        if (m.collider_b->getType() == COLLIDER_TYPE::PROBE) {
            ((ColliderProbe*)m.collider_b)->_addOverlappingCollider(m.collider_a);
        }
    }

    // Resolve ?
    for (int i = 0; i < manifolds.size(); ++i) {
        auto& m = manifolds[i];
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

            gfxm::vec3 Nb = pt->normal_b;
            gfxm::vec3 Na = pt->normal_a;
            float depth_a = pt->depth - gfxm::dot(pt->normal_b, delta_a);
            float depth_b = pt->depth - gfxm::dot(pt->normal_a, delta_b);
            if (depth_a < .0f && depth_b < .0f) {
                continue;
            }
            delta_a += Nb * depth_a;
            delta_b += Na * depth_b;

            //m.collider_a->position += pt->normal_b * pt->depth * weight_a;
            //m.collider_b->position += pt->normal_a * pt->depth * weight_b;   
            //m.collider_a->position = m.collider_a->prev_pos;
            //m.collider_b->position = m.collider_b->prev_pos;
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

    for (int i = 0; i < colliders.size(); ++i) {
        auto c = colliders[i];
        c->prev_pos = c->position;
    }

    engineGetStats().collision_time = timer_.stop();
    //
    debugDraw();
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
    assert(contact_point_count < MAX_CONTACT_POINTS);
    if (contact_point_count >= MAX_CONTACT_POINTS) {
        LOG_ERR("Contact point overflow");
        return 0;
    }
    auto& cp_ref = contact_points[contact_point_count];
    cp_ref = cp;
    contact_point_count++;
    return 1;
}
ContactPoint* CollisionWorld::getContactPointArrayEnd() {
    return &contact_points[contact_point_count];
}