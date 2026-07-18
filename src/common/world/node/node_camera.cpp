#include "node_camera.hpp"

#include "world/world.hpp"

STATIC_BLOCK {
    rtti::type_register<CameraNode>("CameraNode")
        .parent<ActorNode>();
};

