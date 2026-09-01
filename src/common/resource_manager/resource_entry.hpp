#pragma once

#include <atomic>
#include <stdint.h>
#include <string>
#include "reflection/reflection.hpp"
#include "resource_backend.hpp"


enum eResourceState {
    eResourceInvalidState,
    eResourcePresent,
    eResourceAbsent,
    eResourceLoading, // This is for recursive loading on the same thread
    eResourcePending, // This is for async loading
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
    eUriBase64
};
inline const char* uri_schema_to_string(eUriSchema sch) {
    switch (sch) {
    case eUriNone: return "<none>";
    case eUriError: return "<error>";
    case eUriFile: return "file";
    case eUriBase64: return "base64";
    }
    return "<unknown>";
}

struct ResourceEntry {
    ResourceEntry() {}
    virtual ~ResourceEntry() {}

    std::atomic<uint32_t> version = 0;
    std::atomic<uint32_t> cast_mask = 0;  // a set bit means a cast is allowed
    std::atomic<uint32_t> cast_cache = 0; // a set bit means a cast for that bit position is known, cleared on version change
    rtti::type exact_type;
    uint32_t entry_id = nextResourceEntryId();
    IResourceBackend* backend = nullptr;
    void* data = nullptr;
    eResourceState state = eResourceInvalidState;
    std::atomic<int> ref_count = 0;
    std::string resource_id;
    eUriSchema schema = eUriNone;
    std::string resource_path;
    std::unique_ptr<byte_reader> reader;
    std::vector<char> loading_payload; // for base64 source and other embedded loads
    std::set<ResourceEntry*> dependents;

    void addRef() {
        ++ref_count;
    }
    void releaseRef() {
        assert(ref_count > 0);
        --ref_count;
    }

    virtual rtti::type getType() = 0;

    static uint32_t nextResourceEntryId();
};


template<typename RES_T>
struct TResourceEntry : public ResourceEntry {
    TResourceEntry() {}

    rtti::type getType() override {
        return rtti::type_get<RES_T>();
    }
};

