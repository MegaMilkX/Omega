#pragma once

#include "node_character_capsule.auto.hpp"
#include "world/world.hpp"
#include "collision/collision_world.hpp"
#include "collision/shape/capsule.hpp"


[[cppi_class]];
class CharacterCapsuleNode : public TActorNode<phyWorld> {
public:
    TYPE_ENABLE();
    phyCapsuleShape shape;
    phyRigidBody    collider;

    CharacterCapsuleNode() {
        collider.setShape(&shape);
        collider.user_data.type = COLLIDER_USER_NODE;
        collider.user_data.user_ptr = this;
        
        getTransformHandle()->addDirtyCallback([](void* ctx) {
            CharacterCapsuleNode* node = (CharacterCapsuleNode*)ctx;
            node->collider.markAsExternallyTransformed();
        }, this);
    }
    
    // FOR TESTING
    const NodeSlotDescArray& getSlots() override {
        static NodeSlotDescArray slots = {
            NodeSlotDesc{ rtti::type_get<TestDummyLinkData>(), LINK_WRITE, eSlotDownstream }
        };
        return slots;
    }
    // ===========

    void onDefault() override;
    void onSpawnActorNode(phyWorld* world) override {
        world->addCollider(&collider);
        collider.markAsExternallyTransformed();
    }
    void onDespawnActorNode(phyWorld* world) override {
        world->removeCollider(&collider);
    }
};
