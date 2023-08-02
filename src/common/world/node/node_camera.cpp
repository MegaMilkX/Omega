#include "node_camera.hpp"

#include "world/world.hpp"

void CameraNode::onSpawn(gameWorld* world) {
    if (set_as_current_on_spawn) {
        world->setCurrentCameraNode(this);
    }
}
void CameraNode::onDespawn(gameWorld* world) {
    if (this == world->getCurrentCameraNode()) {
        world->setCurrentCameraNode(0);
    }
}