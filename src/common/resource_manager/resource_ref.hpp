#pragma once

#include "resource_ref.auto.hpp"
#include <assert.h>
#include "resource_entry.hpp"
#include "resource_root.hpp"

#include "reflection/type_desc_extender.hpp"

#include "log/log.hpp"


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
    uint32_t entryId() const { return entry ? entry->entry_id : 0; }

    virtual void replace(const std::string& res_id) = 0;
};

[[cppi_tpl]];
template<typename RES_T>
class ResourceRef : public ResourceRefBase {
    template<typename> friend class ResourceRef;

    mutable RES_T* cached_ptr = nullptr;
    mutable uint32_t entry_version = 0;

    template<typename RES_U>
    static constexpr bool convertible_from
        = std::is_convertible_v<RES_U*, RES_T*>
        && std::is_same_v<ResourceFamilyRoot_t<RES_U>, ResourceFamilyRoot_t<RES_T>>;
public:
    using resource_type = RES_T;

    ResourceRef() {}

    template<
        typename RES_U,
        typename = std::enable_if_t<convertible_from<RES_U>>
    > ResourceRef(const ResourceRef<RES_U>& other) {
        entry = other.entry;
        cached_ptr = static_cast<RES_T*>(other.cached_ptr);
        entry_version = other.entry_version;
        if(entry) {
            entry->addRef();
        }
    }

    ResourceRef(ResourceEntry* entry) {
        this->entry = entry;
        if (!entry) {
            return;
        }

        if (rtti::type_get<ResourceFamilyRoot_t<RES_T>>() != entry->getType()) {
            assert(false);
            this->entry = nullptr;
            return;
        }
        entry->addRef();
        deref_slow();
    }
    ResourceRef(const ResourceRef& other) {
        entry = other.entry;
        cached_ptr = other.cached_ptr;
        entry_version = other.entry_version;
        if(entry) {
            entry->addRef();
        }
    }
    ResourceRef(ResourceRef&& other) noexcept {
        entry = other.entry;
        cached_ptr = other.cached_ptr;
        entry_version = other.entry_version;
        other.entry = nullptr;
        other.cached_ptr = nullptr;
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
        cached_ptr = nullptr;
        entry_version = 0;
    }
    
    template<
        typename RES_U,
        typename = std::enable_if_t<convertible_from<RES_U>>
    > ResourceRef& operator=(const ResourceRef<RES_U>& other) {
        if (entry) {
            entry->releaseRef();
        }
        entry = other.entry;
        cached_ptr = static_cast<RES_T*>(other.cached_ptr);
        entry_version = other.entry_version;
        if(entry) {
            entry->addRef();
        }
        return *this;
    }
    template<
        typename RES_U,
        typename = std::enable_if_t<convertible_from<RES_U>>
    > ResourceRef& operator=(ResourceRef<RES_U>&& other) {
        if (entry) {
            entry->releaseRef();
        }
        entry = other.entry;
        cached_ptr = static_cast<RES_T*>(other.cached_ptr);
        entry_version = other.entry_version;
        other.entry = nullptr;
        other.cached_ptr = nullptr;
        return *this;
    }
    ResourceRef& operator=(const ResourceRef& other) {
        if (entry) {
            entry->releaseRef();
        }
        entry = other.entry;
        cached_ptr = other.cached_ptr;
        entry_version = other.entry_version;
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
        cached_ptr = other.cached_ptr;
        entry_version = other.entry_version;
        other.entry = nullptr;
        other.cached_ptr = nullptr;
        return *this;
    }

    // Used only to set an id for an entry you've created yourself,
    // so that the ResourceRef can be serialized as a proper reference
    void _setResourceId(const std::string& id) {
        if(!entry) return;
        entry->resource_id = id;
    }

    void replace(const std::string& res_id) override {
        *this = loadResource<RES_T>(res_id);
    }

    bool isReady() const {
        if(!entry) return false;
        return entry->state == eResourcePresent;
    }

    RES_T* deref_slow() const {
        if constexpr (std::is_base_of_v<PolymorphicResourceRootBase, RES_T>) {
            using ROOT_T = typename RES_T::ResourceRootType;
            const uint32_t type_bit = uint32_t(1) << getResourceFamilyIndex<ROOT_T, RES_T>();
            
            entry_version = entry->version;
            
            uint32_t cast_cache = entry->cast_cache;
            uint32_t cast_mask = entry->cast_mask;
            bool cast_touched = cast_cache & type_bit;
            if (cast_touched) {
                if (cast_mask & type_bit) {
                    cached_ptr = static_cast<RES_T*>(
                        static_cast<ROOT_T*>(entry->data)
                    );
                } else {
                    cached_ptr = nullptr;
                }
            } else {
                LOG_DBG("deref_slow: " << entry->version << ", " << entry->data << ", " << entry->resource_id << ", " << entry->resource_path);
                cached_ptr = dynamic_cast<RES_T*>(
                    static_cast<ROOT_T*>(entry->data)
                );
                if (cached_ptr) {
                    entry->cast_mask |= type_bit;
                }
                entry->cast_cache |= type_bit;
            }

            return cached_ptr;
        } else {
            entry_version = entry->version;
            cached_ptr = static_cast<RES_T*>(entry->data);
            return cached_ptr;
        }
    }
    RES_T* deref() const {
        uint32_t v = entry->version.load(std::memory_order_acquire);
        if(v == entry_version) [[likely]] return cached_ptr;
        return deref_slow();
    }
    RES_T* get() { return entry ? deref() : nullptr; }
    const RES_T* get() const { return entry ? deref() : nullptr; }

    RES_T* operator->() { return deref(); }
    const RES_T* operator->() const { return deref(); }
    RES_T& operator*() { return *deref(); }
    const RES_T& operator*() const { return *deref(); }
    operator bool() const { return entry != nullptr; }
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

