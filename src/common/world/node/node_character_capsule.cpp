#include "node_character_capsule.hpp"


STATIC_BLOCK {
    type_register<CharacterCapsuleNode>("CharacterCapsuleNode")
        .parent<gameActorNode>();
};