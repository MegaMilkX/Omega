#pragma once

#include <assert.h>
#include <typeindex>
#include <string>
#include <unordered_map>

#include "reflection/reflection.hpp"

struct mdlComponentMutable;
struct mdlComponentPrototype;
struct mdlComponentInstance;

template<typename MUTABLE_T, typename PROTO_T, typename INST_T>
class ComponentType {
public:
    typedef MUTABLE_T mutable_t;
    typedef PROTO_T proto_t;
    typedef INST_T inst_t;
};

struct ComponentTypeDesc {
    std::string name;
    mdlComponentMutable*    (*pfn_constructMutable)() = 0;
    mdlComponentPrototype*  (*pfn_constructPrototype)() = 0;
    mdlComponentInstance*   (*pfn_constructInstance)() = 0;

    bool                    (*pfn_serializePrototype)(FILE*, mdlComponentPrototype*) = 0;
};
inline std::unordered_map<type, ComponentTypeDesc>& componentGetTypeMap() {
    static std::unordered_map<type, ComponentTypeDesc> types;
    return types;
}

template<typename T>
void mdlComponentRegister(const char* name) {
    auto& map = componentGetTypeMap();
    ComponentTypeDesc desc;
    desc.name = name;
    desc.pfn_constructMutable = []()->mdlComponentMutable* {
        return new typename T::mutable_t;
    };
    desc.pfn_constructPrototype = []()->mdlComponentPrototype* {
        return new typename T::proto_t;
    };
    desc.pfn_constructInstance = []()->mdlComponentInstance* {
        return new typename T::inst_t;
    };
    map[type_get<T>()] = desc;

    T::mutable_t().reflect();
    T::proto_t().reflect();
    T::inst_t().reflect();
}

inline ComponentTypeDesc* mdlComponentGetTypeDesc(type t) {
    auto& map = componentGetTypeMap();
    auto it = map.find(t);
    if (it == map.end()) {
        return 0;
    }
    return &it->second;
}
inline mdlComponentMutable* mdlComponentCreateMutable(type t) {
    auto desc = mdlComponentGetTypeDesc(t);
    if (!desc) {
        assert(false);
        return 0;
    }
    return desc->pfn_constructMutable();
}
inline mdlComponentPrototype* mdlComponentCreatePrototype(type t) {
    auto desc = mdlComponentGetTypeDesc(t);
    if (!desc) {
        assert(false);
        return 0;
    }
    return desc->pfn_constructPrototype();
}
