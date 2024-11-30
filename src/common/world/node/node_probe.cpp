#include "node_probe.hpp"


ProbeNode::ProbeNode() {
    collider.setShape(&shape);
    collider.user_data.type = COLLIDER_USER_NODE;
    collider.user_data.user_ptr = this;
}

void ProbeNode::onDefault() {
    shape.radius = 0.25f;
    collider.setShape(&shape);

    getTransformHandle()->addDirtyCallback([](void* ctx) {
        ProbeNode* node = (ProbeNode*)ctx;
        node->collider.markAsExternallyTransformed();
    }, this);
}

void ProbeNode::onUpdateTransform() {
    //collider.setPosition(getWorldTranslation());
    //collider.setRotation(getWorldRotation());
}

void ProbeNode::onUpdate(RuntimeWorld* world, float dt) {

}

void ProbeNode::onSpawn(RuntimeWorld* world) {
    world->getCollisionWorld()->addCollider(&collider);
}

void ProbeNode::onDespawn(RuntimeWorld* world) {
    world->getCollisionWorld()->removeCollider(&collider);
}
