#pragma once

#include <memory>
#include <vector>
#include "world/actor_prefab.hpp"
#include "world/actor.hpp"
#include "resource_manager/resource_ref.hpp"

#include "scene_space.hpp"


struct SceneEntry_OLD {
    ResourceRef<ActorPrefab> prefab;
    std::unique_ptr<Actor> instance;
};



constexpr static int SCENE_DATA_VERSION = 2;

struct SceneData : public gpuSceneQueryInterface {
    std::unique_ptr<SceneSpace> scene_space;
    std::vector<SceneEntry_OLD> entries;

    SceneData();

    void queryGeometry(const GeometryQuery& q) override {
        scene_space->queryGeometry(q);
    }

    void toJson(nlohmann::json& json) const;
    bool fromJson(const nlohmann::json& json);
};



