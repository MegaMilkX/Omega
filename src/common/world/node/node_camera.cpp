#include "node_camera.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    type_register<CameraNode>("CameraNode")
        .parent<gameActorNode>();
};


void CameraNode::onSpawn(GameWorld* world) {
    if (set_as_current_on_spawn) {
        world->setCurrentCameraNode(this);
    }
}
void CameraNode::onDespawn(GameWorld* world) {
    if (this == world->getCurrentCameraNode()) {
        world->setCurrentCameraNode(0);
    }
}