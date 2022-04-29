#pragma once

#include <assert.h>
#include <vector>
#include <unordered_map>
#include "common/math/gfxm.hpp"
#include "common/collision/intersection/boxbox.hpp"
#include "common/collision/intersection/sphere_box.hpp"

class CollisionDebugDrawCallbackInterface {
public:
    virtual ~CollisionDebugDrawCallbackInterface() {}

    virtual void onDrawLines(const gfxm::vec3* vertices, int vertex_count, const gfxm::vec4& color) {

    }
};

#include "common/collision/collider.hpp"

constexpr int MAX_CONTACT_POINTS = 2048;

class CollisionWorld {
    CollisionDebugDrawCallbackInterface* debug_draw = 0;
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
    void addCollider(Collider* collider) {
        colliders.push_back(collider);
        if (collider->getType() == COLLIDER_TYPE::PROBE) {
            probes.push_back((ColliderProbe*)collider);
        }
    }
    void removeCollider(Collider* collider) {
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

    void setDebugDrawInterface(CollisionDebugDrawCallbackInterface* iface) {
        debug_draw = iface;
    }

    void debugDraw() {
        assert(debug_draw);
        if (!debug_draw) {
            return;
        }

        auto drawSphere = [this](float radius, const gfxm::mat4& transform, const gfxm::vec4& color) {
            std::vector<gfxm::vec3> verts;
            const int seg = 5;
            const int segments = seg * 2 + 2;
            const int segments_h = seg + 1;
            const float s = 1.0f / (float)segments;
            const float s_h = 1.0f / (float)segments_h;
            const float s_half = 1.0f / ((float)(segments / 2));
            for (int j = 1; j < segments_h; ++j) {
                float h = cosf(j * s_h * gfxm::pi) * radius;
                float w = sinf(j * s_h * gfxm::pi) * radius;
                for (int i = 0; i < segments; ++i) {
                    float a = i * s * 2 * gfxm::pi;
                    float b = (i + 1) * s * 2 * gfxm::pi;
                    gfxm::vec3 v_a(sinf(a) * w, h, cosf(a) * w);
                    gfxm::vec3 v_b(sinf(b) * w, h, cosf(b) * w);
                    verts.push_back(v_a);
                    verts.push_back(v_b);
                }
            }
            for (int i = 0; i < segments / 2; ++i) {
                for (int j = 0; j < segments; ++j) {
                    float a = j * s * 2 * gfxm::pi;
                    float b = (j + 1) * s * 2 * gfxm::pi;
                    gfxm::vec3 v_a(sinf(a) * radius, cosf(a) * radius, 0);
                    gfxm::vec3 v_b(sinf(b) * radius, cosf(b) * radius, 0);
                    gfxm::quat q = gfxm::angle_axis(i * s_half * gfxm::pi, gfxm::vec3(0, 1, 0));
                    gfxm::mat4 rotation = gfxm::to_mat4(q);
                    verts.push_back(rotation * v_a);
                    verts.push_back(rotation * v_b);
                }
            }
            for (int i = 0; i < verts.size(); ++i) {
                verts[i] = transform * gfxm::vec4(verts[i], 1.0f);
            }
            debug_draw->onDrawLines(verts.data(), verts.size(), color);
        };
        auto drawBox = [this](const gfxm::vec3& half_extents, const gfxm::mat4& transform, const gfxm::vec4& color) {
            auto& e = half_extents;
            gfxm::vec3 verts[] = {
                gfxm::vec3(-e.x, -e.y, -e.z),
                gfxm::vec3(e.x, -e.y, -e.z),
                gfxm::vec3(-e.x, -e.y, e.z),
                gfxm::vec3(e.x, -e.y, e.z),
                gfxm::vec3(-e.x, e.y, -e.z),
                gfxm::vec3(e.x, e.y, -e.z),
                gfxm::vec3(-e.x, e.y, e.z),
                gfxm::vec3(e.x, e.y, e.z),

                gfxm::vec3(-e.x, -e.y, -e.z),
                gfxm::vec3(-e.x, -e.y, e.z),
                gfxm::vec3(e.x, -e.y, -e.z),
                gfxm::vec3(e.x, -e.y, e.z),
                gfxm::vec3(-e.x, e.y, -e.z),
                gfxm::vec3(-e.x, e.y, e.z),
                gfxm::vec3(e.x, e.y, -e.z),
                gfxm::vec3(e.x, e.y, e.z),

                gfxm::vec3(-e.x, -e.y, -e.z),
                gfxm::vec3(-e.x, e.y, -e.z),
                gfxm::vec3(e.x, -e.y, -e.z),
                gfxm::vec3(e.x, e.y, -e.z),
                gfxm::vec3(-e.x, -e.y, e.z),
                gfxm::vec3(-e.x, e.y, e.z),
                gfxm::vec3(e.x, -e.y, e.z),
                gfxm::vec3(e.x, e.y, e.z)
            };

            for (int i = 0; i < sizeof(verts) / sizeof(verts[0]); ++i) {
                verts[i] = transform * gfxm::vec4(verts[i], 1.0f);
            }
            debug_draw->onDrawLines(verts, sizeof(verts) / sizeof(verts[0]), color);
        };
        auto drawAxes = [this](const gfxm::mat4& transform) {
            gfxm::vec3 axis_verts[] = {
                gfxm::vec3(0,0,0), gfxm::vec3(1,0,0),
                gfxm::vec3(0,0,0), gfxm::vec3(0,1,0),
                gfxm::vec3(0,0,0), gfxm::vec3(0,0,1)
            };
            for (int i = 0; i < sizeof(axis_verts) / sizeof(axis_verts[0]); ++i) {
                axis_verts[i] = transform * gfxm::vec4(axis_verts[i], 1.0f);
            }
            debug_draw->onDrawLines(axis_verts, 2, gfxm::vec4(1, 0, 0, 1));
            debug_draw->onDrawLines(axis_verts + 2, 2, gfxm::vec4(0, 1, 0, 1));
            debug_draw->onDrawLines(axis_verts + 4, 2, gfxm::vec4(0, 0, 1, 1));
        };
        auto drawCross = [this](const gfxm::mat4& transform) {
            gfxm::vec3 axis_verts[] = {
                gfxm::vec3(-0.3,0,0), gfxm::vec3(0.3,0,0),
                gfxm::vec3(0,-0.3,0), gfxm::vec3(0,0.3,0),
                gfxm::vec3(0,0,-0.3), gfxm::vec3(0,0,0.3)
            };
            for (int i = 0; i < sizeof(axis_verts) / sizeof(axis_verts[0]); ++i) {
                axis_verts[i] = transform * gfxm::vec4(axis_verts[i], 1.0f);
            }
            debug_draw->onDrawLines(axis_verts, 2, gfxm::vec4(1, 0, 0, 1));
            debug_draw->onDrawLines(axis_verts + 2, 2, gfxm::vec4(1, 0, 0, 1));
            debug_draw->onDrawLines(axis_verts + 4, 2, gfxm::vec4(1, 0, 0, 1));
        };
        auto drawCapsule = [this](float height, float radius, const gfxm::mat4& transform, const gfxm::vec4& color) {
            std::vector<gfxm::vec3> verts;
            const int seg = 5;
            const int segments = seg * 2 + 2;
            const int segments_h = seg + 1;
            const float s = 1.0f / (float)segments;
            const float s_h = 1.0f / (float)segments_h;
            const float s_half = 1.0f / ((float)(segments / 2));
            
            // Upper half sphere
            for (int j = 1; j < segments_h / 2 + 1; ++j) {
                float h = cosf(j * s_h * gfxm::pi) * radius + height * 0.5f;
                float w = sinf(j * s_h * gfxm::pi) * radius;
                for (int i = 0; i < segments; ++i) {
                    float a = i * s * 2 * gfxm::pi;
                    float b = (i + 1) * s * 2 * gfxm::pi;
                    gfxm::vec3 v_a(sinf(a) * w, h, cosf(a) * w);
                    gfxm::vec3 v_b(sinf(b) * w, h, cosf(b) * w);
                    verts.push_back(v_a);
                    verts.push_back(v_b);
                }
            }
            for (int i = 0; i < segments / 2; ++i) {
                for (int j = 0; j < segments; ++j) {
                    float a = j * s * gfxm::pi - (gfxm::pi * 0.5f);
                    float b = (j + 1) * s * gfxm::pi - (gfxm::pi * 0.5f);
                    gfxm::vec3 v_a(sinf(a) * radius, cosf(a) * radius + height * 0.5f, 0);
                    gfxm::vec3 v_b(sinf(b) * radius, cosf(b) * radius + height * 0.5f, 0);
                    gfxm::quat q = gfxm::angle_axis(i * s_half * gfxm::pi, gfxm::vec3(0, 1, 0));
                    gfxm::mat4 rotation = gfxm::to_mat4(q);
                    verts.push_back(rotation * v_a);
                    verts.push_back(rotation * v_b);
                }
            }
            // Lower half sphere
            for (int j = segments_h / 2; j < segments_h; ++j) {
                float h = cosf(j * (s_h)* gfxm::pi) * radius - height * 0.5f;
                float w = sinf(j * (s_h)* gfxm::pi) * radius;
                for (int i = 0; i < segments; ++i) {
                    float a = i * s * 2 * gfxm::pi;
                    float b = (i + 1) * s * 2 * gfxm::pi;
                    gfxm::vec3 v_a(sinf(a) * w, h, cosf(a) * w);
                    gfxm::vec3 v_b(sinf(b) * w, h, cosf(b) * w);
                    verts.push_back(v_a);
                    verts.push_back(v_b);
                }
            }
            for (int i = 0; i < segments / 2; ++i) {
                for (int j = 0; j < segments; ++j) {
                    float a = j * s * gfxm::pi + (gfxm::pi * 0.5f);
                    float b = (j + 1) * s * gfxm::pi + (gfxm::pi * 0.5f);
                    gfxm::vec3 v_a(sinf(a) * radius, cosf(a) * radius - height * 0.5f, 0);
                    gfxm::vec3 v_b(sinf(b) * radius, cosf(b) * radius - height * 0.5f, 0);
                    gfxm::quat q = gfxm::angle_axis(i * s_half * gfxm::pi, gfxm::vec3(0, 1, 0));
                    gfxm::mat4 rotation = gfxm::to_mat4(q);
                    verts.push_back(rotation * v_a);
                    verts.push_back(rotation * v_b);
                }
            }

            float h = .0f;
            float w = radius;
            for (int i = 0; i < segments; ++i) {
                float a = i * s * 2 * gfxm::pi;
                float s = sinf(a);
                float c = cosf(a);
                gfxm::vec3 v_a(s * w, -height * 0.5f, c * w);
                gfxm::vec3 v_b(s * w,  height * 0.5f, c * w);
                verts.push_back(v_a);
                verts.push_back(v_b);
            }

            for (int i = 0; i < verts.size(); ++i) {
                verts[i] = transform * gfxm::vec4(verts[i], 1.0f);
            }
            debug_draw->onDrawLines(verts.data(), verts.size(), color);
        };

        for (int k = 0; k < colliders.size(); ++k) {
            auto& col = colliders[k];
            assert(col->getShape());
            auto shape = col->getShape();
            gfxm::mat4 transform = gfxm::translate(gfxm::mat4(1.0f), col->position)
                * gfxm::to_mat4(col->rotation);

            if (shape->getShapeType() == COLLISION_SHAPE_TYPE::SPHERE) {
                auto shape_sphere = (const CollisionSphereShape*)shape;
                drawSphere(shape_sphere->radius, transform, gfxm::vec4(1, 1, 1, 1));
            } else if(shape->getShapeType() == COLLISION_SHAPE_TYPE::BOX) {
                auto shape_box = (const CollisionBoxShape*)shape;
                auto& e = shape_box->half_extents;
                drawBox(shape_box->half_extents, transform, gfxm::vec4(1, 1, 1, 1));
            } else if(shape->getShapeType() == COLLISION_SHAPE_TYPE::CAPSULE) {
                auto shape_capsule = (const CollisionCapsuleShape*)shape;
                auto height = shape_capsule->height;
                auto radius = shape_capsule->radius;
                drawCapsule(height, radius, transform, gfxm::vec4(1, 1, 1, 1));
            }
        }

        for (int i = 0; i < manifolds.size(); ++i) {
            auto& m = manifolds[i];
            for (int j = 0; j < m.point_count; ++j) {
                drawCross(gfxm::translate(gfxm::mat4(1.0f), m.points[j].position));
            }
        }
    }

    void update() {
        // Clear temporary per-frame data
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
        
    }

    // 
    int addContactPoint(const gfxm::vec3& pos, const gfxm::vec3& normal, float depth) {
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
    ContactPoint* getContactPointArrayEnd() {
        return &contact_points[contact_point_count];
    }
};
