#pragma once

#include <string>
#include "math/gfxm.hpp"
#include "nlohmann/json.hpp"


class csgScene;
class csgObject {
public:
    csgScene* scene = 0;
    csgObject* owner = 0;
    int uid = 0;
    gfxm::mat4 transform = gfxm::mat4(1.f);
    gfxm::aabb aabb;

    virtual ~csgObject() {}

    bool isInScene() { return scene != nullptr; }

    virtual csgObject* makeCopy() const = 0;

    virtual void setTransform(const gfxm::mat4& transform) {
        this->transform = transform;
    }

    virtual void translateRelative(const gfxm::vec3& delta, const gfxm::quat& pivot) {}
    virtual void rotateRelative(const gfxm::quat& delta, const gfxm::vec3& pivot) {}

    virtual void onAdd(csgScene* scene) {}
    virtual void onRemove(csgScene* scene) {}

    virtual void serializeJson(nlohmann::json& json) { json["type"] = "NOT_IMPLEMENTED"; }
    virtual bool deserializeJson(const nlohmann::json& json) { return true; }
};

csgObject* csgCreateObjectFromTypeName(const std::string& type);
