#pragma once

#include <assert.h>
#include <set>
#include <vector>
#include "csg_object.hpp"


class csgGroupObject : public csgObject {
    std::set<csgObject*> objects;
public:
    csgGroupObject() {}
    csgGroupObject(csgObject** objects, int count);

    csgObject* makeCopy() const override {
        std::vector<csgObject*> children_copies;
        children_copies.reserve(objects.size());
        for (auto& o : objects) {
            children_copies.push_back(o->makeCopy());
        }
        csgGroupObject* copy = new csgGroupObject(children_copies.data(), children_copies.size());
        copy->uid = uid;
        return copy;
    }

    void onAdd(csgScene* scene) override;
    void onRemove(csgScene* scene) override;

    void translateRelative(const gfxm::vec3& delta, const gfxm::quat& pivot) override;
    void rotateRelative(const gfxm::quat& delta, const gfxm::vec3& pivot) override;

    void updateAabb();

    // Relinquishes all the objects
    // can then freely remove the group from scene without removing all the grouped objects
    void dissolve() {
        for (auto& o : objects) {
            o->owner = 0;
        }
        objects.clear();
    }

    int objectCount() const {
        return objects.size();
    }
    csgObject* getObject(int i) {
        auto it = objects.begin();
        std::advance(it, i);
        return *it;
    }

    void serializeJson(nlohmann::json& json) override;
    bool deserializeJson(const nlohmann::json& json) override;
};