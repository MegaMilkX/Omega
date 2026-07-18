#pragma once

#include <set>
#include <vector>
#include <string>
#include "type.hpp"
#include "type_property_desc.hpp"


struct type_desc {
    struct parent_info {
        type parent_type;
        void* (*pfn_static_upcast)(void*) = nullptr;
        bool operator<(const parent_info& other) const {
            return parent_type.id < other.parent_type.id;
        }
    };

    type_id_t id;
    size_t size;
    std::string name;
    std::set<parent_info> parent_types;
    std::set<type> derived_types;
    std::vector<type_property_desc> properties;

    bool is_pointer = false;

    void(*pfn_construct)(void* object) = 0;
    void(*pfn_destruct)(void* object) = 0;
    void*(*pfn_construct_new)() = 0;
    void (*pfn_destruct_delete)(void* object) = 0;
    void(*pfn_copy_construct)(void* object, const void* other) = 0;

    void(*pfn_serialize_json)(nlohmann::json& j, const void* object) = 0;
    void(*pfn_deserialize_json)(const nlohmann::json& j, void* object) = 0;

    void(*pfn_custom_serialize_json)(nlohmann::json&, const void*) = 0;
    void(*pfn_custom_deserialize_json)(const nlohmann::json&, void*) = 0;
};
template<>
struct std::hash<type_desc::parent_info> {
    size_t operator()(const type_desc::parent_info& p) const {
        return std::hash<uint64_t>()(p.parent_type.id);
    }
};

