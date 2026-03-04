#include "actor_node.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    type_register<EmptyNode>("EmptyNode")
        .parent<ActorNode>();
};

