#include "node_collider.hpp"


STATIC_BLOCK {
    type_register<ColliderNode>("ColliderNode")
        .parent<gameActorNode>();
};