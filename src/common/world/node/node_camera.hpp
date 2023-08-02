#pragma once

#include "actor_node.hpp"
#include "reflection/reflection.hpp"
#include "math/gfxm.hpp"

class gameWorld;


class CameraNode : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
public:
    gfxm::mat4 projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
    bool set_as_current_on_spawn = true;

    void onDefault() override {}
    void onUpdateTransform() override {}
    void onUpdate(gameWorld* world, float dt) override {}
    void onSpawn(gameWorld* world) override;
    void onDespawn(gameWorld* world) override;
};
STATIC_BLOCK{
    type_register<CameraNode>("CameraNode")
        .parent<gameActorNode>();
};
