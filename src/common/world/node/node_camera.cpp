#include "node_camera.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    type_register<CameraNode>("CameraNode")
        .parent<gameActorNode>();
};


void CameraNode::onSpawn(GameWorld* world) {

}
void CameraNode::onDespawn(GameWorld* world) {

}