#pragma once

#include <concepts>
#include <string>
#include <vector>
#include <set>
#include <queue>
#include <unordered_map>
#include <stdint.h>

#include "common.hpp"
#include "property.hpp"
#include "type.hpp"
#include "type_property_desc.hpp"
#include "type_desc.hpp"
#include "type_desc_extender.hpp"
#include "meta_object.hpp"
#include "varying.hpp"

#include "math/gfxm.hpp"
#include "animation/curve.hpp"

#include "log/log.hpp"
#include "nlohmann/json.hpp"

namespace rtti {

#if defined _HAS_CXX17 || defined __cplusplus >= 201703L
template<class F, class... TN> using invoke_result_t = typename std::invoke_result_t<F, TN...>;
#else
template<class F, class... TN> using invoke_result_t = typename std::result_of_t<F(TN...)>;
#endif

// Index generator
type_id_t typeNextGuid();
template<typename T>
struct TYPE_INDEX_GENERATOR {
    static type_id_t guid() {
        static type_id_t guid = typeNextGuid();
        return guid;
    }
};
// ---------------

std::unordered_map<std::string, type>& get_type_name_map();

using type_desc_map_t = std::unordered_map<type_id_t, type_desc>;

template<typename TO_T>
int type_find_cast_path(const type_desc* tfrom, const type_desc::parent_info** path, int max_path_len, int at) {
    if (tfrom->id == type_get<TO_T>().id) {
        return at;
    }
    if (at == max_path_len) {
        assert(false && "Max inheritance depth reached before cast path found");
        return -1;
    }
    for (const auto& parent_info : tfrom->parent_types) {
        path[at] = &parent_info;
        int len = type_find_cast_path<TO_T>(parent_info.parent_type.get_desc(), path, max_path_len, at + 1);
        if (len == -1) {
            continue;
        }
        return len;        
    }
    return -1;
}
template<typename TO_T>
int type_find_cast_path(const type_desc* from, const type_desc::parent_info** path, int max_path_len) {
    return type_find_cast_path<TO_T>(from, path, max_path_len, 0);
}

template<typename BASE_T>
void* type_fix_base_pointer(const type_desc* tfrom, void* ptr) {
    constexpr int MAX_INHERITANCE_DEPTH = 16;
    const type_desc::parent_info* path[MAX_INHERITANCE_DEPTH] = { nullptr };
    int count = type_find_cast_path<BASE_T>(tfrom, path, MAX_INHERITANCE_DEPTH);
    if(count == -1) return nullptr; // Not a valid cast

    {
        // For debugging
        std::string str_chain = tfrom->name;
        for (int i = 0; i < count; ++i) {
            str_chain += " -> ";
            str_chain += path[i]->parent_type.get_name();
        }
        LOG_DBG("TYPE: cast chain: " << str_chain);
    }

    const type_desc* t_from = tfrom;
    for (int i = 0; i < count; ++i) {
        const auto& parent_info = path[i];
        const type_desc* t_to = parent_info->parent_type.get_desc();
        ptr = parent_info->pfn_static_upcast(ptr);
        t_from = t_to;
    }
    return ptr;
}

template<typename BASE_T>
BASE_T* type::construct_new() {
    auto desc = get_type_desc(*this);
    if (!desc->pfn_construct_new) {
        LOG_ERR("TYPE: " << get_name() << " has no constructor");
        assert(false);
        return nullptr;
    }
    void* ptr = desc->pfn_construct_new();
    ptr = type_fix_base_pointer<BASE_T>(desc, ptr);
    if (!ptr) {
        LOG_ERR("TYPE: construct_new: failed to convert " << get_name() << "* to " << type_get<BASE_T>().get_name() << "*, deleting");
        desc->pfn_destruct_delete(ptr);
        assert(false);
        return nullptr;
    }
    return static_cast<BASE_T*>(ptr);
}

template<typename O, typename T>
inline void type::set_property(const char* name, O* object, const T& value) {
    if (type_get<O>() != *this) {
        assert(false);
        return;
    }
    for (auto& prop : get_desc()->properties) {
        if (prop.name != name) {
            continue;
        }
        if (prop.t != type_get<T>()) {
            assert(false);
            return;
        }
        if (prop.fn_set) {
            prop.fn_set(object, (void*)&value);
        }
    }
}


template<typename T, typename = void>
struct smart_is_copy_constructible : std::is_copy_constructible<T> {};
template<typename T>
struct smart_is_copy_constructible<T, std::void_t<typename T::value_type>> : std::is_copy_constructible<typename T::value_type> {};

template<typename T>
constexpr bool smart_is_copy_constructible_v = smart_is_copy_constructible<T>::value;

template<typename T>
std::enable_if_t<std::is_abstract_v<unqualified_type<T>>, type> type_get();
template<typename T>
std::enable_if_t<!std::is_abstract_v<unqualified_type<T>> && !smart_is_copy_constructible_v<unqualified_type<T>>, type> type_get();
template<typename T>
std::enable_if_t<!std::is_abstract_v<unqualified_type<T>> && smart_is_copy_constructible_v<unqualified_type<T>>, type> type_get();
inline type type_get(const char* name);


}


#include "serialization.hpp"


namespace rtti {


template<typename T>
T* type_new_from_json(const nlohmann::json& j) {
    using namespace nlohmann;
    T* ptr = 0;

    if (!j.is_object()) {
        assert(false);
        return 0;
    }

    auto it = j.find("@class");
    if (it == j.end()) {
        assert(false);
        return 0;
    }
    const json& jclass = it.value();
    if (!jclass.is_string()) {
        assert(false);
        return 0;
    }
    std::string strclass = jclass.get<std::string>();
    type t = type_get(strclass.c_str());
    if (!t.is_valid()) {
        LOG_ERR("type_new_from_json(): " << strclass << " unknown type");
        assert(false);
        return 0;
    }
    if (t != type_get<T>() && !t.is_derived_from(type_get<T>())) {
        LOG_ERR(t.get_name() << " is not T and not derived from T");
        assert(false);
        return 0;
    }

    ptr = (T*)t.construct_new();

    t.deserialize_json(j, ptr);

    return ptr;
}

template<typename T>
T* type_new_from_json(const char* filepath) {
    using namespace nlohmann;

    FILE* f = fopen(filepath, "rb");
    if (!f) {
        assert(false);
        return 0;
    }

    std::string data;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    data.resize(sz);
    fseek(f, 0, SEEK_SET);
    size_t n_read = fread((void*)data.data(), sz, 1, f);
    if (n_read != 1) {
        fclose(f);
        assert(false);
        return 0;
    }
    fclose(f);

    json j;
    try {
        j = json::parse(data);
    } catch(const json::exception& ex) {
        LOG_ERR("json exception: " << ex.what());
        assert(false);
        return 0;
    }

    return type_new_from_json<T>(j);
}

template<typename T>
std::enable_if_t<std::is_abstract_v<unqualified_type<T>>, type> type_get() {
    {
        auto log = []()->int {
            LOG_DBG("TYPE: abstract: " << typeid(T).name());
            return 0;
            };
        static int i = log();
    }
    using UNQUALIFIED_T = unqualified_type<T>;

    extern type_desc_map_t& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid();

    auto& map = get_type_desc_map();
    auto it = map.find(guid);
    if (it == map.end()) {
        it = map.insert(std::make_pair(guid, type_desc())).first;
        it->second.id = guid;
        it->second.name = typeid(T).name();
        it->second.size = sizeof(T);
        it->second.pfn_construct = 0;
        it->second.pfn_destruct = 0;
        it->second.pfn_construct_new = 0;
        it->second.pfn_destruct_delete = 0;
        it->second.pfn_copy_construct = 0;

        type_desc_extender<UNQUALIFIED_T>::apply(it->second);
        // ?
        /*
        it->second.pfn_serialize_json = [](nlohmann::json& j, void* object) { type_write_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](nlohmann::json& j, void* object) { type_read_json(j, *(UNQUALIFIED_T*)object); };
        */
    }

    return type(guid);
}

template<typename T>
std::enable_if_t<!std::is_abstract_v<unqualified_type<T>> && !smart_is_copy_constructible_v<unqualified_type<T>>, type> type_get() {
    {
        auto log = []()->int {
            LOG_DBG("TYPE: non copy constructible: " << typeid(T).name());
            return 0;
            };
        static int i = log();
    }
    using UNQUALIFIED_T = unqualified_type<T>;

    extern type_desc_map_t& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid();

    auto& map = get_type_desc_map();
    auto it = map.find(guid);
    if (it == map.end()) {
        it = map.insert(std::make_pair(guid, type_desc())).first;
        it->second.id = guid;
        it->second.name = typeid(T).name();
        it->second.size = sizeof(T);
        it->second.is_pointer = std::is_pointer<UNQUALIFIED_T>();
        it->second.pfn_construct = [](void* object) {
            new ((UNQUALIFIED_T*)object)(UNQUALIFIED_T)();
        };
        it->second.pfn_destruct = [](void* object) {
            ((UNQUALIFIED_T*)object)->~UNQUALIFIED_T();
        };
        it->second.pfn_construct_new = []()->void* {
            return new UNQUALIFIED_T();
        };
        it->second.pfn_destruct_delete = [](void* ptr) {
            delete ((UNQUALIFIED_T*)ptr);
        };
        it->second.pfn_copy_construct = 0;

        it->second.pfn_serialize_json = [](nlohmann::json& j, const void* object) { type_write_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](const nlohmann::json& j, void* object) { type_read_json(j, *(UNQUALIFIED_T*)object); };

        type_desc_extender<UNQUALIFIED_T>::apply(it->second);
    }

    return type(guid);
}

template<typename T>
std::enable_if_t<!std::is_abstract_v<unqualified_type<T>> && smart_is_copy_constructible_v<unqualified_type<T>>, type> type_get() {
    {
        auto log = []()->int {
            LOG_DBG("TYPE: copy constructible: " << typeid(T).name());
            return 0;
        };
        static int i = log();
    }

    using UNQUALIFIED_T = unqualified_type<T>;

    extern type_desc_map_t& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid();
    
    auto& map = get_type_desc_map();
    auto it = map.find(guid);
    if (it == map.end()) {
        it = map.insert(std::make_pair(guid, type_desc())).first;
        it->second.id = guid;
        it->second.name = typeid(T).name();
        it->second.size = sizeof(T);
        it->second.is_pointer = std::is_pointer<UNQUALIFIED_T>();
        it->second.pfn_construct = [](void* object) {
            new ((UNQUALIFIED_T*)object)(UNQUALIFIED_T)();
        };
        it->second.pfn_destruct = [](void* object) {
            ((UNQUALIFIED_T*)object)->~UNQUALIFIED_T();
        };
        it->second.pfn_construct_new = []()->void* {
            return new UNQUALIFIED_T();
        };
        it->second.pfn_destruct_delete = [](void* ptr) {
            delete ((UNQUALIFIED_T*)ptr);
        };
        it->second.pfn_copy_construct = [](void* object, const void* other){
            new (object) UNQUALIFIED_T(*reinterpret_cast<const UNQUALIFIED_T*>(other));
        };

        it->second.pfn_serialize_json = [](nlohmann::json& j, const void* object) { type_write_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](const nlohmann::json& j, void* object) { type_read_json(j, *(UNQUALIFIED_T*)object); };

        type_desc_extender<UNQUALIFIED_T>::apply(it->second);
    }

    return type(guid);
}

inline type type_get(const char* name) {
    auto& map = get_type_name_map();
    auto it = map.find(name);
    if (it == map.end()) {
        return type(0);
    }
    return it->second;
}

void type_dbg_print();


inline type MetaObject::get_type() const { return type_get<decltype(*this)>(); }


#define TYPE_ENABLE() \
friend void cppiReflectInit(); \
template<typename T> \
friend class rtti::type_register; \
virtual rtti::type get_type() const { return rtti::type_get<decltype(*this)>(); }


template<typename T>
bool serializeJson(nlohmann::json& j, const T& object) {
    type_get<T>().serialize_json(j, (void*)&object);
    return true;
}
template<typename T>
bool deserializeJson(const nlohmann::json& j, const T& object) {
    type_get<T>().deserialize_json(j, (void*)&object);
    return true;
}


}

#include "type_register.hpp"
#include "enum_register.hpp"

