#pragma once

#include "math/gfxm.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/render_bucket.hpp"


class scnView {
    gfxm::quat          camera_rotation;
    gfxm::vec3          camera_position;
    float               fov = 90.f;
    float               znear = .01f;
    float               zfar = 1000.f;

    mutable gfxm::mat4 view;
    mutable bool is_dirty = true;


    void updateMatrices() const {
        if(!is_dirty) return;

        view = gfxm::inverse(
            gfxm::translate(gfxm::mat4(1.f), camera_position)
            * gfxm::to_mat4(camera_rotation)
        );
        is_dirty = false;
    }

public:
    void setCameraPosition(const gfxm::vec3& pos) {
        camera_position = pos;
        is_dirty = true;
    }
    void setCameraRotation(const gfxm::quat& rot) {
        camera_rotation = rot;
        is_dirty = true;
    }
    void setFov(float fov) {
        this->fov = fov;
        is_dirty = true;
    }
    void setZNear(float znear) {
        this->znear = znear;
        is_dirty = true;
    }
    void setZFar(float zfar) {
        this->zfar = zfar;
        is_dirty = true;
    }

    float getFov() const { return fov; }
    float getZNear() const { return znear; }
    float getZFar() const { return zfar; }
    
    const gfxm::mat4& getView() const {
        updateMatrices();
        return view;
    }
};

