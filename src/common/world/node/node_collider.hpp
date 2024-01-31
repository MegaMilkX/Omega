#pragma once

#include "node_collider.auto.hpp"
#include "world/world.hpp"

[[cppi_class]];
class ColliderNode : public gameActorNode {
public:
    TYPE_ENABLE();
    CollisionSphereShape    shape;
    ColliderProbe           collider;
    ColliderNode() {
        collider.setShape(&shape);
        collider.user_data.type = COLLIDER_USER_NODE;
        collider.user_data.user_ptr = this;
    }
    void onDefault() override {
        shape.radius = 0.25f;
        collider.setShape(&shape);
    }
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
