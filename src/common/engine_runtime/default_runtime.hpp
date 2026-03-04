#pragma once

#include <vector>
#include "engine_runtime.hpp"
#include "game/game_base.hpp"
#include "gui/gui.hpp"

#include "components/render_view_list.hpp"


class DefaultRuntime : public IEngineRuntime {
    struct ENGINE_STATS {
        float frame_time_no_vsync = .0f;
        float frame_time = .0f;
        float cpu_draw_time = .0f;
        float gpu_wait_time = .0f;
        float collision_time = .0f;
        float fps = .0f;
    } stats;
    GuiLabel* stats_label = 0;

    IGameInstance* game_instance = nullptr;
    RenderViewList render_views;
public:
    DefaultRuntime(IGameInstance* game);
    void onDisplayChanged(int, int) override;
    void run() override;
};

