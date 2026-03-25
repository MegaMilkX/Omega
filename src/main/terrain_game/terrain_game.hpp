#pragma once

#include "terrain_game.auto.hpp"

#include "game/game_base.hpp"
#include "terrain_scene.hpp"
#include "controllers/fps_character_controller.hpp"

#include "common/dbg_render_target_switcher.hpp"

// m3d test
#include "m3d/m3d_project.hpp"

[[cppi_class]];
class TerrainGameInstance : public IGameInstance {
    std::unique_ptr<IWorld> world;
    std::unique_ptr<TerrainScene> scene;
    std::unique_ptr<IPlayer> primary_player;
    std::unique_ptr<EngineRenderView> primary_view;

    Actor fps_player_actor;
    Actor marble_actor;

    InputContext input_ctx = InputContext("TerrainGame");
    InputAction* inputRecover = 0;
    InputAction* inputStepPhysics = 0;
    InputAction* inputRunPhysics = 0;
    InputAction* inputSphereCast;
    InputAction* inputNumButtons[9];

    std::unique_ptr<DbgRenderTargetSwitcher> render_target_switcher;

    IWorld* getWorld() { return world.get(); }
public:
    TYPE_ENABLE();

    void onInit(IEngineRuntime* rt) override;
    void onCleanup() override;

    void onUpdate(float dt) override;
    void onDraw(float dt) override;

    void onPlayerJoined(IPlayer* player) override;
    void onPlayerLeft(IPlayer* player) override;
};

