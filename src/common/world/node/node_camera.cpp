#include "node_camera.hpp"

#include "world/world.hpp"

void nodeCamera::onSpawn(gameWorld* world) {
    if (set_as_current_on_spawn) {
        world->setCurrentCameraNode(this);
    }
}
void nodeCamera::onDespawn(gameWorld* world) {
    if (this == world->getCurrentCameraNode()) {
        world->setCurrentCameraNode(0);
    }
}