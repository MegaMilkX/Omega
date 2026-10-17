#include "scene_data.hpp"

#include "terrain/terrain_space.hpp"


SceneData::SceneData() {
    scene_space.reset(new TerrainSceneSpace);
}

void SceneData::toJson(nlohmann::json& json) const {
    json["version"] = SCENE_DATA_VERSION;

    nlohmann::json& jentries = json["entries"];
    jentries = nlohmann::json::array();
    for (int i = 0; i < entries.size(); ++i) {
        ActorPrefab prefab;
        entries[i].instance->makePrefab(prefab);
        nlohmann::json& jentry = jentries.emplace_back();
        prefab.toJson(jentry);
    }

    nlohmann::json& jspace = json["space"];
    scene_space->toJson(jspace);
}
bool SceneData::fromJson(const nlohmann::json& json) {
    int version = json["version"].get<int>();
    LOG("SCENE VERSION: " << version);

    nlohmann::json jentries = json.value("entries", nlohmann::json::array());
    if (!jentries.is_array()) {
        return false;
    }

    entries.clear();
    for (auto it = jentries.begin(); it != jentries.end(); ++it) {
        nlohmann::json jentry = it->get<nlohmann::json>();
        ActorPrefab prefab;
        if (!prefab.fromJson(jentry)) {
            LOG_ERR("Failed to load actor");
            continue;
        }
        auto& entry = entries.emplace_back();
        entry.instance = std::unique_ptr<Actor>(prefab.instantiate()); // TODO: new is hidden here, kinda uncomfortable
    }

    if (version >= 2) {
        nlohmann::json jspace = json.value("space", nlohmann::json::object());
        scene_space.reset(new TerrainSceneSpace); // TODO: scene space types
        scene_space->fromJson(jspace);
    }

    return true;
}