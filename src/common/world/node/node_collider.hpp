#pragma once

#include "node_collider.auto.hpp"
#include "world/world.hpp"
#include "collision/collision_world.hpp"
#include "collision/shape/sphere.hpp"


[[cppi_class]];
class ColliderNode : public TActorNode<phyWorld> {
public:
    TYPE_ENABLE();
    phySphereShape    shape;
    phyProbe           collider;
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
    void onSpawnActorNode(phyWorld* world) override {
        world->addCollider(&collider);
        collider.markAsExternallyTransformed();
    }
    void onDespawnActorNode(phyWorld* world) override {
        world->removeCollider(&collider);
    }
};
