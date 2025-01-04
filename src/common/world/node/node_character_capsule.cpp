#include "node_character_capsule.hpp"


STATIC_BLOCK {
    type_register<CharacterCapsuleNode>("CharacterCapsuleNode")
        .parent<ActorNode>();
};


void CharacterCapsuleNode::onDefault() {
    shape.radius = .3f;
    shape.height = 0.8f;
    collider.setCenterOffset(gfxm::vec3(.0f, shape.height * .5f + shape.radius + .20f, .0f));
    collider.setShape(&shape);
    collider.collision_group
        = COLLISION_LAYER_CHARACTER;
    collider.collision_mask
        = COLLISION_LAYER_DEFAULT
        | COLLISION_LAYER_PROBE
        | COLLISION_LAYER_CHARACTER;
}