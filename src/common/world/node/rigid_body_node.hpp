#pragma once

#include "rigid_body_node.auto.hpp"
#include "world/world.hpp"
#include "collision/collision_world.hpp"

#include "collider/collider.hpp"


[[cppi_class]];
class RigidBodyNode : public TActorNode<phyWorld> {
    ResourceRef<Collider>   collider;
    phyRigidBody            body;

public:
    TYPE_ENABLE();

    [[cppi_decl]]
    bool sleep_on_spawn = true;

    RigidBodyNode() {
        body.user_data.type = COLLIDER_USER_NODE;
        body.user_data.user_ptr = this;
        
        body.mass = 1.f;
        body.friction = .6f;
        body.collision_group = COLLISION_LAYER_DEFAULT;
        
        getTransformHandle()->addDirtyCallback([](void* ctx) {
            RigidBodyNode* node = (RigidBodyNode*)ctx;
            node->body.markAsExternallyTransformed();
        }, this);
        getTransformHandle()->setInheritFlags(FTransformInherit::None);
    }

    [[cppi_decl, get("collider")]]
    const ResourceRef<Collider>& getCollider() const { return collider; }
    [[cppi_decl, set("collider")]]
    void setCollider(const ResourceRef<Collider>& col) {
        if(col) {
            body.setShape(col->getShape());
        } else {
            body.setShape(nullptr);
        }
        collider = col;
        requestRebuild();
    }

    uint64_t getGroups() const { return body.collision_group; }
    void setGroups(uint64_t groups) { body.collision_group = groups; }
    void addGroups(uint64_t groups) { body.collision_group |= groups; }

    uint64_t getMask() const { return body.collision_mask; }
    void setMask(uint64_t mask) { body.collision_mask = mask; }
    void addMask(uint64_t mask) { body.collision_mask |= mask; }

    void setFlags(int flags) { body.setFlags(flags); }

    [[cppi_decl, get("mass")]]
    float getMass() const { return body.mass; }
    [[cppi_decl, set("mass")]]
    void setMass(float m) { body.mass = m; }

    [[cppi_decl, get("mass_center")]]
    const gfxm::vec3& getMassCenter() const { return body.mass_center; }
    [[cppi_decl, set("mass_center")]]
    void setMassCenter(const gfxm::vec3& mc) { body.mass_center = mc; }

    [[cppi_decl, get("friction")]]
    float getFriction() const { return body.friction; }
    [[cppi_decl, set("friction")]]
    void setFriction(float f) { body.friction = f; }

    const gfxm::vec3& getVelocity() const { return body.velocity; }
    void setVelocity(const gfxm::vec3& v) { body.velocity = v; }

    const gfxm::vec3& getAngularVelocity() const { return body.angular_velocity; }
    void setAngularVelocity(const gfxm::vec3& av) { body.angular_velocity = av; }

    void impulseAtPoint(const gfxm::vec3& impulse, const gfxm::vec3& point) {
        body.impulseAtPoint(impulse, point);
    }

    void wake() { body.is_sleeping = false; }

    // These should not be used normally
    void _setBodyPosition(const gfxm::vec3& p) { body.setPosition(p); }
    void _setBodyRotation(const gfxm::quat& q) { body.setRotation(q); }
    phyRigidBody* _getBody() { return &body; }

    void onDefault() override {}
    void onSpawnActorNode(phyWorld* world) override {
        if (!body.getShape()) {
            return;
        }
        world->addCollider(&body);
        body.markAsExternallyTransformed();
        body.is_sleeping = sleep_on_spawn;
    }
    void onDespawnActorNode(phyWorld* world) override {
        world->removeCollider(&body);
    }
};

