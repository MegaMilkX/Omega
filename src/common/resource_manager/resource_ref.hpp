#pragma once

#include "resource_ref.auto.hpp"
#include "resource_entry.hpp"
#include "resource_root.hpp"

#include "reflection/type_desc_extender.hpp"

template<typename RES_T>
class ResourceRef;

template<typename RES_T>
ResourceRef<RES_T> loadResource(const std::string& resource_id);

[[cppi_class]];
class ResourceRefBase {
protected:
    ResourceEntry* entry = nullptr;
public:
    virtual ~ResourceRefBase() {}

    bool hasEntry() const { return entry; }

    const std::string& getResourceId() const {
        static std::string empty;
        if(!entry) return empty;
        return entry->resource_id;
    }

    virtual void replace(const std::string& res_id) = 0;
};

[[cppi_tpl]];
template<typename RES_T>
class ResourceRef : public ResourceRefBase {
    static_assert(
        !std::is_base_of_v<PolymorphicResourceRootBase, RES_T>
        || std::is_base_of_v<PolymorphicResourceRoot<RES_T>, RES_T>,
        "RES_T must be the root of a polymorphic resource inheritance tree, "
        "never declare ResourceRef<T> where T is a derived resource type"
    );
public:
    using resource_type = RES_T;

    ResourceRef() {}
    ResourceRef(ResourceEntry* entry) {
        this->entry = entry;
        if (!entry) {
            return;
        }/*
        // TODO: I don't quite remember why's this here exactly
        if (entry->state != eResourcePresent) {
            assert(false);
            return;
        }*/
        if (rtti::type_get<RES_T>() != entry->getType()) {
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

    // Used only to add an id for an entry you've created yourself,
    // so that the ResourceRef can be serialized as a proper reference
    void _setResourceId(const std::string& id) {
        if(!entry) return;
        entry->resource_id = id;
    }

    void replace(const std::string& res_id) override {
        *this = loadResource<RES_T>(res_id);
    }

    RES_T* get() { return entry ? static_cast<RES_T*>(entry->data) : nullptr; }
    const RES_T* get() const { return entry ? static_cast<RES_T*>(entry->data) : nullptr; }

    RES_T* operator->() { return static_cast<RES_T*>(entry->data); }
    const RES_T* operator->() const { return static_cast<RES_T*>(entry->data); }
    RES_T& operator*() { return *static_cast<RES_T*>(entry->data); }
    const RES_T& operator*() const { return *static_cast<RES_T*>(entry->data); }
    operator bool() const {
        return entry != nullptr;
    }
};
template<typename T>
struct rtti::type_desc_extender<ResourceRef<T>> {
    static void apply(rtti::type_desc& desc) {
        desc.is_wrapper = true;
        desc.wrapped_type = type_get<T>();
        desc.pfn_as_resource_ref_base = [](void* object)->ResourceRefBase* {
            return reinterpret_cast<ResourceRef<T>*>(object);
        };
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
            if (it_data.value().is_object()) {
                object = createResource<T>("");
                rtti::type_get<T>().deserialize_json(it_data.value(), object.get());
            } else {
                assert(it_data.value().is_string());
                object = loadResource<T>("base64://" + it_data.value().get<std::string>());
            }
        } else if (it_ref != j.end()) {
            assert(it_ref.value().is_string());
            std::string ref_name = it_ref.value().get<std::string>();
            std::filesystem::path path = ref_name;
            //path.replace_extension("");
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
            //path.replace_extension("");
            res_id = path.string();
        }
    }
    object = loadResource<T>(res_id);
}

