#include "actor_node.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    type_register<EmptyNode>("EmptyNode")
        .parent<ActorNode>();
};

void ActorNode::_registerGraphWorld(RuntimeWorld* world) {
    world->_registerNode(this);
    for (auto& c : children) {
        c->_registerGraphWorld(world);
    }
}
void ActorNode::_unregisterGraphWorld(RuntimeWorld* world) {
    for (auto& c : children) {
        c->_unregisterGraphWorld(world);
    }
    world->_unregisterNode(this);
}