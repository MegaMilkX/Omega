#pragma once

#include "game_base.auto.hpp"

#include "gpu/gpu.hpp"
#include "gpu/render_bucket.hpp"
#include "player/player.hpp"
#include "world/world.hpp"


[[cppi_class]];
class GameBase {
    RuntimeWorld* world;

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

    virtual void init() {
        world = gameWorldCreate();
    }
    virtual void cleanup() {
        gameWorldDestroy(world);
        world = 0;
    }
    virtual void update(float dt) {
        world->update(dt);
    }
    
    virtual void draw(float dt) {}

};
