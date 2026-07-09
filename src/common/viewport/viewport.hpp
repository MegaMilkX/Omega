#pragma once

#include <memory>
#include "math/gfxm.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/render_target_map.hpp"
#include "gpu/render_bucket.hpp"
#include "world/world.hpp"


class EngineRenderView {
    gfxm::rect          rc;
    Camera*             cam = nullptr;

    gpuRenderBucket     render_bucket;

    gfxm::mat4          view_transform = gfxm::mat4(1.f);
    gfxm::mat4          projection = gfxm::mat4(1.f);

    bool                is_offscreen = false;

public:
    std::unique_ptr<gpuRenderTarget> render_target;
    std::unique_ptr<gpuRenderTargetMap> rt_map_clear; // temporarily here
    std::unique_ptr<gpuRenderTargetMap> rt_map_world; // temporarily here
    std::unique_ptr<gpuRenderTargetMap> rt_map_view; // temporarily here
    std::unique_ptr<gpuRenderTargetMap> rt_map_blit_depth; // temporarily here
    std::unique_ptr<gpuRenderTargetMap> rt_map_post; // temporarily here

    EngineRenderView(const gfxm::rect& rc, Camera* cam, bool is_offscreen = false)
        : rc(rc), cam(cam), is_offscreen(is_offscreen)
        , render_bucket(gpuGetPipeline(), 1000)
    {}

    void setCamera(Camera* c) {
        cam = c;
    }

    const gfxm::mat4& getViewTransform() {
        if(!cam) return view_transform;
        view_transform = cam->getViewTransform();
        return view_transform;
    }
    const gfxm::mat4& getProjection() {
        if(!cam) return projection;
        if(!render_target) return projection;

        const float w = render_target->getWidth() * (rc.max.x - rc.min.x);
        const float h = render_target->getHeight() * (rc.max.y - rc.min.y);
        projection = gfxm::perspective(cam->getFov(), w / h, cam->getZNear(), cam->getZFar());
        return projection;
    }
    const gfxm::rect& getRect() const { return rc; }
    float getWidth() const { return rc.max.x - rc.min.x; }
    float getHeight() const { return rc.max.y - rc.min.y; }

    bool                isOffscreen() const { return is_offscreen; }
    gpuRenderTarget*    getRenderTarget() { return render_target.get(); }
    gpuRenderBucket*    getRenderBucket() { return &render_bucket; }
    Camera*             getCamera() { return cam; }
};

