#pragma once

#include "node_character_capsule.auto.hpp"
#include "world/world.hpp"
#include "collision/collision_world.hpp"
#include "collision/shape/capsule.hpp"


[[cppi_class]];
class CharacterCapsuleNode : public TActorNode<phyWorld> {
public:
    TYPE_ENABLE();
    phyCapsuleShape   shape;
    phyRigidBody                collider;

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
            NodeSlotDesc{ type_get<TestDummyLinkData>(), LINK_WRITE, eSlotDownstream }
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
    
    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j) override {
        type_write_json(j["offset"], collider.getCenterOffset());

        type_write_json(j["height"], shape.height);
        type_write_json(j["radius"], shape.radius);
    }
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j) override {
        gfxm::vec3 offset;
        type_read_json(j["offset"], offset);
        collider.setCenterOffset(offset);

        type_read_json(j["height"], shape.height);
        type_read_json(j["radius"], shape.radius);
        return true;
    }
};
