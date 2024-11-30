#include "node_camera.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    type_register<CameraNode>("CameraNode")
        .parent<ActorNode>();
};


void CameraNode::onSpawn(RuntimeWorld* world) {

}
void CameraNode::onDespawn(RuntimeWorld* world) {

}