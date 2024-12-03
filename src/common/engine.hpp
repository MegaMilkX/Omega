#pragma once

#include "game/game_base.hpp"
#include "viewport/viewport.hpp"
#include "player/player.hpp"


struct ENGINE_INIT_DATA {
    GameBase* game;
    Viewport* primary_viewport;
    LocalPlayer* primary_player;
};

struct ENGINE_STATS {
    float frame_time_no_vsync;
    float frame_time;
    float render_time;
    float collision_time;
    float fps;
};


int     engineGameInit();
void    engineGameCleanup();

void    engineGameRun(ENGINE_INIT_DATA& data);

ENGINE_STATS& engineGetStats();