#include "actor_node.hpp"

#include "game/world/world.hpp"

void gameActorNode::_registerGraphWorld(gameWorld* world) {
    world->_registerNode(this);
    for (auto& c : children) {
        c->_registerGraphWorld(world);
    }
}
void gameActorNode::_unregisterGraphWorld(gameWorld* world) {
    for (auto& c : children) {
        c->_unregisterGraphWorld(world);
    }
    world->_unregisterNode(this);
}