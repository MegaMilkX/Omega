#pragma once

#include <string>
#include <vector>
#include <set>
#include <queue>
#include <stdint.h>

#include "handle/hshared.hpp"

#include "math/gfxm.hpp"
#include "animation/curve.hpp"
#include "resource/resource.hpp"

#include "log/log.hpp"
#include "nlohmann/json.hpp"

template<typename T>
using unqualified_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

// Index generator
uint64_t typeNextGuid();
template<typename T>
struct TYPE_INDEX_GENERATOR {
    static uint64_t guid() {
        static uint64_t guid = typeNextGuid();
        return guid;
    }
};
// ---------------

struct type_property_desc;
struct type_desc;
struct type {
    uint64_t guid;

    type()
        : guid(0) {}
    type(uint64_t guid)
        : guid(guid) {}

    size_t      get_size() const;
    const char* get_name() const;
    const type_desc* get_desc() const;

    bool is_valid() const;

    bool is_pointer() const;
    bool is_copy_constructible() const;

    bool is_derived_from(type other) const;

    int   prop_count() const;
    const type_property_desc* get_prop(int i);

    template<typename O, typename T>
    void set_property(const char* name, O* object, const T& value);
    void set_property_unsafe(const char* name, void* object, void* value) const;

    void  construct(void* ptr);
    void  destruct(void* ptr);
    void* construct_new();
    void  destruct_delete(void* ptr);
    void  copy_construct(void* ptr, const void* other);

    void serialize_json(nlohmann::json& j, void* object) const;
    bool deserialize_json(const nlohmann::json& j, void* object) const;
    void serialize_json(const char* filename, void* object);
    bool deserialize_json(const char* filename, void* object);

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
    bool writable = true;
    bool readable = true;
    
    std::function<void*(void*)> fn_get_ptr;
    std::function<void(void*, void*)> fn_get_value;
    std::function<void(void*, void*)> fn_set;
    //std::function<void(void*, void*)> fn_setter;
    //std::function<void(void*, void*)> fn_getter;

    std::function<void(void*, nlohmann::json&)> fn_serialize_json;
    std::function<void(void*, const nlohmann::json&)> fn_deserialize_json;

    template<typename T>
    T getValue(void* object) const {
        T value = T();        
        if (type_get<T>() != t) {
            assert(false);
            return value;
        }
        
        if (fn_get_value) {
            fn_get_value(object, &value);
        } else if(fn_get_ptr) {
            void* ptr = fn_get_ptr(object);
            value = *(T*)ptr;
        }
        return value;
    }
    template<typename T, typename std::enable_if<!std::is_pointer<T>::value, int>::value* = nullptr>
    void setValue(void* object, const T& value) const {
        if (type_get<T>() != t) {
            assert(false);
            return;
        }
        setValue(object, (void*)&value);
    }
    void setValue(void* object, void* value) const {
        if (fn_set) {
            fn_set(object, value);
        } else if(fn_get_ptr) {
            void* ptr = fn_get_ptr(object);
            memcpy(ptr, value, t.get_size());
        }
    }
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
    void(*pfn_deserialize_json)(const nlohmann::json& j, void* object) = 0;

    void(*pfn_custom_serialize_json)(nlohmann::json&, void*) = 0;
    void(*pfn_custom_deserialize_json)(const nlohmann::json&, void*) = 0;
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
inline const type_desc* type::get_desc() const {
    extern type_desc* get_type_desc(type t);
    auto desc = get_type_desc(*this);
    return desc;
}

inline bool type::is_valid() const {
    return guid != 0;
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
inline int   type::prop_count() const {
    return get_desc()->properties.size();
}
inline const type_property_desc* type::get_prop(int i) {
    return &get_desc()->properties[i];
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
inline void type::set_property_unsafe(const char* name, void* object, void* value) const {
    for (auto& prop : get_desc()->properties) {
        if (prop.name != name) {
            continue;
        }
        if (prop.fn_set) {
            prop.fn_set(object, value);
        }
        // TODO: only handles properties with setters right now (not objects)
    }
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

        t = type_get<unqualified_type<T>>();
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
void type_write_json(nlohmann::json& j, const T& object) {
    j["@class"] = type_get<T>().get_name();
}
template<> inline void type_write_json(nlohmann::json& j, const bool& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const signed char& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const unsigned char& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const char& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const wchar_t& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const char16_t& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const char32_t& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const short& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const unsigned short& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const int& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const unsigned& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const long& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const unsigned long& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const long long& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const unsigned long long& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const float& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const double& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const long double& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const std::string& object) { j = object; }
template<> inline void type_write_json(nlohmann::json& j, const gfxm::vec2& object) { j = nlohmann::json::array({ object.x, object.y }); }
template<> inline void type_write_json(nlohmann::json& j, const gfxm::vec3& object) { j = nlohmann::json::array({ object.x, object.y, object.z }); }
template<> inline void type_write_json(nlohmann::json& j, const gfxm::vec4& object) { j = nlohmann::json::array({ object.x, object.y, object.z, object.w }); }
template<> inline void type_write_json(nlohmann::json& j, const gfxm::quat& object) { j = nlohmann::json::array({ object.x, object.y, object.z, object.w }); }
template<> inline void type_write_json(nlohmann::json& j, const gfxm::mat3& object) { j = nlohmann::json::array({ object[0][0], object[0][1], object[0][2], object[1][0], object[1][1], object[1][2], object[2][0], object[2][1], object[2][2] }); }
template<> inline void type_write_json(nlohmann::json& j, const gfxm::mat4& object) { j = nlohmann::json::array({ object[0][0], object[0][1], object[0][2], object[0][3], object[1][0], object[1][1], object[1][2], object[1][3], object[2][0], object[2][1], object[2][2], object[2][3], object[3][0], object[3][1], object[3][2], object[3][3] }); }
template<> inline void type_write_json(nlohmann::json& j, const type& object) { j = object.get_name(); }
template<typename T> inline void type_write_json(nlohmann::json& j, const curve<T>& curv) {
    j = nlohmann::json::array();
    const auto& keyframes = curv.get_keyframes();
    for (int i = 0; i < keyframes.size(); ++i) {
        const auto& kf = keyframes[i];
        nlohmann::json jkf = nlohmann::json::object();
        jkf["time"] = kf.time;
        type_write_json(jkf["value"], kf.value);
        j.push_back(jkf);
    }
}
template<typename T>
void type_write_json(nlohmann::json& j, const std::vector<T>& object) {
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
void type_write_json(nlohmann::json& j, const std::unordered_map<K, V>& object) {
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
void type_write_json(nlohmann::json& j, const std::map<K, V>& object) {
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
void type_write_json(nlohmann::json& j, const std::unique_ptr<T>& object) {
    if (!object) {
        j = nullptr;
        return;
    }
    type actual_type = object->get_type();
    j["type"] = actual_type.get_name();
    actual_type.serialize_json(j["data"], object.get());
}
template<typename T>
void type_write_json(nlohmann::json& j, const HSHARED<T>& object) {
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
void type_read_json(const nlohmann::json& j, T& object) { /*static_assert(false, "deserialization not implemented");*/ }
template<> inline void type_read_json(const nlohmann::json& j, bool& object) { if (!j.is_boolean()) return; object = j.get<bool>(); }
template<> inline void type_read_json(const nlohmann::json& j, signed char& object) { if (!j.is_number()) return; object = j.get<signed char>(); }
template<> inline void type_read_json(const nlohmann::json& j, unsigned char& object) { if (!j.is_number()) return; object = j.get<unsigned char>(); }
template<> inline void type_read_json(const nlohmann::json& j, char& object) { if (!j.is_number()) return; object = j.get<char>(); }
template<> inline void type_read_json(const nlohmann::json& j, wchar_t& object) { if (!j.is_number()) return; object = j.get<wchar_t>(); }
template<> inline void type_read_json(const nlohmann::json& j, char16_t& object) { if (!j.is_number()) return; object = j.get<char16_t>(); }
template<> inline void type_read_json(const nlohmann::json& j, char32_t& object) { if (!j.is_number()) return; object = j.get<char32_t>(); }
template<> inline void type_read_json(const nlohmann::json& j, short& object) { if (!j.is_number()) return; object = j.get<short>(); }
template<> inline void type_read_json(const nlohmann::json& j, unsigned short& object) { if (!j.is_number()) return; object = j.get<unsigned short>(); }
template<> inline void type_read_json(const nlohmann::json& j, int& object) { if (!j.is_number()) return; object = j.get<int>(); }
template<> inline void type_read_json(const nlohmann::json& j, unsigned& object) { if (!j.is_number()) return; object = j.get<unsigned>(); }
template<> inline void type_read_json(const nlohmann::json& j, long& object) { if (!j.is_number()) return; object = j.get<long>(); }
template<> inline void type_read_json(const nlohmann::json& j, unsigned long& object) { if (!j.is_number()) return; object = j.get<unsigned long>(); }
template<> inline void type_read_json(const nlohmann::json& j, long long& object) { if (!j.is_number()) return; object = j.get<long long>(); }
template<> inline void type_read_json(const nlohmann::json& j, unsigned long long& object) { if (!j.is_number()) return; object = j.get<unsigned long long>(); }
template<> inline void type_read_json(const nlohmann::json& j, float& object) { if (!j.is_number()) return; object = j.get<float>(); }
template<> inline void type_read_json(const nlohmann::json& j, double& object) { if (!j.is_number()) return; object = j.get<double>(); }
template<> inline void type_read_json(const nlohmann::json& j, long double& object) { if (!j.is_number()) return; object = j.get<long double>(); }
template<> inline void type_read_json(const nlohmann::json& j, std::string& object) { if (!j.is_string()) return; object = j.get<std::string>(); }
template<> inline void type_read_json(const nlohmann::json& j, gfxm::vec2& object) { if (!j.is_array() || j.size() != 2) return; object = gfxm::vec2(j[0].get<float>(), j[1].get<float>()); }
template<> inline void type_read_json(const nlohmann::json& j, gfxm::vec3& object) { if (!j.is_array() || j.size() != 3) return; object = gfxm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>()); }
template<> inline void type_read_json(const nlohmann::json& j, gfxm::vec4& object) { if (!j.is_array() || j.size() != 4) return; object = gfxm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); }
template<> inline void type_read_json(const nlohmann::json& j, gfxm::quat& object) { if (!j.is_array() || j.size() != 4) return; object = gfxm::quat(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); }
template<> inline void type_read_json(const nlohmann::json& j, gfxm::mat3& object) { if (!j.is_array() || j.size() != 9) return; object = gfxm::mat3(gfxm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>()), gfxm::vec3(j[3].get<float>(), j[4].get<float>(), j[5].get<float>()), gfxm::vec3(j[6].get<float>(), j[7].get<float>(), j[8].get<float>())); }
template<> inline void type_read_json(const nlohmann::json& j, gfxm::mat4& object) { if (!j.is_array() || j.size() != 16) return; object = gfxm::mat4(gfxm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()), gfxm::vec4(j[4].get<float>(), j[5].get<float>(), j[6].get<float>(), j[7].get<float>()), gfxm::vec4(j[8].get<float>(), j[9].get<float>(), j[10].get<float>(), j[11].get<float>()), gfxm::vec4(j[12].get<float>(), j[13].get<float>(), j[14].get<float>(), j[15].get<float>())); }
template<> inline void type_read_json(const nlohmann::json& j, type& object) {
    type type_get(const char* name);
    if (!j.is_string()) object = type(0);
    object = type_get(j.get<std::string>().c_str());
}
template<typename T> void type_read_json(const nlohmann::json& j, curve<T>& curv) {
    if (!j.is_array()) {
        assert(false);
        return;
    }
    for (const auto& jkf : j) {
        float time = jkf["time"].get<float>();
        T value;
        type_read_json(jkf["value"], value);
        curv[time] = value;
    }
}
template<typename T>
void type_read_json(const nlohmann::json& j, std::vector<T>& object) {
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
void type_read_json(const nlohmann::json& j, std::unordered_map<K, V>& object) {
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
void type_read_json(const nlohmann::json& j, std::map<K, V>& object) {
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
void type_read_json(const nlohmann::json& j, std::unique_ptr<T>& object) {
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
    const nlohmann::json& jdata = it_data.value();

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
void type_read_json(const nlohmann::json& j, HSHARED<T>& object) {
    if (j.is_null()) {
        object.reset();
        return;
    }
    if (j.is_string()) {
        std::string ref_name = j.get<std::string>();
        object = resGet<T>(ref_name.c_str());
    } else if(j.is_object()) {
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
    } else {
        assert(false);
        return;
    }
}

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
std::enable_if_t<std::is_abstract<unqualified_type<T>>::value, type> type_get() {
    using UNQUALIFIED_T = unqualified_type<T>;

    extern std::unordered_map<uint64_t, type_desc>& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid();

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
        it->second.pfn_serialize_json = [](nlohmann::json& j, void* object) { type_write_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](nlohmann::json& j, void* object) { type_read_json(j, *(UNQUALIFIED_T*)object); };
        */
    }

    return type(guid);
}

template<typename T>
std::enable_if_t<!std::is_abstract<unqualified_type<T>>::value && !std::is_copy_constructible<unqualified_type<T>>::value, type> type_get() {
    using UNQUALIFIED_T = unqualified_type<T>;

    extern std::unordered_map<uint64_t, type_desc>& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid();

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

        it->second.pfn_serialize_json = [](nlohmann::json& j, void* object) { type_write_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](const nlohmann::json& j, void* object) { type_read_json(j, *(UNQUALIFIED_T*)object); };
    }

    return type(guid);
}

template<typename T>
std::enable_if_t<!std::is_abstract<unqualified_type<T>>::value && std::is_copy_constructible<unqualified_type<T>>::value, type> type_get() {
    using UNQUALIFIED_T = unqualified_type<T>;

    extern std::unordered_map<uint64_t, type_desc>& get_type_desc_map();
    auto guid = TYPE_INDEX_GENERATOR<UNQUALIFIED_T>::guid();
    
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

        it->second.pfn_serialize_json = [](nlohmann::json& j, void* object) { type_write_json(j, *(UNQUALIFIED_T*)object); };
        it->second.pfn_deserialize_json = [](const nlohmann::json& j, void* object) { type_read_json(j, *(UNQUALIFIED_T*)object); };
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

#define TYPE_ENABLE() \
friend void cppiReflectInit(); \
template<typename T> \
friend class type_register; \
virtual type get_type() const { return type_get<decltype(*this)>(); }


template<class T>
struct GET_MEMBER_TYPE;

template<class C, class M>
struct GET_MEMBER_TYPE<M C::*> {
    using type = M;
};


template<typename T> struct ARGUMENT_CHECKER;

template<typename C, typename R, typename FirstArg, typename... Args>
struct ARGUMENT_CHECKER<R(C::*)(FirstArg, Args...)> {
    using ARG_TYPE = FirstArg;
    constexpr static int arg_count = 1 + sizeof...(Args);
};

template<typename C, typename R>
struct ARGUMENT_CHECKER<R(C::*)()> {
    constexpr static int arg_count = 0;
};

template<typename C, typename R>
struct ARGUMENT_CHECKER<R(C::*)() const> {
    constexpr static int arg_count = 0;
};


template<typename T>
class type_register {
    std::string name;
    std::set<type> parents;
    std::vector<type_property_desc> properties;
    void(*pfn_custom_serialize_json)(nlohmann::json&, void*) = 0;
    void(*pfn_custom_deserialize_json)(const nlohmann::json&, void*) = 0;
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

    template<
        typename MEMBER_T,
        std::enable_if_t<std::is_member_object_pointer<MEMBER_T>::value>* = nullptr
    >
    type_register<T>& prop(const char* name, MEMBER_T member) {
        using MemberType = GET_MEMBER_TYPE<MEMBER_T>::type;

        type_property_desc prop_desc;
        prop_desc.name = name;
        prop_desc.t = type_get<MemberType>();
        prop_desc.fn_get_ptr = [member](void* object)->void* {
            return &(((T*)object)->*member);
        };
        prop_desc.fn_get_value = nullptr;
        
        prop_desc.fn_set = [member](void* object, void* value) {
            // TODO: Does not compile for unique_ptr
            // figure it out!
            (((T*)object)->*member) = (*(MemberType*)value);
        };

        prop_desc.fn_serialize_json = [member](void* object, nlohmann::json& j) {
            type_get<MemberType>().serialize_json(j, &(((T*)object)->*member));
        };
        prop_desc.fn_deserialize_json = [member](void* object, const nlohmann::json& j) {
            type_get<MemberType>().deserialize_json(j, &(((T*)object)->*member));
        };
        properties.push_back(prop_desc);
        return *this;
    }

    template<
        typename GETTER_T,
        std::enable_if_t<std::is_member_function_pointer<GETTER_T>::value>* = nullptr
    >
    type_register<T>& prop_read_only(const char* name, GETTER_T getter) {
        static_assert(ARGUMENT_CHECKER<GETTER_T>::arg_count == 0, "A property getter must have 0 arguments");
        using ReturnType = std::result_of_t<decltype(getter)(T*)>; 
        using ReturnType_Unqualified = unqualified_type<ReturnType>;

        type_property_desc prop_desc;
        prop_desc.writable = false;
        prop_desc.readable = true;
        prop_desc.name = name;
        prop_desc.t = type_get<unqualified_type<ReturnType>>();
        prop_desc.fn_get_ptr = [getter](void* object)->void* {
            // TODO: Try to avoid copying when possible
            // TODO: Actually wtf is this, we're returning a pointer to a temporary
            // Change it so the caller supplies a buffer of appropriate size
            const auto copy = (((T*)object)->*getter)();
            const void* p = &copy;
            return const_cast<void*>(p);
        };
        prop_desc.fn_get_value = [getter](void* object, void* out) {
            *((ReturnType_Unqualified*)out) = (((T*)object)->*getter)();
        };

        prop_desc.fn_serialize_json = [getter](void* object, nlohmann::json& j) {
            const auto&& temporary = (((T*)object)->*getter)();
            type_get<unqualified_type<ReturnType>>().serialize_json(j, (void*)&temporary);
        };
        prop_desc.fn_deserialize_json = nullptr; // Can't deserialize without a setter
        properties.push_back(prop_desc);
        return *this;
    }

    template<
        typename GETTER_T,
        typename SETTER_T, std::enable_if_t<std::is_member_function_pointer<GETTER_T>::value>* = nullptr,
        std::enable_if_t<std::is_member_function_pointer<SETTER_T>::value>* = nullptr
    >
    type_register<T>& prop(const char* name, GETTER_T getter, SETTER_T setter) {
        static_assert(ARGUMENT_CHECKER<GETTER_T>::arg_count == 0, "A property getter must have 0 arguments");
        static_assert(ARGUMENT_CHECKER<SETTER_T>::arg_count == 1, "A property setter must have 1 argument");
        using ReturnType = std::result_of_t<decltype(getter)(T*)>;
        using ReturnType_Unqualified = unqualified_type<ReturnType>;
        using ArgType = ARGUMENT_CHECKER<SETTER_T>::ARG_TYPE;
        static_assert(std::is_same<unqualified_type<ReturnType>, unqualified_type<ArgType>>::value, "property setter and getter return and argument types must be the same");

        type_property_desc prop_desc;
        prop_desc.writable = true;
        prop_desc.readable = true;
        prop_desc.name = name;
        prop_desc.t = type_get<unqualified_type<ReturnType>>();
        prop_desc.fn_get_ptr = [getter](void* object)->void* {
            // TODO: Try to avoid copying when possible
            // TODO: Actually wtf is this, we're returning a pointer to a temporary
            // Change it so the caller supplies a buffer of appropriate size
            const auto copy = (((T*)object)->*getter)();
            const void* p = &copy;
            return const_cast<void*>(p);
        };
        prop_desc.fn_get_value = [getter](void* object, void* out) {
            *((ReturnType_Unqualified*)out) = (((T*)object)->*getter)();
        };

        // TODO:
        prop_desc.fn_set = [setter](void* object, void* value) {
            using NoRefArgType = unqualified_type<ArgType>;
            (((T*)object)->*setter)(*(NoRefArgType*)value);
        };

        prop_desc.fn_serialize_json = [getter](void* object, nlohmann::json& j) {
            const auto temporary = (((T*)object)->*getter)();
            type_get<unqualified_type<ReturnType>>().serialize_json(j, (void*)&temporary);
        };
        prop_desc.fn_deserialize_json = [setter](void* object, const nlohmann::json& j) {
            type member_type = type_get<unqualified_type<ArgType>>();
            std::vector<unsigned char> buf(member_type.get_size());
            member_type.construct(buf.data());
            member_type.deserialize_json(j, buf.data());
            (((T*)object)->*setter)(*(unqualified_type<ArgType>*)buf.data());
            member_type.destruct(buf.data());
        };
        properties.push_back(prop_desc);
        return *this;
    }

    type_register<T>& custom_serialize_json(void(*pfn_custom_serialize_json)(nlohmann::json&, void*)) {
        this->pfn_custom_serialize_json = pfn_custom_serialize_json;
        return *this;
    }
    type_register<T>& custom_deserialize_json(void(*pfn_custom_deserialize_json)(const nlohmann::json&, void*)) {
        this->pfn_custom_deserialize_json = pfn_custom_deserialize_json;
        return *this;
    }
};


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


class MyBase {
public:
    TYPE_ENABLE();

    virtual ~MyBase() {}

    virtual void foo() { LOG_DBG("MyBase!"); }
};
class MyDerived : public MyBase {
public:
    TYPE_ENABLE();
    void foo() override { LOG_DBG("MyDerived!"); }
};
class MyClass {
public:
    TYPE_ENABLE();

    int         decimal;
    float       floating;
    std::string string;
    gfxm::mat3  mat = gfxm::mat3(1.0f);
    std::map<std::string, int32_t> my_map;
    const char* c_string = "Hello, World!";
    //std::unordered_map<std::string, std::unique_ptr<MyBase>> objects;


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
        //.prop("objects", &MyClass::objects)
        .prop("int", &MyClass::getInt, &MyClass::setInt)
        .prop("double", &MyClass::getDouble, &MyClass::setDouble)
        .prop("string2", &MyClass::getStringNonConst, &MyClass::setString);
    
    type t = type_get<MyClass>();
    MyClass my_obj;
    my_obj.decimal = 13;
    my_obj.string = "c++ string";
    my_obj.floating = gfxm::pi;
    //my_obj.objects["my_object"].reset(new MyDerived());

    nlohmann::json j;
    t.serialize_json(j, &my_obj);
    LOG_DBG(j.dump(4));

    MyClass new_obj;
    type_get<decltype(new_obj)>().deserialize_json(j, &new_obj);
    LOG_DBG("Deserialization result: " << new_obj.string);
    /*
    for (auto& kv : new_obj.objects) {
        kv.second->foo();
    }*/
}