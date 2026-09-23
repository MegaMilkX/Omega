#include "actor.hpp"

#include "nlohmann/json.hpp"
#include <filesystem>
#include "util/timer.hpp"
#include "filesystem/filesystem.hpp"
#include "world/world.hpp"



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
        resolveDirtyNodes(root_node);
    }
}

void Actor::registerNode(ActorNode* parent, ActorNode* child) {
    if (child->actor) {
        LOG_ERR("Can't re-register node, already belongs to an actor");
        assert(false);
        return;
    }

    child->actor = this;
    child->parent = parent;
    child->flags &= ~FActorNode::TreeOwned;

    if(parent) {
        parent->children.push_back(child);
        transformNodeAttach(parent->getTransformHandle(), child->getTransformHandle());
    } else {
        _removeRoot();
        root_node = child;
    }

    child->onDefault();
    child->requestRebuild();
}


Actor::Actor() {
    setRoot<EmptyNode>("root");
}

void Actor::requestRebuild() {
    if(!isSpawned()) return;
    getWorld()->requestRespawn(this);
}

void Actor::onSpawn(WorldSystemRegistry& reg) {
    timer timer_;
    timer_.start();

    // Build node internals
    {
        timer timer_;
        timer_.start();

        root_node->forEachNode([](ActorNode* node) {
            node->onBuild();
        });

        LOG_DBG("[Actor spawn]: onBuild " << timer_.stop() * 1000.f << "ms");
    }

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

        LOG_DBG("[Actor spawn]: Node links rebuilt in " << timer_.stop() * 1000.f << "ms");
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

        LOG_DBG("[Actor spawn]: Post-link dirty resolve in " << timer_.stop() * 1000.f << "ms");
    }
    
    // Prepare nodes for spawning
    {
        timer timer_;
        timer_.start();

        root_node->forEachNode([](ActorNode* node) {
            node->onReady();
        });

        LOG_DBG("[Actor spawn]: onReady " << timer_.stop() * 1000.f << "ms");
    }

    // ===
    {
        timer timer_;
        timer_.start();

        for (auto& kv : drivers) {
            kv.second->onReset();
        }

        LOG_DBG("[Actor spawn]: driver onReset " << timer_.stop() * 1000.f << "ms");
    }
    
    if (root_node) {
        timer timer_;
        timer_.start();

        for (auto& kv : drivers) {
            root_node->_registerGraph(kv.second.get());
        }

        LOG_DBG("[Actor spawn]: driver _registerGraph " << timer_.stop() * 1000.f << "ms");
    }

    if (auto sys = reg.getSystem<ActorDriverSystem>()) {
        timer timer_;
        timer_.start();

        for (auto& kv : drivers) {
            sys->addActorDriver(kv.second.get());
            kv.second->onSpawnActorDriver(reg, this);
        }

        LOG_DBG("[Actor spawn]: onSpawnActorDriver " << timer_.stop() * 1000.f << "ms");
    }

    if (root_node) {
        timer timer_;
        timer_.start();

        root_node->onSpawnNodeInternal(reg);

        LOG_DBG("[Actor spawn]: onSpawnNodeInternal " << timer_.stop() * 1000.f << "ms");
    }

    LOG_DBG("[Actor spawn]: total " << timer_.stop() * 1000.f << "ms");
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

static void collectPrefabProperties(const rtti::MetaObject* object, rtti::type t, std::map<rtti::property, rtti::varying>& props) {
    /*const auto& parent_types = t.get_desc()->parent_types;
    for (const auto& parent_info : parent_types) {
        collectPrefabProperties(object, parent_info.parent_type, props);
    }*/

    for (int i = 0; i < t.prop_count(); ++i) {
        rtti::property prop = t.get_property(i);
        rtti::varying var = t.get_prop_value(object, i);
        props[prop] = var;
    }
}
static void makeNodePrefab(const ActorNode* node, ActorPrefab::NodeBlueprint* prefab_node) {
    auto t = node->get_type();

    node->makeSnapshot(prefab_node->snap);

    for (int i = 0; i < node->childCount(); ++i) {
        auto& ch = prefab_node->children.emplace_back();
        makeNodePrefab(node->getChild(i), &ch);
    }
}
void Actor::makePrefab(ActorPrefab& prefab) {
    prefab.snapshot.clear();
    prefab.drivers.clear();
    prefab.root_node.children.clear();
    
    makeSnapshot(prefab.snapshot);

    for (auto& kv : drivers) {
        auto& drv_snap = prefab.drivers.emplace_back();
        kv.second->makeSnapshot(drv_snap);
    }
    if (root_node) {
        makeNodePrefab(root_node, &prefab.root_node);
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
    dbgDrawActorNodeTree(root_node);
}

