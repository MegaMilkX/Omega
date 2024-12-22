#pragma once

#include "math/gfxm.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/render_bucket.hpp"
#include "world/world.hpp"


class Viewport {
    gfxm::rect          rc;
    //gfxm::ivec2         size_pixels;
    RuntimeWorld*       world = 0;
    gfxm::quat          camera_rotation;
    gfxm::vec3          camera_position;
    float               fov = 90.f;
    float               znear = .01f;
    float               zfar = 1000.f;

    gpuRenderTarget*    p_render_target = 0;
    gpuRenderBucket     render_bucket;

    gfxm::mat4          view_transform;
    gfxm::mat4          projection;

    bool                is_offscreen = false;

public:
    Viewport(const gfxm::rect& rc, RuntimeWorld* world, gpuRenderTarget* target, bool is_offscreen = false)
        : rc(rc), world(world), is_offscreen(is_offscreen)
        , p_render_target(target)
        , render_bucket(gpuGetPipeline(), 1000)
    {
    
    }

    void setWorld(RuntimeWorld* world) { this->world = world; }

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
        if (p_render_target == nullptr) {
            assert(false);
            return gfxm::mat4(1.f);
        }
        const float w = p_render_target->getWidth() * (rc.max.x - rc.min.x);
        const float h = p_render_target->getHeight() * (rc.max.y - rc.min.y);
        projection = gfxm::perspective(gfxm::radian(fov), w / h, znear, zfar);
        return projection;
    }
    const gfxm::rect& getRect() const { return rc; }
    float getWidth() const { return rc.max.x - rc.min.x; }
    float getHeight() const { return rc.max.y - rc.min.y; }

    bool                isOffscreen() const { return is_offscreen; }
    gpuRenderTarget*    getRenderTarget() { return p_render_target; }
    gpuRenderBucket*    getRenderBucket() { return &render_bucket; }
    RuntimeWorld*       getWorld() { return world; }

    void                setRenderTarget(gpuRenderTarget* target) { p_render_target = target; }
    /*
    void updateAvailableSize(int screen_width, int screen_height) {
        gfxm::rect rc_ = gfxm::rect(
            screen_width * rc.min.x, screen_height * rc.min.y,
            screen_width * rc.max.x, screen_height * rc.max.y
        );
        gfxm::ivec2 size(
            rc_.max.x - rc_.min.x,
            rc_.max.y - rc_.min.y
        );
        if (size_pixels.x != size.x || size_pixels.y != size.y) {
            size_pixels = size;
        }
    }*/
};

