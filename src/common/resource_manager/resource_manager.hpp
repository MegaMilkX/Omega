#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include "loadable.hpp"

#include "resource_ref.hpp"
#include "resource_backend.hpp"

/*
class IResourceStorage {
public:
    virtual ~IResourceStorage() {}
};

class IResourceLoader {
public:
    virtual ~IResourceLoader() {}
};

class IResourceIdResolver {
public:
    virtual ~IResourceIdResolver() {}
};*/

template<typename RES_T>
class BasicResourceBackend : public IResourceBackend {
    std::map<std::string, std::unique_ptr<ResourceEntry>> entries;
public:
    ~BasicResourceBackend() {}
    ResourceEntry* findEntry(const std::string& resource_id) override {
        auto it = entries.find(resource_id);
        if (it == entries.end()) {
            return nullptr;
        }
        return it->second.get();
    }
    ResourceEntry* createEntry(const std::string& resource_id) override {
        auto it = entries.find(resource_id);
        if (it != entries.end()) {
            assert(false);
            return nullptr;
        }
        it = entries.insert(std::make_pair(resource_id, std::unique_ptr<ResourceEntry>(new TResourceEntry<RES_T>()))).first;
        return it->second.get();
    }
    void* load(byte_reader& reader) override {
        RES_T* res = new RES_T();
        if (!res->load(reader)) {
            delete res;
            return nullptr;
        }
        return res;
    }
    void* create() override {
        return new RES_T();
    }
    void release(void* ptr) override {
        delete static_cast<RES_T*>(ptr);
    }
    void collectGarbage() override {
        for (auto& kv : entries) {
            auto entry = kv.second.get();
            if (entry->data != nullptr && entry->ref_count == 0) {
                entry->backend->release(entry->data);
                entry->data = nullptr;
                entry->state = eResourceUnloaded;
            }
        }
    }
};

#include <algorithm>
#include <map>
#include <unordered_map>
#include "log/log.hpp"
#include "reflection/reflection.hpp"

#include "byte_reader/file_reader.hpp"

// TODO: Separate data providers per schema

class ResourceManager {
    std::unordered_map<type, std::unique_ptr<IResourceBackend>> backend_map;
    //std::map<std::string, std::unique_ptr<ResourceEntry>> resource_id_cache;
    std::vector<ResourceEntry*> loading_stack;
    std::vector<std::unique_ptr<ResourceEntry>> orphan_entries;

    eUriSchema convertUri(std::string& inout) {
        std::string& str = inout;
        size_t pos = str.find("://");
        if (pos == std::string::npos) {
            return eUriNone;
        }

        static std::map<std::string, eUriSchema> schema_map = {
            { "file", eUriFile }
        };

        std::string schema(str.begin(), str.begin() + pos);
        auto sch_it = schema_map.find(schema);
        if (sch_it == schema_map.end()) {
            return eUriError;
        }

        eUriSchema sch = sch_it->second;
        str = std::string(str.begin() + pos + 3, str.end());
        return sch;
    }

    template<typename RES_T>
    IResourceBackend* getBackend() {
        auto it = backend_map.find(type_get<RES_T>());
        if (it != backend_map.end()) {
            return it->second.get();
        }

        if constexpr (std::is_base_of_v<ILoadable, RES_T>) {
            it = backend_map.insert(
                std::make_pair(
                    type_get<RES_T>(),
                    std::unique_ptr<IResourceBackend>(new BasicResourceBackend<RES_T>())
                )
            ).first;
            return it->second.get();
        }

        LOG_ERR("Missing resource backend for " << type_get<RES_T>().get_name());
        assert(false);
        return nullptr;
    }

    template<typename RES_T>
    ResourceEntry* resolveResourceId(const std::string& resource_id) {
        IResourceBackend* backend = getBackend<RES_T>();
        if (!backend) {
            assert(false);
            return nullptr;
        }

        ResourceEntry* entry = backend->findEntry(resource_id);
        if (entry) {
            return entry;
        }
        /*
        auto it = resource_id_cache.find(resource_id);
        if (it != resource_id_cache.end()) {
            ResourceEntry* e = it->second.get();
            return e;
        }*/

        std::string resource_path = resource_id;
        eUriSchema schema = convertUri(resource_path);
        if (schema == eUriError) {
            assert(false);
            return nullptr;
        }

        entry = backend->createEntry(resource_id);
        entry->backend = backend;
        entry->resource_id = resource_id;
        entry->schema = schema;
        entry->resource_path = resource_path;
        entry->state = eResourceUnloaded;
        /*
        auto entry = new TResourceEntry<RES_T>();
        {
            std::unique_ptr<TResourceEntry<RES_T>> uptr_entry(entry);
            uptr_entry->backend = backend;
            uptr_entry->resource_id = resource_id;
            uptr_entry->schema = schema;
            uptr_entry->resource_path = resource_path;
            uptr_entry->state = eResourceUnloaded;
            resource_id_cache[resource_id] = std::move(uptr_entry);
        }*/

        if(schema == eUriNone) {
            entry->schema = eUriFile;

            extension_list extensions;
            if constexpr (has_static_get_extensions<RES_T>) {
                extensions = RES_T::get_extensions();
            } else {
                LOG_ERR("No supported extension list provided by " << type_get<RES_T>().get_name());
                entry->state = eResourceAbsent;
                return entry;
            }

            std::unique_ptr<file_reader> fr(new file_reader);
            for (int i = 0; i < extensions.size(); ++i) {
                extension ext = extensions[i];
                // TODO: check extension_to_string's result validity
                std::string fname = MKSTR(resource_id << "." << extension_to_string(ext));
                LOG_DBG(fname);
                if (!std::filesystem::exists(fname)) {
                    continue;
                }
                entry->resource_path = fname;
                return entry;
            }
            entry->state = eResourceAbsent;
        }

        return entry;
    }
public:
    static ResourceManager* get() {
        static std::unique_ptr<ResourceManager> resman(new ResourceManager());
        return resman.get();
    }

    void collectGarbage() {
        for (auto& kv : backend_map) {
            auto backend = kv.second.get();
            backend->collectGarbage();
        }/*
        for (auto& kv : resource_id_cache) {
            auto entry = kv.second.get();
            if (entry->data != nullptr && entry->ref_count == 0) {
                entry->backend->release(entry->data);
                entry->data = nullptr;
                entry->state = eResourceUnloaded;
            }
        }*/
        for (int i = 0; i < orphan_entries.size();) {
            auto e = orphan_entries[i].get();
            if (e->ref_count == 0) {
                e->backend->release(e->data);
                e->data = nullptr;
                orphan_entries.erase(orphan_entries.begin() + i);
                continue;
            }
            ++i;
        }
    }

    template<typename RES_T>
    ResourceRef<RES_T> load(ResourceEntry* entry) {
        LOG("RES: Loading " << uri_schema_to_string(entry->schema) << "://" << entry->resource_path);

        entry->state = eResourceLoading;
        loading_stack.push_back(entry);

        switch (entry->schema) {
        case eUriFile: {
            file_reader fr(entry->resource_path);
            if (!fr) {
                LOG_ERR("RES: File not found: " << entry->resource_path);
                loading_stack.pop_back();
                return nullptr;
            }
            void* res = entry->backend->load(fr);
            if (!res) {
                LOG_WARN("RES: Failed to load resource " << entry->resource_id);
                entry->data = nullptr;
                entry->state = eResourceAbsent;
                loading_stack.pop_back();
                return ResourceRef<RES_T>(nullptr);
            }
            entry->data = res;
            entry->state = eResourcePresent;
            ResourceRef<RES_T> ref(entry);
            loading_stack.pop_back();
            return ref;
        }
        }

        LOG_ERR("RES: Unsupported schema: " << uri_schema_to_string(entry->schema));

        entry->state = eResourceAbsent;
        assert(false);
        loading_stack.pop_back();
        return ResourceRef<RES_T>(nullptr);
    }

    template<typename RES_T>
    ResourceRef<RES_T> load(const std::string& resource_id) {        
        ResourceEntry* e = resolveResourceId<RES_T>(resource_id);
        if (!e) {
            LOG_ERR("RES: Failed to resolve resource id: " << resource_id);
            assert(false);
            return nullptr;
        }

        if (!loading_stack.empty()) {
            e->dependents.insert(loading_stack.back());
        }
        for (auto le : loading_stack) {
            if (le == e) {
                LOG_ERR("RES: Cyclic dependency detected, the resource will never be unloaded. Loading stack:");
                for (auto le : loading_stack) {
                    LOG_ERR(le->resource_id);
                }
            }
        }

        switch (e->state) {
        case eResourceInvalidState:
        case eResourceUnloaded:
            return load<RES_T>(e);
        case eResourcePresent:
            return ResourceRef<RES_T>(e);
        case eResourceAbsent:
            LOG_WARN("RES: Tried to load absent resource " << e->resource_id);
            return nullptr;
        case eResourceLoading:
            return ResourceRef<RES_T>(e);
        };

        LOG_ERR("RES: Invalid resource entry state: " << resource_state_to_string(e->state));
        assert(false);
        return nullptr;
    }

    template<typename RES_T>
    ResourceRef<RES_T> create(const std::string& resource_id) {
        IResourceBackend* backend = getBackend<RES_T>();
        if (!backend) {
            assert(false);
            return nullptr;
        }

        void* res = backend->create();
        if (!res) {
            assert(false);
            return nullptr;
        }

        auto entry = new TResourceEntry<RES_T>();
        {
            std::unique_ptr<TResourceEntry<RES_T>> uptr_entry(entry);
            uptr_entry->backend = backend;
            uptr_entry->resource_id = resource_id;
            uptr_entry->schema = eUriNone;
            uptr_entry->resource_path = "";
            uptr_entry->state = eResourcePresent;
            uptr_entry->data = res;
            orphan_entries.push_back(std::move(uptr_entry));
        }

        return ResourceRef<RES_T>(entry);
    }
};


template<typename RES_T>
ResourceRef<RES_T> loadResource(const std::string& resource_id) {
    return ResourceManager::get()->load<RES_T>(resource_id);
}

template<typename RES_T>
ResourceRef<RES_T> createResource(const std::string& resource_id) {
    return ResourceManager::get()->create<RES_T>(resource_id);
}
