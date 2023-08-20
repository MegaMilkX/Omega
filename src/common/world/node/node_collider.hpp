#pragma once

#include "world/world.hpp"

class ColliderNode : public gameActorNode {
    TYPE_ENABLE();
public:
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
    void onUpdate(GameWorld* world, float dt) override {}
    void onSpawn(GameWorld* world) override {
        world->getCollisionWorld()->addCollider(&collider);
    }
    void onDespawn(GameWorld* world) override {
        world->getCollisionWorld()->removeCollider(&collider);
    }
};
