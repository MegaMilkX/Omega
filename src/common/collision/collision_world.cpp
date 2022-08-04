#include "collision_world.hpp"


void CollisionWorld::addCollider(Collider* collider) {
    colliders.push_back(collider);
    if (collider->getType() == COLLIDER_TYPE::PROBE) {
        probes.push_back((ColliderProbe*)collider);
    }
}
void CollisionWorld::removeCollider(Collider* collider) {
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

void CollisionWorld::castSphere(const gfxm::mat4& tr, float radius) {
    dbgDrawSphere(tr, radius, DBG_COLOR_RED);
    // TODO
}

void CollisionWorld::debugDraw() {
    for (int k = 0; k < colliders.size(); ++k) {
        auto& col = colliders[k];
        assert(col->getShape());
        auto shape = col->getShape();
        gfxm::mat4 transform = gfxm::translate(gfxm::mat4(1.0f), col->position)
            * gfxm::to_mat4(col->rotation);

        if (shape->getShapeType() == COLLISION_SHAPE_TYPE::SPHERE) {
            auto shape_sphere = (const CollisionSphereShape*)shape;
            dbgDrawSphere(transform, shape_sphere->radius, 0xFFFFFFFF);
        } else if(shape->getShapeType() == COLLISION_SHAPE_TYPE::BOX) {
            auto shape_box = (const CollisionBoxShape*)shape;
            auto& e = shape_box->half_extents;
            dbgDrawBox(transform, shape_box->half_extents, 0xFFFFFFFF);
        } else if(shape->getShapeType() == COLLISION_SHAPE_TYPE::CAPSULE) {
            auto shape_capsule = (const CollisionCapsuleShape*)shape;
            auto height = shape_capsule->height;
            auto radius = shape_capsule->radius;
            dbgDrawCapsule(transform, height, radius, 0xFFFFFFFF);
        }
    }

    for (int i = 0; i < manifolds.size(); ++i) {
        auto& m = manifolds[i];
        for (int j = 0; j < m.point_count; ++j) {
            dbgDrawArrow(m.points[j].position, m.points[j].normal, 0xFF0000FF);
            //dbgDrawCross(gfxm::translate(gfxm::mat4(1.0f), m.points[j].position), .5f, 0xFF0000FF);
        }
    }
}

void CollisionWorld::update() {
    // Clear per-frame data
    manifolds.clear();
    clearContactPoints();
    for (auto& it : potential_pairs) {
        it.second.clear();
    }
    for (int i = 0; i < probes.size(); ++i) {
        probes[i]->_clearOverlappingColliders();
    }

    // Determine potential collisions
    for (int i = 0; i < colliders.size(); ++i) {
        for (int j = i + 1; j < colliders.size(); ++j) {
            auto a = colliders[i];
            auto b = colliders[j];
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
        gfxm::vec3 vec_a = b->position - a->position;
        float center_distance = gfxm::length(vec_a);
        float radius_distance = sa->radius + sb->radius;
        float distance = center_distance - radius_distance;
        if (distance <= FLT_EPSILON) {
            gfxm::vec3 normal_a = vec_a / center_distance;
            gfxm::vec3 normal_b = -normal_a;
            gfxm::vec3 pt_a = normal_a * sa->radius + a->position;
            gfxm::vec3 pt_b = normal_b * sb->radius + b->position;

            ContactPoint* cp_ptr = &contact_points[contact_point_count];
            int cp_count = 1;
            addContactPoint((pt_a + pt_b) * 0.5f, normal_a, distance);

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

        CollisionManifold manifold;
        btVector3 p1;
        btVector3 p2;
        btVector3 bt_side1;
        btVector3 bt_side2;
        p1.setValue(a->position.x, a->position.y, a->position.z);
        p2.setValue(b->position.x, b->position.y, b->position.z);
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
        gfxm::mat4 box_transform = 
            gfxm::translate(gfxm::mat4(1.0f), b->position)
            * gfxm::to_mat4(b->rotation);
        gfxm::vec3 point_on_box;
        gfxm::vec3 normal;
        float distance;
        if (intersectionSphereBox(sa->radius, a->position, sb->half_extents, box_transform, point_on_box, normal, distance)) {
            ContactPoint* cp_ptr = &contact_points[contact_point_count];
            int cp_num = addContactPoint(point_on_box, normal, distance);
            CollisionManifold manifold;
            manifold.collider_a = a;
            manifold.collider_b = b;
            manifold.points = cp_ptr;
            manifold.point_count = cp_num;
            manifold.normal = normal;
            manifold.depth = distance;
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
        //m.collider_a->position += m.normal * m.depth;
    }
        
    debugDraw();
}

int CollisionWorld::addContactPoint(const gfxm::vec3& pos, const gfxm::vec3& normal, float depth) {
    assert(contact_point_count < MAX_CONTACT_POINTS);
    if (contact_point_count >= MAX_CONTACT_POINTS) {
        return 0;
    }
    auto& cp = contact_points[contact_point_count];
    cp.position = pos;
    cp.normal = normal;
    cp.depth = depth;
    contact_point_count++;
    return 1;
}
ContactPoint* CollisionWorld::getContactPointArrayEnd() {
    return &contact_points[contact_point_count];
}