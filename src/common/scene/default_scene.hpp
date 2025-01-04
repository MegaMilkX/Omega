#pragma once

#include "default_scene.auto.hpp"

#include "scene.hpp"

#include <vector>

#include "world/actor.hpp"
#include "collision/collider.hpp"
#include "filesystem/filesystem.hpp"


class DefaultSceneBuilder;

[[cppi_class]];
class DefaultScene : public IScene {
    friend DefaultSceneBuilder;
    std::vector<std::unique_ptr<Actor>> actors;
public:
    TYPE_ENABLE();

    ~DefaultScene() = default;

    template<typename T>
    T* createActor() {
        Actor* pa = new T();
        std::unique_ptr<Actor> a(pa);
        actors.push_back(std::move(a));
        return pa;
    }

    [[cppi_decl, serialize_json]]
    bool toJson(nlohmann::json& j) {
        nlohmann::json& jactors = j["actors"];
        jactors = nlohmann::json::array();

        for (int i = 0; i < actors.size(); ++i) {
            nlohmann::json jactor = nlohmann::json::object();
            type_write_json(jactor, actors[i]);
            jactors.push_back(jactor);
        }
        return !bool();
    }

    bool serializeJson(const char* filename) {
        get_type().serialize_json(filename, this);
        return true;
    }
};

