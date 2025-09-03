#pragma once

#include "node_collider.auto.hpp"
#include "world/world.hpp"
#include "collision/collision_world.hpp"


[[cppi_class]];
class ColliderNode : public TActorNode<CollisionWorld> {
public:
    TYPE_ENABLE();
    CollisionSphereShape    shape;
    ColliderProbe           collider;
    ColliderNode() {
        collider.setShape(&shape);
        collider.user_data.type = COLLIDER_USER_NODE;
        collider.user_data.user_ptr = this;
        
        getTransformHandle()->addDirtyCallback([](void* ctx) {
            ColliderNode* node = (ColliderNode*)ctx;
            node->collider.markAsExternallyTransformed();
        }, this);
    }
    void onDefault() override;
    void onUpdateTransform() override {
        collider.setPosition(getWorldTranslation());
        collider.setRotation(getWorldRotation());
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(CollisionWorld* world) override {
        world->addCollider(&collider);
        collider.markAsExternallyTransformed();
    }
    void onDespawn(CollisionWorld* world) override {
        world->removeCollider(&collider);
    }
};
