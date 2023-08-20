#pragma once

#include "actor_node.hpp"
#include "reflection/reflection.hpp"
#include "math/gfxm.hpp"

class GameWorld;


class CameraNode : public gameActorNode {
    TYPE_ENABLE();
public:
    gfxm::mat4 projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
    bool set_as_current_on_spawn = true;

    void onDefault() override {}
    void onUpdateTransform() override {}
    void onUpdate(GameWorld* world, float dt) override {}
    void onSpawn(GameWorld* world) override;
    void onDespawn(GameWorld* world) override;
};