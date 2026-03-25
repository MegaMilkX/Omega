#pragma once

#include "resource_ref.auto.hpp"
#include "resource_entry.hpp"


[[cppi_tpl]];
template<typename RES_T>
class ResourceRef {
    ResourceEntry* entry = nullptr;
public:
    using resource_type = RES_T;

    ResourceRef() {}
    ResourceRef(ResourceEntry* entry)
    : entry(entry) {
        if (!entry) {
            return;
        }/*
        // TODO: I don't quite remember why's this here exactly
        if (entry->state != eResourcePresent) {
            assert(false);
            return;
        }*/
        if (type_get<RES_T>() != entry->getType()) {
            assert(false);
            entry = nullptr;
            return;
        }
        entry->addRef();
    }
    ResourceRef(const ResourceRef& other) {
        if (entry) {
            entry->releaseRef();
        }
        entry = other.entry;
        if(entry) {
            entry->addRef();
        }
    }
    ResourceRef(ResourceRef&& other) noexcept {
        if (entry) {
            entry->releaseRef();
        }
        entry = other.entry;
        other.entry = nullptr;
    }
    ~ResourceRef() {
        if (entry) {
            entry->releaseRef();
        }
    }

    void reset() {
        if (entry) {
            entry->releaseRef();
        }
        entry = nullptr;
    }

    ResourceRef& operator=(const ResourceRef& other) {
        if (entry) {
            entry->releaseRef();
        }
        entry = other.entry;
        if(entry) {
            entry->addRef();
        }
        return *this;
    }
    ResourceRef& operator=(ResourceRef&& other) {
        if (entry) {
            entry->releaseRef();
        }
        entry = other.entry;
        other.entry = nullptr;
        return *this;
    }

    const std::string& getResourceId() const {
        return entry->resource_id;
    }

    // Used only to add an id for an entry you've created yourself,
    // so that the ResourceRef can be serialized as a proper reference
    void _setResourceId(const std::string& id) {
        if(!entry) return;
        entry->resource_id = id;
    }

    RES_T* get() { return static_cast<RES_T*>(entry->data); }
    const RES_T* get() const { return static_cast<RES_T*>(entry->data); }

    RES_T* operator->() { return static_cast<RES_T*>(entry->data); }
    const RES_T* operator->() const { return static_cast<RES_T*>(entry->data); }
    RES_T& operator*() { return *static_cast<RES_T*>(entry->data); }
    const RES_T& operator*() const { return *static_cast<RES_T*>(entry->data); }
    operator bool() const {
        return entry != nullptr;
    }
};


template<typename T>
void type_write_json(nlohmann::json& j, const ResourceRef<T>& object) {
    if (!object) {
        j = nullptr;
        return;
    }
    j = object.getResourceId();
}
template<typename T>
void type_read_json(const nlohmann::json& j, ResourceRef<T>& object) {
    // Backward compatibility
    if (j.is_object()) {
        auto it_data = j.find("data");
        auto it_ref = j.find("ref");
        if (it_data != j.end()) {
            object = createResource<T>("");
            type_get<T>().deserialize_json(it_data.value(), object.get());
        } else if (it_ref != j.end()) {
            std::string ref_name = it_ref.value().get<std::string>();
            std::filesystem::path path = ref_name;
            path.replace_extension("");
            std::string resid = path.string();
            object = loadResource<T>(resid);
        } else {
            assert(false);
            object = nullptr;
        }
        return;
    }

    // ===
    if (j.is_null()) {
        object = nullptr;
        return;
    }
    if (!j.is_string()) {
        return;
    }
    std::string res_id = j.get<std::string>();
    // Temporary for backward compatibility
    {
        std::filesystem::path path = res_id;
        if(path.has_extension()) {
            path.replace_extension("");
            res_id = path.string();
        }
    }
    object = loadResource<T>(res_id);
}

