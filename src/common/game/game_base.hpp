#pragma once

#include "game_base.auto.hpp"

#include "engine_runtime/engine_runtime.hpp"
#include "player/player.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render_bucket.hpp"
#include "player/player.hpp"
#include "world/world.hpp"


[[cppi_class]];
class IGameInstance : public IPlayerListener {
public:
    TYPE_ENABLE();

    IGameInstance() {
        playerAddListener(this);
    }
    virtual ~IGameInstance() {
        playerRemoveListener(this);
    }

    virtual void onViewportResize(int width, int height) {}

    void init(IEngineRuntime* rt) {
        onInit(rt);
    }
    void cleanup() {
        onCleanup();
    }
    void update(float dt) {
        onUpdate(dt);
    }
    
    void draw(float dt) {
        onDraw(dt);
    }

    virtual void onInit(IEngineRuntime*) = 0;
    virtual void onCleanup() = 0;
    virtual void onUpdate(float dt) = 0;
    virtual void onDraw(float dt) = 0;
};


