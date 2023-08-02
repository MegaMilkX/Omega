#pragma once

#include "math/gfxm.hpp"

class Actor {
    gfxm::mat4 world_transform;
    gfxm::vec3 translation;
    gfxm::quat rotation;
    bool transform_dirty = false;

    void setTransformDirty() {
        transform_dirty = true;
    }

public:
    virtual ~Actor() {}

    void setTranslation(const gfxm::vec3& t) {
        translation = t;
        setTransformDirty();
    }
    void setRotation(const gfxm::quat& q) {
        rotation = q;
        setTransformDirty();
    }

    void translate(const gfxm::vec3& t) {
        translation += t;
        setTransformDirty();
    }

    const gfxm::vec3& getTranslation() const {
        return translation;
    }
    const gfxm::quat& getRotation() const {
        return rotation;
    }

    gfxm::vec3 getForward() {
        return getWorldTransform() * gfxm::vec4(0, 0, 1, 0);
    }

    const gfxm::mat4& getWorldTransform() {
        if (transform_dirty) {
            return world_transform 
                = gfxm::translate(gfxm::mat4(1.0f), translation)
                * gfxm::to_mat4(rotation);
            transform_dirty = false;
        } else {
            return world_transform;
        }
    }
};