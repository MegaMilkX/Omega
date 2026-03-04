#pragma once

#include <atomic>
#include <string>
#include "reflection/reflection.hpp"
#include "resource_backend.hpp"


enum eResourceState {
    eResourceInvalidState,
    eResourcePresent,
    eResourceAbsent,
    eResourceLoading,
    eResourceUnloaded,
};
inline const char* resource_state_to_string(eResourceState state) {
    switch (state) {
    case eResourceInvalidState: return "invalid";
    case eResourcePresent: return "present";
    case eResourceAbsent: return "absent";
    case eResourceLoading: return "loading";
    case eResourceUnloaded: return "unloaded";
    }
    return "<unknown>";
}

enum eUriSchema {
    eUriNone,
    eUriError,
    eUriFile,
};
inline const char* uri_schema_to_string(eUriSchema sch) {
    switch (sch) {
    case eUriNone: return "<none>";
    case eUriError: return "<error>";
    case eUriFile: return "file";
    }
    return "<unknown>";
}

struct ResourceEntry {
    ResourceEntry() {}
    virtual ~ResourceEntry() {}

    IResourceBackend* backend = nullptr;
    void* data = nullptr;
    eResourceState state = eResourceInvalidState;
    std::atomic<int> ref_count = 0;
    std::string resource_id;
    eUriSchema schema = eUriNone;
    std::string resource_path;
    std::set<ResourceEntry*> dependents;

    void addRef() {
        ++ref_count;
    }
    void releaseRef() {
        assert(ref_count > 0);
        --ref_count;
    }

    virtual type getType() = 0;
};


template<typename RES_T>
struct TResourceEntry : public ResourceEntry {
    TResourceEntry() {}

    type getType() override {
        return type_get<RES_T>();
    }
};

