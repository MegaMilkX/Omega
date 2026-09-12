#pragma once

#include <unordered_map>
#include "reflection/reflection.hpp"
#include "byte_reader/byte_reader.hpp"
#include "resource_manager/resource.hpp"


enum class eResourceLoadResult {
    Failed,
    Pending,
    Done
};

class IResourceBackend;
template<typename T>
struct ResourceBackendTraits {
    constexpr static bool available = false;
};

#define RESOURCE_BACKEND(RES_T, BACKEND_T) \
template<> \
struct ResourceBackendTraits<RES_T> { \
    constexpr static bool available = true; \
    using BACKEND_TYPE = BACKEND_T; \
};

struct ResourceEntry;
class IResourceBackend {
    std::unordered_map<rtti::type, Resource*(*)(void)> factories;
public:
    virtual ~IResourceBackend() {}
    virtual ResourceEntry* findEntry(const std::string&) = 0;
    virtual ResourceEntry* createEntry(const std::string&) = 0;
    virtual eResourceLoadResult load(ResourceEntry*) = 0;
    virtual void release(Resource*) = 0;
    virtual void collectGarbage() = 0;
    virtual void update() {}

    Resource* create(rtti::type t) {
        auto it = factories.find(t);
        if (it == factories.end()) {
            LOG_DBG("IResourceBackend: create NOT IMPLEMENTED for " << t.get_name());
            return nullptr;
        }
        LOG_DBG("IResourceBackend: creating " << t.get_name());
        return it->second();
    }

    template<typename T>
    Resource* create() {
        return create(rtti::type_get<T>());
    }

    template<typename T>
    void registerFactory(Resource*(*factory)(void)) {
        factories[rtti::type_get<T>()] = factory;
    }
};


