#include "node_probe.hpp"


ProbeNode::ProbeNode() {
    collider.setShape(&shape);
    collider.user_data.type = COLLIDER_USER_NODE;
    collider.user_data.user_ptr = this;
    
    getTransformHandle()->addDirtyCallback([](void* ctx) {
        ProbeNode* node = (ProbeNode*)ctx;
        node->collider.markAsExternallyTransformed();
    }, this);
}

void ProbeNode::onDefault() {
    shape.radius = 0.25f;
    collider.setShape(&shape);
}

void ProbeNode::onSpawnActorNode(phyWorld* world) {
    world->addCollider(&collider);
    collider.markAsExternallyTransformed();
}

void ProbeNode::onDespawnActorNode(phyWorld* world) {
    world->removeCollider(&collider);
}
