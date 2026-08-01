#pragma once

#include <unordered_map>
#include "reflection/reflection.hpp"
#include "byte_reader/byte_reader.hpp"


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
    std::unordered_map<rtti::type, void*(*)(void)> factories;
public:
    virtual ~IResourceBackend() {}
    virtual ResourceEntry* findEntry(const std::string&) = 0;
    virtual ResourceEntry* createEntry(const std::string&) = 0;
    virtual void* load(ResourceEntry*) = 0;
    virtual void release(void*) = 0;
    virtual void collectGarbage() = 0;
    virtual void update() {}

    void* create(rtti::type t) {
        auto it = factories.find(t);
        if (it == factories.end()) {
            LOG_DBG("IResourceBackend: create NOT IMPLEMENTED for " << t.get_name());
            return nullptr;
        }
        LOG_DBG("IResourceBackend: creating " << t.get_name());
        return it->second();
    }

    template<typename T>
    void* create() {
        return create(rtti::type_get<T>());
    }

    template<typename T>
    void registerFactory(void*(*factory)(void)) {
        factories[rtti::type_get<T>()] = factory;
    }
};


