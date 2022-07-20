#pragma once

#include <set>
#include <string>
#include <memory>
#include "reflection/reflection.hpp"

#include "math/gfxm.hpp"


class sklSkeletonEditable;
class sklBone {
    friend sklSkeletonEditable;

    int                                     index = -1;
    sklSkeletonEditable*                    owner = 0;
    sklBone*                                parent = 0;
    std::vector<sklBone*>                   children;
    std::string                             name;

    gfxm::vec3                              translation = gfxm::vec3(0, 0, 0);
    gfxm::quat                              rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::vec3                              scale = gfxm::vec3(1, 1, 1);
    gfxm::mat4                              world_transform = gfxm::mat4(1.0f);

private:
    sklBone(sklSkeletonEditable* owner, sklBone* parent, const char* name = "UnnamedBone")
    : owner(owner), parent(parent), name(name) {
        for (auto it : children) {
            delete it;
        }
    }

public:
    int getIndex() const { return index; }

    void                setName(const char* name) { this->name = name; }
    const std::string&  getName() const { return name; }

    sklBone*    getParent() { return parent; }

    sklBone*    createChild(const char* name = "UnnamedBone");
    bool        removeChild(sklBone* bone);

    size_t childCount() const { return children.size(); }
    sklBone* getChild(int id) {
        auto it = children.begin();
        std::advance(it, id);
        if (it == children.end()) {
            return 0;
        }
        return *it;
    }

    void setTranslation(const gfxm::vec3& offset) {
        translation = offset;
    }
    void setTranslation(float x, float y, float z) {
        setTranslation(gfxm::vec3(x, y, z));
    }
    void setRotation(const gfxm::quat& q) {
        rotation = q;
    }
    void setScale(const gfxm::vec3& scl) {
        scale = scl;
    }


    const gfxm::vec3& getLclTranslation() const {
        return translation;
    }
    const gfxm::quat& getLclRotation() const {
        return rotation;
    }
    const gfxm::vec3& getLclScale() const {
        return scale;
    }

    gfxm::mat4 getLocalTransform() {
        return gfxm::translate(gfxm::mat4(1.0f), translation)
            * gfxm::to_mat4(rotation)
            * gfxm::scale(gfxm::mat4(1.0f), scale);
    }
    const gfxm::mat4& getWorldTransform() {
        gfxm::mat4 local_transform = getLocalTransform();
        if (parent) {
            world_transform = parent->getWorldTransform() * local_transform;
        } else {
            world_transform = local_transform;
        }
        return world_transform;
    }
};