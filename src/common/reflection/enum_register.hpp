#pragma once

#include <string>
#include <type_traits>
#include "nlohmann/json.hpp"
#include "type.hpp"
#include "type_desc.hpp"
#include "log/log.hpp"


namespace rtti {


template<typename T>
class enum_register {
    static_assert(std::is_enum_v<T>, "rtti::enum_register<T>: T must be an enum");

    std::string name;
    std::vector<enumerator_desc> enumerators;
public:
    enum_register(const char* name)
        : name(name) {}
    ~enum_register() {
        type t = type_get<T>();
        auto desc = get_type_desc(t);
        desc->name = name;
        desc->is_enum = true;
        desc->enumerators = enumerators;
        std::sort(desc->enumerators.begin(), desc->enumerators.end(), [](const enumerator_desc& a, const enumerator_desc& b)->bool {
            return a.value < b.value;
        });
        
        desc->pfn_set_enum = [](void* object, long long in) {
            T& value = *(T*)object;
            value = (T)in;
        };
        desc->pfn_get_enum = [](const void* object, long long& out) {
            const T& value = *(const T*)object;
            out = (long long)value;
        };

        desc->pfn_custom_serialize_json = [](nlohmann::json& json, const void* object) {
            const T& value = *(const T*)object;
            auto desc = get_type_desc(type_get<T>());
            const auto& enumerators = desc->enumerators;
            for (const auto& e : enumerators) {
                if (e.value == (long long)value) {
                    json = e.name;
                    return;
                }
            }

            // Fallback behavior, enumerator not registered, just write the raw value
            json = (long long)value;
        };
        desc->pfn_custom_deserialize_json = [](const nlohmann::json& json, void* object) {
            T& value = *(T*)object;
            auto desc = get_type_desc(type_get<T>());
            const auto& enumerators = desc->enumerators;
            if (json.is_string()) {
                const std::string& str = json.get<std::string>();
                for (const auto& e : enumerators) {
                    if (e.name == str) {
                        value = static_cast<T>(e.value);
                        return;
                    }
                }
                LOG_WARN("Unknown enumerator name during deserialization: " << str);
            } else if(json.is_number()) {
                value = static_cast<T>(json.get<long long>());
            } else {
                LOG_WARN("Json expected to be a string or number during enum deserialization");
            }
        };
        
        {
            auto& map = get_type_name_map();
            map[name] = t;
        }
    }

    enum_register<T>& enumerator(const char* e_name, T value) {
        enumerators.push_back(enumerator_desc{ std::string(e_name), (long long)value });
        return *this;
    }
};


}