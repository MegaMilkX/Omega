#pragma once

#include "hl2_game.auto.hpp"

#include "game/game_base.hpp"

#include "controllers/fps_character_controller.hpp"
#include "experimental/hl2/hl2_bsp.hpp"


[[cppi_class]];
class HL2GameInstance : public IGameInstance {
    std::unique_ptr<IWorld> world;
    std::unique_ptr<HL2Scene> scene;
    std::unique_ptr<IPlayer> primary_player;
    std::unique_ptr<EngineRenderView> primary_view;

    Actor fps_player_actor;

    InputContext input_ctx = InputContext("HL2Game");
    InputAction* inputRecover = 0;
    InputAction* inputToggleWireframe;
    InputAction* inputStepPhysics = 0;
    InputAction* inputRunPhysics = 0;
    InputAction* inputFButtons[12];
    InputAction* inputNumButtons[9];

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