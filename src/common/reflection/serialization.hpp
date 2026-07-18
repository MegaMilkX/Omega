#pragma once

#include "type.hpp"
#include "nlohmann/json.hpp"
#include "math/gfxm.hpp"
#include "animation/curve.hpp"
#include "handle/hshared.hpp"
#include "resource/resource.hpp"


namespace rtti {


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


}