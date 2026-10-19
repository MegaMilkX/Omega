#pragma once

#include <memory>
#include "math/gfxm.hpp"
#include "gpu/types.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/scene_query_interface.hpp"

struct RenderViewStats {
    float frame_time = .0f;
    float geom_query_time = .0f;
};

class gpuRenderer;
class EngineRenderView {
    friend class gpuPipeline;

    gfxm::rect              rc;

    std::vector<gpuSceneQueryInterface*> query_interfaces;
    gpuRenderBucket         render_bucket;

    gfxm::mat4              view_transform = gfxm::mat4(1.f);
    gfxm::mat4              projection = gfxm::mat4(1.f);
    float                   fov = gfxm::radian(65.f);
    float                   znear = .1f;
    float                   zfar = 1000.f;

    bool                    is_offscreen = false;

public:
    RenderViewStats stats;

    gpuRenderer* renderer = nullptr;
    std::unique_ptr<gpuRenderTarget> render_target;

    EngineRenderView(const gfxm::rect& rc, RendererType renderer_type, bool is_offscreen = false);
    ~EngineRenderView();

    void clearQueryInterfaces() {
        query_interfaces.clear();
    }
    void addQueryInterface(gpuSceneQueryInterface* qi) {
        query_interfaces.push_back(qi);
    }

    void setView(const gfxm::mat4& view) { view_transform = view; }
    void setFov(float fov) { this->fov = fov; }
    void setZNear(float znear) { this->znear = znear; }
    void setZFar(float zfar) { this->zfar = zfar; }

    const gfxm::mat4& getViewTransform() { return view_transform; }
    const gfxm::mat4& getProjection() {
        if(!render_target) return projection;

        const float w = render_target->getWidth() * (rc.max.x - rc.min.x);
        const float h = render_target->getHeight() * (rc.max.y - rc.min.y);
        projection = gfxm::perspective(fov, w / h, znear, zfar);
        return projection;
    }

    const gfxm::rect& getRect() const { return rc; }
    float getWidth() const { return rc.max.x - rc.min.x; }
    float getHeight() const { return rc.max.y - rc.min.y; }

    bool                isOffscreen() const { return is_offscreen; }
    gpuRenderer*        getRenderer() { return renderer; }
    gpuRenderTarget*    getRenderTarget() { return render_target.get(); }
    gpuRenderBucket*    getRenderBucket() { return &render_bucket; }
    int                 queryInterfaceCount() { return query_interfaces.size(); }
    gpuSceneQueryInterface* getQueryInterface(int i) { return query_interfaces[i]; }
};

