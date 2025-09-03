#include "node_camera.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    type_register<CameraNode>("CameraNode")
        .parent<ActorNode>();
};


void CameraNode::onSpawn(WorldSystemRegistry* reg) {

}
void CameraNode::onDespawn(WorldSystemRegistry* reg) {

}