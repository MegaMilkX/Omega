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

void ProbeNode::onUpdateTransform() {
    //collider.setPosition(getWorldTranslation());
    //collider.setRotation(getWorldRotation());
}

void ProbeNode::onUpdate(RuntimeWorld* world, float dt) {

}

void ProbeNode::onSpawn(CollisionWorld* world) {
    world->addCollider(&collider);
    collider.markAsExternallyTransformed();
}

void ProbeNode::onDespawn(CollisionWorld* world) {
    world->removeCollider(&collider);
}
