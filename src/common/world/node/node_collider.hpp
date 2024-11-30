#pragma once

#include "node_collider.auto.hpp"
#include "world/world.hpp"

[[cppi_class]];
class ColliderNode : public ActorNode {
public:
    TYPE_ENABLE();
    CollisionSphereShape    shape;
    ColliderProbe           collider;
    ColliderNode() {
        collider.setShape(&shape);
        collider.user_data.type = COLLIDER_USER_NODE;
        collider.user_data.user_ptr = this;
    }
    void onDefault() override;
    void onUpdateTransform() override {
        collider.setPosition(getWorldTranslation());
        collider.setRotation(getWorldRotation());
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(RuntimeWorld* world) override {
        world->getCollisionWorld()->addCollider(&collider);
    }
    void onDespawn(RuntimeWorld* world) override {
        world->getCollisionWorld()->removeCollider(&collider);
    }
};
