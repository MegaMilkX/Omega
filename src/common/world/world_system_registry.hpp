#pragma once

#include <assert.h>
#include <unordered_map>
#include "log/log.hpp"
#include "reflection/reflection.hpp"


// World systems are present from world construction
// to it's destruction. Also other stuff can and will
// rely on systems always present if they already are,
// so no registration removal
class WorldSystemRegistry {
    std::unordered_map<type, void*> system_registry;
public:
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
        for (auto parent_type : parent_types) {
            registerSystem(parent_type, psys);
        }
        if (system_registry.find(t) != system_registry.end()) {
            LOG_ERR("WorldSystemRegistry: '" << t.get_name() << "' already registered, overwritten");
            assert(false);
        }
        system_registry.insert(std::make_pair(t, psys));
    }
};

