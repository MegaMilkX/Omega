#pragma once

#include <assert.h>
#include <set>
#include <unordered_map>
#include "log/log.hpp"
#include "reflection/reflection.hpp"
#include "spawnable/spawnable.hpp"


// World systems are present from world construction
// to it's destruction. Also other stuff can and will
// rely on systems always present if they already are,
// so no registration removal
class WorldSystemRegistry {
    std::unordered_map<type, void*> system_registry;
    std::set<ISpawnable*> spawned;
    std::set<ISpawnable*> deferred_despawns;
public:
    virtual ~WorldSystemRegistry() {
        // Unregister systems first,
        // since by this point they
        // can't be considered valid
        system_registry.clear();
        // There's no more systems,
        // but onSpawn/onDespawn symmetry is still important
        for (auto& s : spawned) {
            despawn(s);
        }
    }

    template<typename SYSTEM_T>
    SYSTEM_T* getSystem() {
        return (SYSTEM_T*)getSystem(type_get<SYSTEM_T>());
    }
    void* getSystem(type t) {
        auto it = system_registry.find(t);
        if (it == system_registry.end()) {
            return nullptr;
        }
        return it->second;
    }
protected:
    // Registration is only used for other things,
    // for example ActorNodes, to be able to query
    // a world for a system. If something's not used
    // externally - it does not need to be registered
    template<typename SYSTEM_T>
    void registerSystem(SYSTEM_T* psys) {
        registerSystem(type_get<SYSTEM_T>(), psys);
    }
    void registerSystem(type t, void* psys) {
        const auto& parent_types = t.get_desc()->parent_types;
        for (const auto& parent_info : parent_types) {
            registerSystem(parent_info.parent_type, psys);
        }
        if (system_registry.find(t) != system_registry.end()) {
            LOG_ERR("WorldSystemRegistry: '" << t.get_name() << "' already registered, overwritten");
            assert(false);
        }
        system_registry.insert(std::make_pair(t, psys));
    }

public:
    void beginFrame() {
        for (auto s : deferred_despawns) {
            despawn(s);
        }
        deferred_despawns.clear();
    }

    void spawn(ISpawnable* s) {
        s->registry = this;
        spawned.insert(s);
        s->onSpawn(*this);
    }

    void despawn(ISpawnable* s) {
        s->onDespawn(*this);
        spawned.erase(s);
        s->registry = nullptr;
    }
    void despawnDeferred(ISpawnable* s) {
        deferred_despawns.insert(s);
    }
};

