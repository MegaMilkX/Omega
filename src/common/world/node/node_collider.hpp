#pragma once

#include "world/world.hpp"

class nodeCollider : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
public:
    CollisionSphereShape    shape;
    ColliderProbe           collider;
    nodeCollider() {
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
    void onUpdate(gameWorld* world, float dt) override {}
    void onSpawn(gameWorld* world) override {
        world->getCollisionWorld()->addCollider(&collider);
    }
    void onDespawn(gameWorld* world) override {
        world->getCollisionWorld()->removeCollider(&collider);
    }
};
STATIC_BLOCK{
    type_register<nodeCollider>("nodeCollider")
        .parent<gameActorNode>();
};