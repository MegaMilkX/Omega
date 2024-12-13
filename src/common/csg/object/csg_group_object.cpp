#include "csg_group_object.hpp"
#include "../csg_scene.hpp"


csgGroupObject::csgGroupObject(csgObject** objects, int count) {
    if (count < 1) {
        assert(false);
        return;
    }
    for (int i = 0; i < count; ++i) {
        this->objects.insert(objects[i]);
        objects[i]->owner = this;
    }

    aabb = objects[0]->aabb;
    for (int i = 1; i < count; ++i) {
        aabb = gfxm::aabb_union(aabb, objects[i]->aabb);
    }
    transform = gfxm::translate(gfxm::mat4(1.f), gfxm::lerp(aabb.from, aabb.to, .5f));
}

void csgGroupObject::onAdd(csgScene* scene) {
    assert(!isInScene());
    std::vector<csgObject*> sorted;
    sorted.reserve(objects.size());
    for (auto& o : objects) {
        sorted.push_back(o);
    }
    std::sort(sorted.begin(), sorted.end(), [](const csgObject* a, const csgObject* b)->bool {
        return a->uid < b->uid;
    });
    for (int i = 0; i < sorted.size(); ++i) {
        auto o = sorted[i];
        if (o->isInScene()) {
            continue;
        }
        scene->addObject(o);
    }
}

void csgGroupObject::onRemove(csgScene* scene) {
    assert(isInScene());
    for (auto& o : objects) {
        if (!o->isInScene()) {
            continue;
        }
        scene->removeObject(o);
    }
}

void csgGroupObject::translateRelative(const gfxm::vec3& delta, const gfxm::quat& pivot) {
    transform
        = gfxm::translate(gfxm::mat4(1.f), delta)
        * transform;
    for (auto& o : objects) {
        if (!o->isInScene()) {
            assert(false);
            continue;
        }
        o->translateRelative(delta, pivot);
    }
    updateAabb();
}
void csgGroupObject::rotateRelative(const gfxm::quat& delta, const gfxm::vec3& pivot) {
    gfxm::mat4 tr = gfxm::translate(gfxm::mat4(1.f), pivot);
    gfxm::mat4 invtr = gfxm::inverse(tr);
    transform = tr * gfxm::to_mat4(delta) * invtr * transform;
    
    for (auto& o : objects) {
        if (!o->isInScene()) {
            assert(false);
            continue;
        }
        o->rotateRelative(delta, pivot);
    }
    updateAabb();
}

void csgGroupObject::updateAabb() {
    if (objects.empty()) {
        return;
    }
    aabb = (*objects.begin())->aabb;
    for (auto& o : objects) {
        aabb = gfxm::aabb_union(aabb, o->aabb);
    }
}

void csgGroupObject::serializeJson(nlohmann::json& json) {
    json["type"] = "csgGroupObject";
    type_write_json(json["transform"], transform);
    type_write_json(json["uid"], uid);

    nlohmann::json& jobjects = json["objects"];
    jobjects = nlohmann::json::array();
    for (auto& o : objects) {
        nlohmann::json jobject;
        o->serializeJson(jobject);
        jobjects.push_back(jobject);
    }
}
bool csgGroupObject::deserializeJson(const nlohmann::json& json) {
    type_read_json(json["transform"], transform);
    type_read_json(json["uid"], uid);

    const nlohmann::json& jobjects = json["objects"];
    if (!jobjects.is_array()) {
        return false;
    }
    int object_count = jobjects.size();
    for (int i = 0; i < object_count; ++i) {
        const nlohmann::json& jobject = jobjects[i];
        std::string type;
        type_read_json(jobject["type"], type);
        csgObject* object = csgCreateObjectFromTypeName(type);
        if (!object) {
            continue;
        }
        object->owner = this;
        object->scene = scene;
        object->deserializeJson(jobject);
        object->scene = 0;  // TODO: hack, object must have a scene ref to get materials
                            // but they must be not 'in scene' when being added
        objects.insert(object);
    }
    updateAabb();
    return true;
}

