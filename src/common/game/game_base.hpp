#pragma once

#include "game_base.auto.hpp"

#include "gpu/gpu.hpp"
#include "gpu/render_bucket.hpp"
#include "player/player.hpp"
#include "world/world.hpp"


[[cppi_class]];
class GameBase {
    RuntimeWorld* world = 0;

public:
    TYPE_ENABLE();
    RuntimeWorld* getWorld() { return world; }

    virtual void onViewportResize(int width, int height) {}

    virtual void onPlayerJoined(IPlayer* player) {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
        if (!local) {
            return;
        }
        assert(local->getViewport());
        local->getViewport()->setWorld(world);
    }
    virtual void onPlayerLeft(IPlayer* player) {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
        if (!local) {
            return;
        }
        assert(local->getViewport());
        local->getViewport()->setWorld(0);
    }

    void init() {
        world = gameWorldCreate();

        onInit();
    }
    void cleanup() {
        onCleanup();

        gameWorldDestroy(world);
        world = 0;
    }
    void update(float dt) {
        onUpdate(dt);

        world->update(dt);
    }
    
    void draw(float dt) {
        onDraw(dt);
    }

    virtual void onInit() = 0;
    virtual void onCleanup() = 0;
    virtual void onUpdate(float dt) = 0;
    virtual void onDraw(float dt) = 0;
};
