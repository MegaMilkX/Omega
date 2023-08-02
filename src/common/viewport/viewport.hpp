#pragma once

#include "math/gfxm.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/render_bucket.hpp"
#include "world/world.hpp"


class Viewport {
    gfxm::rect          rc;
    gpuRenderTarget*    render_target = 0;
    gameWorld*          world = 0;
    gfxm::quat          camera_rotation;
    gfxm::vec3          camera_position;
    float               fov = 90.f;
    float               znear = .01f;
    float               zfar = 1000.f;

    gpuRenderBucket     render_bucket;

    gfxm::mat4          view_transform;
    gfxm::mat4          projection;

    bool                is_offscreen = false;

public:
    Viewport(const gfxm::rect& rc, gpuRenderTarget* target, gameWorld* world, bool is_offscreen = false)
        : rc(rc), render_target(target), world(world), is_offscreen(is_offscreen)
        , render_bucket(gpuGetPipeline(), 1000) {}

    void setCameraPosition(const gfxm::vec3& pos) { camera_position = pos; }
    void setCameraRotation(const gfxm::quat& rot) { camera_rotation = rot; }
    void setFov(float fov) { this->fov = fov; }
    void setZNear(float znear) { this->znear = znear; }
    void setZFar(float zfar) { this->zfar = zfar; }

    const gfxm::mat4& getViewTransform() {
        view_transform = gfxm::inverse(
            gfxm::translate(gfxm::mat4(1.f), camera_position)
            * gfxm::to_mat4(camera_rotation)
        );
        return view_transform;
    }
    const gfxm::mat4& getProjection() {
        const float w = rc.max.x - rc.min.x;
        const float h = rc.max.y - rc.min.y;
        projection = gfxm::perspective(gfxm::radian(fov), w / h, znear, zfar);
        return projection;
    }
    float getWidth() const { return rc.max.x - rc.min.x; }
    float getHeight() const { return rc.max.y - rc.min.y; }

    bool                isOffscreen() const { return is_offscreen; }
    gpuRenderTarget*    getRenderTarget() { return render_target; }
    gameWorld*          getWorld() { return world; }
};