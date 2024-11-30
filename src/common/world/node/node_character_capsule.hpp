#pragma once

#include "node_character_capsule.auto.hpp"
#include "world/world.hpp"


[[cppi_class]];
class CharacterCapsuleNode : public ActorNode {
public:
    TYPE_ENABLE();
    CollisionCapsuleShape   shape;
    Collider                collider;

    CharacterCapsuleNode() {
        collider.setShape(&shape);
        collider.user_data.type = COLLIDER_USER_NODE;
        collider.user_data.user_ptr = this;
    }
    void onDefault() override;
    void onUpdateTransform() override {
        collider.setPosition(getWorldTranslation());
        collider.setRotation(getWorldRotation());
    }
    void onSpawn(RuntimeWorld* world) override {
        world->getCollisionWorld()->addCollider(&collider);
    }
    void onDespawn(RuntimeWorld* world) override {
        world->getCollisionWorld()->removeCollider(&collider);
    }
};
