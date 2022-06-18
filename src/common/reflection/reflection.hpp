#pragma once

#include <string>
#include <vector>
#include <set>
#include <queue>
#include <stdint.h>

#include "handle/hshared.hpp"

#include "math/gfxm.hpp"
#include "resource/resource.hpp"

#include "log/log.hpp"
#include "nlohmann/json.hpp"

template<typename T>
using base_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

// Index generator
uint64_t typeNextGuid();
template<typename T>
struct TYPE_INDEX_GENERATOR {
    static uint64_t guid;
};
template<typename T>
uint64_t TYPE_INDEX_GENERATOR<T>::guid = typeNextGuid();
// ---------------

struct type {
    uint64_t guid;

    type()
        : guid(0) {}
    type(uint64_t guid)
        : guid(guid) {}

    size_t      get_size() const;
    const char* get_name() const;

    bool is_pointer() const;
    bool is_copy_constructible() const;

    bool is_derived_from(type other) const;

    void  construct(void* ptr);
    void  destruct(void* ptr);
    void* construct_new();
    void  destruct_delete(void* ptr);
    void  copy_construct(void* ptr, const void* other);

    void serialize_json(nlohmann::json& j, void* object);
    void deserialize_json(nlohmann::json& j, void* object);

    void dbg_print();

    bool operator==(const type& other) const {
        return guid == other.guid;
    }
    bool operator!=(const type& other) const {
        return guid != other.guid;
    }
    operator bool() const {
        return (*this) != type(0);
    }
};
template<>
struct std::hash<type> {
    size_t operator()(const type& t) const {
        return std::hash<uint64_t>()(t.guid);
    }
};

struct type_property_desc {
    type t;
    std::string name;
    
    std::function<void*(void*)> fn_get_ptr;
    std::function<void(void*, void*)> fn_get_value;
    //std::function<void(void*, void*)> fn_setter;
    //std::function<void(void*, void*)> fn_getter;

    std::function<void(void*, nlohmann::json&)> fn_serialize_json;
    std::function<void(void*, nlohmann::json&)> fn_deserialize_json;
};
struct type_desc {
    uint64_t guid;
    size_t size;
    std::string name;
    std::set<type> parent_types;
    std::vector<type_property_desc> properties;

    bool is_pointer = false;

    void(*pfn_construct)(void* object) = 0;
    void(*pfn_destruct)(void* object) = 0;
    void*(*pfn_construct_new)() = 0;
    void (*pfn_destruct_delete)(void* object) = 0;
    void(*pfn_copy_construct)(void* object, const void* other) = 0;

    void(*pfn_serialize_json)(nlohmann::json& j, void* object) = 0;
    void(*pfn_deserialize_json)(nlohmann::json& j, void* object) = 0;

    void(*pfn_custom_serialize_json)(nlohmann::json&, void*) = 0;
    void(*pfn_custom_deserialize_json)(nlohmann::json&, void*) = 0;
};

inline size_t      type::get_size() const {
    extern type_desc* get_type_desc(type t);
    auto desc = get_type_desc(*this);
    return desc->size;
}
inline const char* type::get_name() const {
    extern type_desc* get_type_desc(type t);
    auto desc = get_type_desc(*this);
    return desc->name.c_str();
}
inline bool type::is_pointer() const {
    extern type_desc* get_type_desc(type t);
    auto desc = get_type_desc(*this);
    return desc->is_pointer;
}
inline bool type::is_copy_constructible() const {
    extern type_desc* get_type_desc(type t);
    auto desc = get_type_desc(*this);
    return desc->pfn_copy_construct != nullptr;
}
inline bool type::is_derived_from(type other) const {
    extern type_desc* get_type_desc(type t);

    type current_type = *this;
    std::queue<type> type_q;
    while (current_type) {
        auto desc = get_type_desc(current_type);
        for (auto& parent : desc->parent_types) {
            type_q.push(parent);
        }

        if (current_type == other) {
            return true;
        }

        if (type_q.empty()) {
            current_type = type(0);
        } else {
            current_type = type_q.front();
            type_q.pop();
        }
    }

    return false;
}
inline void type::dbg_print() {
    extern type_desc* get_type_desc(type t);
    auto desc = get_type_desc(*this);
    LOG_DBG(desc->name << "(" << desc->guid << ")");
    for (int i = 0; i < desc->properties.size(); ++i) {
        LOG_DBG("\t" << desc->properties[i].name << "(" << desc->properties[i].t.get_name() << ")");
    }
}


class varying {
    std::vector<unsigned char> buffer;
    type t = type(0);

public:
    ~varying() {
        clear();
    }

    void clear() {
        if (t == type(0)) {
            return;
        }
        if (!t.is_pointer()) {
            t.destruct(buffer.data());
        }
        buffer.clear();
    }

    type get_type() const { return t; }

    template<typename T>
    std::enable_if_t<!std::is_pointer<T>::value, void> set(const T& value) {
        clear();

        t = type_get<base_type<T>>();
        buffer.resize(t.get_size());
        t.construct(buffer.data());
    }
    template<typename T>
    std::enable_if_t<std::is_pointer<T>::value, void> set(T pointer) {
        clear();
        
        t = type_get<T>();
        buffer.resize(sizeof(void*));
        (*(void**)buffer.data()) = (void*)pointer;
    }

    varying& operator=(const varying& other) {
        clear();
        if (other.get_type().is_pointer()) {
            t = other.t;
            buffer = other.buffer;
        } else if(t.is_copy_constructible()) {
            t = other.t;
            buffer.resize(t.get_size());
            t.copy_construct(buffer.data(), other.buffer.data());
        }
    }
};


template<typename T>
void type_serialize_json(nlohmann::json& j, const T& object) { j = nlohmann::json::object(); }
template<> inline void type_serialize_json(nlohmann::json& j, const bool& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const signed char& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const unsigned char& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const char& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const wchar_t& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const char16_t& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const char32_t& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const short& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const unsigned short& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const int& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const unsigned& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const long& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const unsigned long& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const long long& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const unsigned long long& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const float& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const double& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const long double& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const std::string& object) { j = object; }
template<> inline void type_serialize_json(nlohmann::json& j, const gfxm::vec2& object) { j = nlohmann::json::array({ object.x, object.y }); }
template<> inline void type_serialize_json(nlohmann::json& j, const gfxm::vec3& object) { j = nlohmann::json::array({ object.x, object.y, object.z }); }
template<> inline void type_serialize_json(nlohmann::json& j, const gfxm::vec4& object) { j = nlohmann::json::array({ object.x, object.y, object.z, object.w }); }
template<> inline void type_serialize_json(nlohmann::json& j, const gfxm::quat& object) { j = nlohmann::json::array({ object.x, object.y, object.z, object.w }); }
template<> inline void type_serialize_json(nlohmann::json& j, const gfxm::mat3& object) { j = nlohmann::json::array({ object[0][0], object[0][1], object[0][2], object[1][0], object[1][1], object[1][2], object[2][0], object[2][1], object[2][2] }); }
template<> inline void type_serialize_json(nlohmann::json& j, const gfxm::mat4& object) { j = nlohmann::json::array({ object[0][0], object[0][1], object[0][2], object[0][3], object[1][0], object[1][1], object[1][2], object[1][3], object[2][0], object[2][1], object[2][2], object[2][3], object[3][0], object[3][1], object[3][2], object[3][3] }); }
template<> inline void type_serialize_json(nlohmann::json& j, const type& object) { j = object.get_name(); }
template<typename T>
void type_serialize_json(nlohmann::json& j, const std::vector<T>& object) {
    j = nlohmann::json::array();
    if (object.empty()) {
        return;
    }
    for (int i = 0; i < object.size(); ++i) {
        nlohmann::json je;
        type_get<T>().serialize_json(je, const_cast<T*>(&object[i]));
        j.push_back(je);
    }
}
template<typename K, typename V>
void type_serialize_json(nlohmann::json& j, const std::unordered_map<K, V>& object) {
    j = nlohmann::json::array();
    if (object.empty()) {
        return;
    }
    for (auto& kv : object) {
        nlohmann::json je = nlohmann::json::array();
        nlohmann::json jk;
        nlohmann::json jv;
        type_get<K>().serialize_json(jk, const_cast<K*>(&kv.first));
        type_get<V>().serialize_json(jv, const_cast<V*>(&kv.second));
        je.push_back(jk);
        je.push_back(jv);
        j.push_back(je);
    }
}
template<typename K, typename V>
void type_serialize_json(nlohmann::json& j, const std::map<K, V>& object) {
    j = nlohmann::json::array();
    if (object.empty()) {
        return;
    }
    for (auto& kv : object) {
        nlohmann::json je = nlohmann::json::array();
        nlohmann::json jk;
        nlohmann::json jv;
        type_get<K>().serialize_json(jk, const_cast<K*>(&kv.first));
        type_get<V>().serialize_json(jv, const_cast<V*>(&kv.second));
        je.push_back(jk);
        je.push_back(jv);
        j.push_back(je);
    }
}
template<typename T>
void type_serialize_json(nlohmann::json& j, const std::unique_ptr<T>& object) {
    if (!object) {
        j = nullptr;
        return;
    }
    type actual_type = object->get_type();
    j["type"] = actual_type.get_name();
    actual_type.serialize_json(j["data"], object.get());
}
template<typename T>
void type_serialize_json(nlohmann::json& j, const HSHARED<T>& object) {
    if (!object) {
        j = nullptr;
        return;
    }
    j = nlohmann::json::object();
    std::string ref_name = object.getReferenceName();
    if (ref_name.empty()) {
        type_get<T>().serialize_json(j["data"], const_cast<T*>(object.get()));
    } else {
        j["ref"] = ref_name;
    }
}


template<typename T>
void type_deserialize_json(nlohmann::json& j, T& object) { /* Do nothing */ }
template<> inline void type_deserialize_json(nlohmann::json& j, bool& object) { object = j.get<bool>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, signed char& object) { object = j.get<signed char>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, unsigned char& object) { object = j.get<unsigned char>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, char& object) { object = j.get<char>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, wchar_t& object) { object = j.get<wchar_t>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, char16_t& object) { object = j.get<char16_t>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, char32_t& object) { object = j.get<char32_t>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, short& object) { object = j.get<short>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, unsigned short& object) { object = j.get<unsigned short>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, int& object) { object = j.get<int>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, unsigned& object) { object = j.get<unsigned>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, long& object) { object = j.get<long>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, unsigned long& object) { object = j.get<unsigned long>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, long long& object) { object = j.get<long long>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, unsigned long long& object) { object = j.get<unsigned long long>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, float& object) { object = j.get<float>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, double& object) { object = j.get<double>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, long double& object) { object = j.get<long double>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, std::string& object) { object = j.get<std::string>(); }
template<> inline void type_deserialize_json(nlohmann::json& j, gfxm::vec2& object) { if (!j.is_array() || j.size() != 2) return; object = gfxm::vec2(j[0].get<float>(), j[1].get<float>()); }
template<> inline void type_deserialize_json(nlohmann::json& j, gfxm::vec3& object) { if (!j.is_array() || j.size() != 3) return; object = gfxm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>()); }
template<> inline void type_deserialize_json(nlohmann::json& j, gfxm::vec4& object) { if (!j.is_array() || j.size() != 4) return; object = gfxm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); }
template<> inline void type_deserialize_json(nlohmann::json& j, gfxm::quat& object) { if (!j.is_array() || j.size() != 4) return; object = gfxm::quat(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); }
template<> inline void type_deserialize_json(nlohmann::json& j, gfxm::mat3& object) { if (!j.is_array() || j.size() != 9) return; object = gfxm::mat3(gfxm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>()), gfxm::vec3(j[3].get<float>(), j[4].get<float>(), j[5].get<float>()), gfxm::vec3(j[6].get<float>(), j[7].get<float>(), j[8].get<float>())); }
template<> inline void type_deserialize_json(nlohmann::json& j, gfxm::mat4& object) { if (!j.is_array() || j.size() != 16) return; object = gfxm::mat4(gfxm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()), gfxm::vec4(j[4].get<float>(), j[5].get<float>(), j[6].get<float>(), j[7].get<float>()), gfxm::vec4(j[8].get<float>(), j[9].get<float>(), j[10].get<float>(), j[11].get<float>()), gfxm::vec4(j[12].get<float>(), j[13].get<float>(), j[14].get<float>(), j[15].get<float>())); }
template<> inline void type_deserialize_json(nlohmann::json& j, type& object) {
    type type_get(const char* name);
    if (!j.is_string()) object = type(0);
    object = type_get(j.get<std::string>().c_str());
}
template<typename T>
void type_deserialize_json(nlohmann::json& j, std::vector<T>& object) {
    if (j.is_null()) {
        return;
    }
    assert(j.is_array());
    size_t sz = j.size();
    object.resize(sz);
    for (int i = 0; i < sz; ++i) {
        type_get<T>().deserialize_json(j[i], &object[i]);
    }
}
template<typename K, typename V>
void type_deserialize_json(nlohmann::json& j, std::unordered_map<K, V>& object) {
    if (j.is_null()) { return; }
    if (!j.is_array()) {
        assert(false);
        return;
    }
    size_t sz = j.size();
    object.reserve(sz);
    for (int i = 0; i < sz; ++i) {
        auto& jkv = j[i];
        if (!jkv.is_array() || jkv.size() != 2) {
            assert(false);
            continue;
        }
        auto& jk = jkv[0];
        auto& jv = jkv[1];
        K key;
        type_get<K>().deserialize_json(jk, &key);
        type_get<V>().deserialize_json(jv, &object[key]);
    }
}
template<typename K, typename V>
void type_deserialize_json(nlohmann::json& j, std::map<K, V>& object) {
    if (j.is_null()) { return; }
    if (!j.is_array()) {
        assert(false);
        return;
    }
    size_t sz = j.size();
    for (int i = 0; i < sz; ++i) {
        auto& jkv = j[i];
        if (!jkv.is_array() || jkv.size() != 2) {
            assert(false);
            continue;
        }
        auto& jk = jkv[0];
        auto& jv = jkv[1];
        K key;
        type_get<K>().deserialize_json(jk, &key);
        type_get<V>().deserialize_json(jv, &object[key]);
    }
}
template<typename T>
void type_deserialize_json(nlohmann::json& j, std::unique_ptr<T>& object) {
    if (j.is_null()) {
        object.reset();
        return;
    }
    if (!j.is_object()) {
        assert(false);
        return;
    }
    auto it_type = j.find("type");
    auto it_data = j.find("data");
    if (it_type == j.end() || it_data == j.end()) {
        assert(false);
        return;
    }
    if (!it_type.value().is_string()) {
        assert(false);
        return;
    }
    std::string type_name = it_type.value().get<std::string>();
    nlohmann::json& jdata = it_data.value();

    type t = type_get(type_name.c_str());
    bool valid = (t == type_get<T>()) || t.is_derived_from(type_get<T>());
    if (!valid) {
        object.reset();
        return;
    }
    void* ptr = t.construct_new();
    t.deserialize_json(jdata, ptr);
    object.reset((T*)ptr);
}
template<typename T>
void type_deserialize_json(nlohmann::json& j, HSHARED<T>& object) {
    if (j.is_null()) {
        object.reset();
        return;
    }
    if (!j.is_object()) {
        assert(false);
        return;
    }
    auto it_data = j.find("data");
    auto it_ref = j.find("ref");
    if (it_data != j.end()) {
        object.reset(HANDLE_MGR<T>::acquire());
        type_get<T>().deserialize_json(it_data.value(), object.get());
    } else if(it_ref != j.end()) {
        std::string ref_name = it_ref.value().get<std::string>();
        object = resGet<T>(ref_name.c_str());
    } else {
        assert(false);
        return;
    }
}


template<typename T>
std::enable_if_t<std::is_abstract<base_type<T>>::value, type> type_get() {
    using UNQUALIFIED_T = base_type<T>;

    extern std::unordered_map<uint64_t, type_desc>& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid;

    auto& map = get_type_desc_map();
    auto it = map.find(guid);
    if (it == map.end()) {
        it = map.insert(std::make_pair(guid, type_desc())).first;
        it->second.guid = guid;
        it->second.name = typeid(T).name();
        it->second.size = sizeof(T);
        it->second.pfn_construct = 0;
        it->second.pfn_destruct = 0;
        it->second.pfn_construct_new = 0;
        it->second.pfn_destruct_delete = 0;
        it->second.pfn_copy_construct = 0;

        // ?
        /*
        it->second.pfn_serialize_json = [](nlohmann::json& j, void* object) { type_serialize_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](nlohmann::json& j, void* object) { type_deserialize_json(j, *(UNQUALIFIED_T*)object); };
        */
    }

    return type(guid);
}

template<typename T>
std::enable_if_t<!std::is_abstract<base_type<T>>::value && !std::is_copy_constructible<base_type<T>>::value, type> type_get() {
    using UNQUALIFIED_T = base_type<T>;

    extern std::unordered_map<uint64_t, type_desc>& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid;

    auto& map = get_type_desc_map();
    auto it = map.find(guid);
    if (it == map.end()) {
        it = map.insert(std::make_pair(guid, type_desc())).first;
        it->second.guid = guid;
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

        it->second.pfn_serialize_json = [](nlohmann::json& j, void* object) { type_serialize_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](nlohmann::json& j, void* object) { type_deserialize_json(j, *(UNQUALIFIED_T*)object); };
    }

    return type(guid);
}

template<typename T>
std::enable_if_t<!std::is_abstract<base_type<T>>::value && std::is_copy_constructible<base_type<T>>::value, type> type_get() {
    using UNQUALIFIED_T = base_type<T>;

    extern std::unordered_map<uint64_t, type_desc>& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid;
    
    auto& map = get_type_desc_map();
    auto it = map.find(guid);
    if (it == map.end()) {
        it = map.insert(std::make_pair(guid, type_desc())).first;
        it->second.guid = guid;
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
        };/* // TODO:
        it->second.pfn_copy_construct = [](void* object, const void* other) {
            new ((UNQUALIFIED_T*)object)(UNQUALIFIED_T)(*(UNQUALIFIED_T*)other);
        };*/

        it->second.pfn_serialize_json = [](nlohmann::json& j, void* object) { type_serialize_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](nlohmann::json& j, void* object) { type_deserialize_json(j, *(UNQUALIFIED_T*)object); };
    }

    return type(guid);
}

inline type type_get(const char* name) {
    extern std::unordered_map<std::string, type>& get_type_name_map();
    auto& map = get_type_name_map();
    auto it = map.find(name);
    if (it == map.end()) {
        return type(0);
    }
    return it->second;
}

void type_dbg_print();

#define TYPE_ENABLE_BASE() \
virtual type get_type() const { return type_get<decltype(*this)>(); }
#define TYPE_ENABLE(BASE) \
        type get_type() const override { return type_get<decltype(*this)>(); }

template<typename T>
class type_register {
    std::string name;
    std::set<type> parents;
    std::vector<type_property_desc> properties;
    void(*pfn_custom_serialize_json)(nlohmann::json&, void*) = 0;
    void(*pfn_custom_deserialize_json)(nlohmann::json&, void*) = 0;
public:
    type_register(const char* name)
    : name(name) {
        // TODO
    }
    ~type_register() {
        extern type_desc* get_type_desc(type t);
        auto desc = get_type_desc(type_get<T>());
        desc->name = name;
        desc->parent_types = parents;
        for (int i = 0; i < properties.size(); ++i) {
            desc->properties.push_back(properties[i]);
        }
        desc->pfn_custom_serialize_json = pfn_custom_serialize_json;
        desc->pfn_custom_deserialize_json = pfn_custom_deserialize_json;

        {
            extern std::unordered_map<std::string, type>& get_type_name_map();
            auto& map = get_type_name_map();
            map[name] = type_get<T>();
        }
    }
    template<typename PARENT_T>
    type_register<T>& parent() {
        parents.insert(type_get<PARENT_T>());
        return *this;
    }
    template<typename MEMBER_T>
    type_register<T>& prop(const char* name, MEMBER_T T::*member) {
        type_property_desc prop_desc;
        prop_desc.name = name;
        prop_desc.t = type_get<MEMBER_T>();
        prop_desc.fn_get_ptr = [member](void* object)->void* {
            return &(((T*)object)->*member);
        };
        prop_desc.fn_get_value = nullptr;

        prop_desc.fn_serialize_json = [member](void* object, nlohmann::json& j) {
            type_get<MEMBER_T>().serialize_json(j, &(((T*)object)->*member));
        };
        prop_desc.fn_deserialize_json = [member](void* object, nlohmann::json& j) {
            type_get<MEMBER_T>().deserialize_json(j, &(((T*)object)->*member));
        };
        properties.push_back(prop_desc);
        return *this;
    }
    // const getter, reference return type
    template<typename GETTER_T, typename SETTER_T>
    std::enable_if_t<std::is_reference<GETTER_T>::value, type_register<T>&> prop(const char* name, GETTER_T(T::*getter)() const, void(T::*setter)(SETTER_T)) {
        static_assert(std::is_same<base_type<GETTER_T>, base_type<SETTER_T>>::value, "property setter and getter types must be the same");
        type_property_desc prop_desc;
        prop_desc.name = name;
        prop_desc.t = type_get<base_type<GETTER_T>>();
        prop_desc.fn_get_ptr = [getter](void* object)->void* {
            return &(((T*)object)->*getter)();
        };
        prop_desc.fn_get_value = nullptr;

        prop_desc.fn_serialize_json = [getter](void* object, nlohmann::json& j) {
            auto&& temporary = (((T*)object)->*getter)();
            type_get<base_type<GETTER_T>>().serialize_json(j, &temporary);
        };
        prop_desc.fn_deserialize_json = [setter](void* object, nlohmann::json& j) {
            type member_type = type_get<base_type<SETTER_T>>();
            std::vector<unsigned char> buf(member_type.get_size());
            member_type.construct(buf.data());
            member_type.deserialize_json(j, buf.data());
            (((T*)object)->*setter)(*(base_type<SETTER_T>*)buf.data());
            member_type.destruct(buf.data());
        };
        properties.push_back(prop_desc);
        return *this;
    }
    // non-const getter, reference return type
    template<typename GETTER_T, typename SETTER_T>
    std::enable_if_t<std::is_reference<GETTER_T>::value, type_register<T>&> prop(const char* name, GETTER_T(T::*getter)(), void(T::*setter)(SETTER_T)) {
        static_assert(std::is_same<base_type<GETTER_T>, base_type<SETTER_T>>::value, "property setter and getter types must be the same");
        type_property_desc prop_desc;
        prop_desc.name = name;
        prop_desc.t = type_get<base_type<GETTER_T>>();
        prop_desc.fn_get_ptr = [getter](void* object)->void* {
            return &(((T*)object)->*getter)();
        };
        prop_desc.fn_get_value = nullptr;

        prop_desc.fn_serialize_json = [getter](void* object, nlohmann::json& j) {
            auto&& temporary = (((T*)object)->*getter)();
            type_get<base_type<GETTER_T>>().serialize_json(j, &temporary);
        };
        prop_desc.fn_deserialize_json = [setter](void* object, nlohmann::json& j) {
            type member_type = type_get<base_type<SETTER_T>>();
            std::vector<unsigned char> buf(member_type.get_size());
            member_type.construct(buf.data());
            member_type.deserialize_json(j, buf.data());
            (((T*)object)->*setter)(*(base_type<SETTER_T>*)buf.data());
            member_type.destruct(buf.data());
        };
        properties.push_back(prop_desc);
        return *this;
    }
    // const getter, value return type
    template<typename GETTER_T, typename SETTER_T>
    std::enable_if_t<!std::is_reference<GETTER_T>::value, type_register<T>&> prop(const char* name, GETTER_T(T::*getter)(), void (T::*setter)(SETTER_T)) {
        static_assert(std::is_same<base_type<GETTER_T>, base_type<SETTER_T>>::value, "property setter and getter types must be the same");
        type_property_desc prop_desc;
        prop_desc.name = name;
        prop_desc.t = type_get<base_type<GETTER_T>>();
        prop_desc.fn_get_ptr = 0;
        prop_desc.fn_get_value = [getter](void* object, void* pvalue) {
            (*(base_type<GETTER_T>*)pvalue) = (((T*)object)->*getter)();
        };

        prop_desc.fn_serialize_json = [getter](void* object, nlohmann::json& j) {
            auto&& temporary = (((T*)object)->*getter)();
            type_get<base_type<GETTER_T>>().serialize_json(j, &temporary);
        };
        prop_desc.fn_deserialize_json = [setter](void* object, nlohmann::json& j) {
            type member_type = type_get<base_type<SETTER_T>>();
            std::vector<unsigned char> buf(member_type.get_size());
            member_type.construct(buf.data());
            member_type.deserialize_json(j, buf.data());
            (((T*)object)->*setter)(*(base_type<SETTER_T>*)buf.data());
            member_type.destruct(buf.data());
        };
        properties.push_back(prop_desc);
        return *this;
    }
    // non-const getter, value return type
    template<typename GETTER_T, typename SETTER_T>
    std::enable_if_t<!std::is_reference<GETTER_T>::value, type_register<T>&> prop(const char* name, GETTER_T(T::*getter)() const, void (T::*setter)(SETTER_T)) {
        static_assert(std::is_same < base_type<GETTER_T>, base_type<SETTER_T>>::value, "property setter and getter types must be the same");
        type_property_desc prop_desc;
        prop_desc.name = name;
        prop_desc.t = type_get<base_type<GETTER_T>>();
        prop_desc.fn_get_ptr = 0;
        prop_desc.fn_get_value = [getter](void* object, void* pvalue) {
            (*(base_type<GETTER_T>*)pvalue) = (((T*)object)->*getter)();
        };

        prop_desc.fn_serialize_json = [getter](void* object, nlohmann::json& j) {
            auto&& temporary = (((T*)object)->*getter)();
            type_get<base_type<GETTER_T>>().serialize_json(j, &temporary);
        };
        prop_desc.fn_deserialize_json = [setter](void* object, nlohmann::json& j) {
            type member_type = type_get<base_type<SETTER_T>>();
            std::vector<unsigned char> buf(member_type.get_size());
            member_type.construct(buf.data());
            member_type.deserialize_json(j, buf.data());
            (((T*)object)->*setter)(*(base_type<SETTER_T>*)buf.data());
            member_type.destruct(buf.data());
        };
        properties.push_back(prop_desc);
        return *this;
    }

    type_register<T>& custom_serialize_json(void(*pfn_custom_serialize_json)(nlohmann::json&, void*)) {
        this->pfn_custom_serialize_json = pfn_custom_serialize_json;
        return *this;
    }
    type_register<T>& custom_deserialize_json(void(*pfn_custom_deserialize_json)(nlohmann::json&, void*)) {
        this->pfn_custom_deserialize_json = pfn_custom_deserialize_json;
        return *this;
    }
};

class MyBase {
public:
    TYPE_ENABLE_BASE();

    virtual ~MyBase() {}

    virtual void foo() { LOG_DBG("MyBase!"); }
};
class MyDerived : public MyBase {
    TYPE_ENABLE(MyBase);
    void foo() override { LOG_DBG("MyDerived!"); }
};
class MyClass {
public:
    int         decimal;
    float       floating;
    std::string string;
    gfxm::mat3  mat = gfxm::mat3(1.0f);
    std::map<std::string, int32_t> my_map;
    const char* c_string = "Hello, World!";
    std::unordered_map<std::string, std::unique_ptr<MyBase>> objects;


    int                 getInt() { return 0; }
    void                setInt(int a) {}
    double              getDouble() const { return .0; }
    void                setDouble(double a) {}

    const std::string&  getString() const { return "foo"; }
    std::string         getStringNonConst() const { return "bar"; }
    void                setString(const std::string& str) {}
};

inline void type_foo() {
    type_register<MyBase>("MyBase");
    type_register<MyDerived>("MyDerived")
        .parent<MyBase>();

    type_register<MyClass>("MyClass")
        .prop("decimal", &MyClass::decimal)
        .prop("floating", &MyClass::floating)
        .prop("string", &MyClass::string)
        .prop("matrix", &MyClass::mat)
        .prop("my_map", &MyClass::my_map)
        .prop("c_string", &MyClass::c_string)
        .prop("objects", &MyClass::objects)
        .prop("int", &MyClass::getInt, &MyClass::setInt)
        .prop("double", &MyClass::getDouble, &MyClass::setDouble)
        .prop("string2", &MyClass::getStringNonConst, &MyClass::setString);
    
    type t = type_get<MyClass>();
    MyClass my_obj;
    my_obj.decimal = 13;
    my_obj.string = "c++ string";
    my_obj.floating = gfxm::pi;
    my_obj.objects["my_object"].reset(new MyDerived());

    nlohmann::json j;
    t.serialize_json(j, &my_obj);
    LOG_DBG(j.dump(4));

    MyClass new_obj;
    type_get<decltype(new_obj)>().deserialize_json(j, &new_obj);
    LOG_DBG("Deserialization result: " << new_obj.string);
    for (auto& kv : new_obj.objects) {
        kv.second->foo();
    }
}