#include "actor_node.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    rtti::type_register<EmptyNode>("EmptyNode")
        .parent<ActorNode>();
};

