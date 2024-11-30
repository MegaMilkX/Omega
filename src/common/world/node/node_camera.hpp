#pragma once

#include "node_camera.auto.hpp"
#include "actor_node.hpp"
#include "reflection/reflection.hpp"
#include "math/gfxm.hpp"

class RuntimeWorld;

[[cppi_class]];
class CameraNode : public ActorNode {
public:
    TYPE_ENABLE();
    gfxm::mat4 projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
    bool set_as_current_on_spawn = true;

    void onDefault() override {}
    void onUpdateTransform() override {}
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(RuntimeWorld* world) override;
    void onDespawn(RuntimeWorld* world) override;
};