#pragma once

#include <vector>
#include "engine_runtime.hpp"
#include "game/game_base.hpp"
#include "gui/gui.hpp"

#include "engine_runtime/components/render_view_list.hpp"
#include "engine_runtime/util/dev_console.hpp"


class DefaultRuntime : public IEngineRuntime {
    struct ENGINE_STATS {
        float frame_time_no_vsync = FLT_MAX;
        float frame_time = FLT_MAX;
        float cpu_draw_time = FLT_MAX;
        float gpu_wait_time = FLT_MAX;
        float collision_time = FLT_MAX;
        float ui_layout_time = FLT_MAX;
        float ui_draw_time = FLT_MAX;
        float ui_render_time = FLT_MAX;
        float fps = FLT_MAX;
    } stats;
    GuiTextElement* stats_label = 0;

    IGameInstance* game_instance = nullptr;
    RenderViewList render_views;

    GuiDevConsole* dev_console = nullptr;
    InputContext input_ctx = InputContext("DefaultRuntime");
    InputAction* inputDevConsole = 0;
public:
    DefaultRuntime(IGameInstance* game);
    void onDisplayChanged(int, int) override;
    void run() override;
};

