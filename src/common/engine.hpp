#pragma once

#include "game/game_base.hpp"
#include "viewport/viewport.hpp"
#include "player/player.hpp"


struct ENGINE_RUN_DATA {
    GameBase* game;
    Viewport* primary_viewport;
    LocalPlayer* primary_player;
};


int     engineGameInit();
void    engineGameRun(ENGINE_RUN_DATA& data);
void    engineGameCleanup();
