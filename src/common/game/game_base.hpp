#pragma once

#include "game_base.auto.hpp"

#include "gpu/gpu.hpp"
#include "gpu/render_bucket.hpp"
#include "player/player.hpp"
#include "world/world.hpp"


[[cppi_class]];
class GameBase {
    RuntimeWorld* world;
    //std::unique_ptr<gpuRenderBucket> render_bucket;
public:
    TYPE_ENABLE();
    RuntimeWorld* getWorld() { return world; }
    //gpuRenderBucket* getRenderBucket() { return render_bucket.get(); }

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
        //render_bucket.reset(new gpuRenderBucket(gpuGetPipeline(), 10000));
    }
    virtual void cleanup() {
        gameWorldDestroy(world);
        world = 0;
    }
    virtual void update(float dt) {
        world->update(dt);
    }
    
    virtual void draw(float dt) {/*
        // TODO: I hate this
        static gfxm::mat4 projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
        static gfxm::mat4 view(1.0f);
        auto cam_node = getWorld()->getCurrentCameraNode();
        if (cam_node) {
            projection = cam_node->projection;
            view = gfxm::inverse(cam_node->getWorldTransform());
        }

        world->getRenderScene()->draw(render_bucket.get());     

        gpuDraw(render_bucket.get(), gpuGetDefaultRenderTarget(), view, projection);
        */
        /*
        // Clearing the bucket at the end so that we can add renderables outside of this function
        render_bucket->clear();*/
    }
};
