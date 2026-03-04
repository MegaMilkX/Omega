#pragma once

#include "game/game_base.hpp"

#include "engine_runtime/components/render_view_list.hpp"


class TestGameInstance2 : public IGameInstance {
    std::unique_ptr<IPlayer> primary_player;
    std::unique_ptr<EngineRenderView> primary_view;

public:
    void onInit(IEngineRuntime* rt) override {
        primary_view.reset(new EngineRenderView(gfxm::rect(0, 0, 1, 1), 0, 0, false));
        primary_player.reset(new LocalPlayer(primary_view.get(), 0));

        primary_view->setRenderTarget(gpuGetDefaultRenderTarget());    
        if(auto list = rt->getComponent<RenderViewList>()) {
            list->push_back(primary_view.get());
        }

        playerAdd(primary_player.get());
        playerSetPrimary(primary_player.get());

    }
    void onCleanup() override {

    }

    void onUpdate(float dt) override {

    }

    void onDraw(float dt) override {

    }
};