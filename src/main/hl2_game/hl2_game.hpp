#pragma once

#include "hl2_game.auto.hpp"

#include "game/game_base.hpp"

#include "world/player_agent_actor.hpp"
#include "controllers/fps_character_controller.hpp"
#include "experimental/hl2/hl2_bsp.hpp"


[[cppi_class]];
class HL2Game : public GameBase {
    PlayerAgentActor fps_player_actor;
    hl2Scene hl2scene;

    InputContext input_ctx = InputContext("HL2Game");
    InputAction* inputRecover = 0;
    InputAction* inputStepPhysics = 0;
    InputAction* inputRunPhysics = 0;
    InputAction* inputNumButtons[9];

public:
    TYPE_ENABLE();

    void onInit() override;
    void onCleanup() override;

    void onUpdate(float dt) override;
    void onDraw(float dt) override;
};