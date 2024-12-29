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
