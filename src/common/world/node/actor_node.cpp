#include "actor_node.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    type_register<EmptyNode>("EmptyNode")
        .parent<gameActorNode>();
};

void gameActorNode::_registerGraphWorld(GameWorld* world) {
    world->_registerNode(this);
    for (auto& c : children) {
        c->_registerGraphWorld(world);
    }
}
void gameActorNode::_unregisterGraphWorld(GameWorld* world) {
    for (auto& c : children) {
        c->_unregisterGraphWorld(world);
    }
    world->_unregisterNode(this);
}