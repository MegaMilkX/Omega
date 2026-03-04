#include "actor.hpp"

#include "nlohmann/json.hpp"
#include <filesystem>
#include "util/timer.hpp"
#include "filesystem/filesystem.hpp"
#include "world/world.hpp"


bool actorWriteJson(Actor* actor, const char* path) {
    using namespace nlohmann;

    json j = json::object();// actor->toJson();
    type t = actor->get_type();
    t.serialize_json(j, actor);

    std::string data = j.dump(4);

    std::filesystem::path fspath = path;
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


static void resolveDirtyNodes(ActorNode* n) {
    for (int i = 0; i < n->childCount(); ++i) {
        resolveDirtyNodes(n->getChild(i));
    }
    if (auto d = dynamic_cast<IDirty*>(n)) {
        d->resolveDirty();
    }
}
void Actor::_resolveDirtyNodes() {
    if(root_node) {
        resolveDirtyNodes(root_node.get());
    }
}


Actor::Actor() {
    setRoot<EmptyNode>("root");
}

void Actor::onSpawn(WorldSystemRegistry& reg) {
    timer timer_;
    timer_.start();

    // Build data links between nodes
    {
        timer timer_;
        timer_.start();

        if (root_node) {
            NodeSlotArray slots;
            root_node->_buildLinks(slots);
            if (!slots.empty()) {
                LOG("Slots unused:");
                for (int i = 0; i < slots.size(); ++i) {
                    LOG("\t" << slots[i].desc.link_type.get_name());
                }
            }
        }

        LOG_DBG("Actor::onSpawn: Node links rebuilt in " << timer_.stop() * 1000.f << "ms");
    }

    // Resolve dirty
    {
        timer timer_;
        timer_.start();

        // Since node links could have caused changes enough to resolve,
        // need to resolve again here
        _resolveDirtyNodes();

        for (auto& kv : drivers) {
            if (auto d = dynamic_cast<IDirty*>(kv.second.get())) {
                d->resolveDirty();
            }
        }
        for (auto& kv : components) {
            if (auto d = dynamic_cast<IDirty*>(kv.second.get())) {
                d->resolveDirty();
            }
        }

        LOG_DBG("Actor::onSpawn: Post-link dirty resolve in " << timer_.stop() * 1000.f << "ms");
    }

    // ===
    for (auto& kv : drivers) {
        kv.second->onReset();
    }
    
    if (root_node) {
        for (auto& kv : drivers) {
            root_node->_registerGraph(kv.second.get());
        }
    }

    if (auto sys = reg.getSystem<ActorDriverSystem>()) {
        for (auto& kv : drivers) {
            sys->addActorDriver(kv.second.get());
            kv.second->onSpawnActorDriver(reg, this);
        }
    }

    if (root_node) {
        root_node->onSpawnNodeInternal(reg);
    }

    LOG_DBG("Actor spawned in " << timer_.stop() * 1000.f << "ms");
}

void Actor::onDespawn(WorldSystemRegistry& reg) {
    if (root_node) {
        for (auto& kv : drivers) {
            root_node->_unregisterGraph(kv.second.get());
        }
        root_node->onDespawnNodeInternal(reg);
    }

    if (auto sys = reg.getSystem<ActorDriverSystem>()) {
        for (auto& kv : drivers) {
            sys->removeActorDriver(kv.second.get());
            kv.second->onDespawnActorDriver(reg, this);
        }
    }
}

GAME_MESSAGE Actor::onMessage(GAME_MESSAGE msg) {
    for (auto& kv : drivers) {
        auto rsp = kv.second->onMessage(msg);
        if (rsp.msg != GAME_MSG::NOT_HANDLED) {
            return rsp;
        }
    }
    return GAME_MSG::NOT_HANDLED;
}

static void collectPrefabProperties(const MetaObject* object, type t, std::map<property, varying>& props) {
    /*const auto& parent_types = t.get_desc()->parent_types;
    for (const auto& parent_info : parent_types) {
        collectPrefabProperties(object, parent_info.parent_type, props);
    }*/

    for (int i = 0; i < t.prop_count(); ++i) {
        property prop = t.get_property(i);
        varying var = t.get_prop_value(object, i);
        props[prop] = var;
    }
}
static void makeNodePrefab(const ActorNode* node, ActorPrefab::NodeBlueprint* prefab_node) {
    auto t = node->get_type();
    prefab_node->t = t;

    collectPrefabProperties(node, t, prefab_node->properties);

    for (int i = 0; i < node->childCount(); ++i) {
        auto& ch = prefab_node->children.emplace_back();
        makeNodePrefab(node->getChild(i), &ch);
    }
}
void Actor::makePrefab(ActorPrefab& prefab) {
    prefab.components.clear();
    prefab.drivers.clear();
    prefab.root_node.children.clear();
    
    for (auto& kv : components) {
        auto& comp = prefab.components[kv.first];
        auto t = kv.second->get_type();
        collectPrefabProperties(kv.second.get(), t, comp.properties);
    }
    for (auto& kv : drivers) {
        auto& drv = prefab.drivers[kv.first];
        auto t = kv.second->get_type();
        collectPrefabProperties(kv.second.get(), t, drv.properties);
    }
    if (root_node) {
        makeNodePrefab(root_node.get(), &prefab.root_node);
    }
}

static void dbgDrawActorNodeTree(const ActorNode* n) {
    if(!n) return;
    n->dbgDraw();
    for (int i = 0; i < n->childCount(); ++i) {
        dbgDrawActorNodeTree(n->getChild(i));
    }
}
void Actor::dbgDraw() const {
    dbgDrawActorNodeTree(root_node.get());
}

void Actor::toJson(nlohmann::json& j) {
    using namespace nlohmann;

    type t = get_type();
    //j["type"] = t.get_name();
    j["flags64"] = getFlags();

    ActorNode* root_node = getRoot();
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
    for (auto& c : drivers) {
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
    
    ActorNode* node = type_new_from_json<ActorNode>(jroot_node);
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
            ActorDriver* ctrl = type_new_from_json<ActorDriver>(jj);
            if (!ctrl) {
                LOG_ERR("Failed to read actor controller json");
                assert(false);
                continue;
            }
            ctrl->owner = this;
            drivers.insert(std::make_pair(ctrl->get_type(), std::unique_ptr<ActorDriver>(ctrl)));
        }
    }

    return true;
}