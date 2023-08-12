#pragma once

#include "world/world.hpp"


class CharacterCapsuleNode : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
public:
    CollisionCapsuleShape   shape;
    Collider                collider;

    CharacterCapsuleNode() {
        collider.setShape(&shape);
        collider.user_data.type = COLLIDER_USER_NODE;
        collider.user_data.user_ptr = this;
    }
    void onDefault() override {
        shape.radius = .3f;
        shape.height = 1.f;
        collider.setCenterOffset(gfxm::vec3(.0f, shape.height * .5f + shape.radius + .20f, .0f));
        collider.setShape(&shape);
        collider.collision_group
            = COLLISION_LAYER_CHARACTER;
        collider.collision_mask
            = COLLISION_LAYER_DEFAULT
            | COLLISION_LAYER_PROBE
            | COLLISION_LAYER_CHARACTER;
    }
    void onUpdateTransform() override {
        collider.setPosition(getWorldTranslation());
        collider.setRotation(getWorldRotation());
    }
    void onSpawn(GameWorld* world) override {
        world->getCollisionWorld()->addCollider(&collider);
    }
    void onDespawn(GameWorld* world) override {
        world->getCollisionWorld()->removeCollider(&collider);
    }
};
