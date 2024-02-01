#include "actor.hpp"

#include "nlohmann/json.hpp"
#include <experimental/filesystem>
#include "filesystem/filesystem.hpp"

bool actorWriteJson(Actor* actor, const char* path) {
    using namespace nlohmann;

    json j = json::object();// actor->toJson();
    type t = actor->get_type();
    t.serialize_json(j, actor);

    std::string data = j.dump(4);

    std::experimental::filesystem::path fspath = path;
    fspath.remove_filename();
    fsCreateDirRecursive(fspath.string());

    FILE* f = fopen(path, "wb");
    if (!f) {
        assert(false);
        return false;
    }
    size_t written = fwrite(data.data(), data.size(), 1, f);
    if (written != 1) {
        fclose(f);
        assert(false);
        return false;
    }
    fclose(f);
    return true;
}

Actor* actorReadJson(const char* path) {
    return type_new_from_json<Actor>(path);
}


GAME_MESSAGE Actor::onMessage(GAME_MESSAGE msg) {
    return GAME_MSG::NOT_HANDLED;
}


void Actor::toJson(nlohmann::json& j) {
    using namespace nlohmann;

    type t = get_type();
    //j["type"] = t.get_name();
    j["flags64"] = getFlags();

    gameActorNode* root_node = getRoot();
    if (root_node) {
        type t = root_node->get_type();
        t.serialize_json(j["root_node"], root_node);
    }

    json& jcomponents = j["components"];
    for (auto& c : components) {
        json jc = json::object();
        type t = c.second->get_type();
        t.serialize_json(jc, c.second.get());
        jcomponents.push_back(jc);
    }

    json& jcontrollers = j["controllers"];
    for (auto& c : controllers) {
        json jc = json::object();
        type t = c.second->get_type();
        t.serialize_json(jc, c.second.get());
        jcontrollers.push_back(jc);
    }
}

bool Actor::fromJson(const nlohmann::json& j) {
    using namespace nlohmann;
    const json& jflags = j["flags64"];
    const json& jroot_node = j["root_node"];
    const json& jcomponents = j["components"];
    const json& jcontrollers = j["controllers"];

    flags = jflags.get<actor_flags_t>();
    
    gameActorNode* node = type_new_from_json<gameActorNode>(jroot_node);
    root_node.reset(node);

    if (jcomponents.is_array()) {
        for (const auto& jj : jcomponents) {
            ActorComponent* comp = type_new_from_json<ActorComponent>(jj);
            if (!comp) {
                LOG_ERR("Failed to read actor component json");
                assert(false);
                continue;
            }
            components.insert(std::make_pair(comp->get_type(), std::unique_ptr<ActorComponent>(comp)));
        }
    }

    if (jcontrollers.is_array()) {
        for (const auto& jj : jcontrollers) {
            ActorController* ctrl = type_new_from_json<ActorController>(jj);
            if (!ctrl) {
                LOG_ERR("Failed to read actor controller json");
                assert(false);
                continue;
            }
            ctrl->owner = this;
            controllers.insert(std::make_pair(ctrl->get_type(), std::unique_ptr<ActorController>(ctrl)));
        }
    }

    return true;
}