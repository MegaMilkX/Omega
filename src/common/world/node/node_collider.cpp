#include "node_collider.hpp"


STATIC_BLOCK {
    type_register<ColliderNode>("ColliderNode")
        .parent<ActorNode>();
};


void ColliderNode::onDefault() {
    shape.radius = 0.25f;
    collider.setShape(&shape);
}