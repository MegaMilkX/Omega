#include "reflection.hpp"

#include <unordered_map>

uint64_t typeNextGuid() {
    static uint64_t guid = 0;
    return ++guid;
}


std::unordered_map<uint64_t, type_desc>& get_type_desc_map() {
    static std::unordered_map<uint64_t, type_desc> types;
    return types;
}

std::unordered_map<std::string, type>& get_type_name_map() {
    static std::unordered_map<std::string, type> names;
    return names;
}

type_desc* get_type_desc(type t) {
    auto& map = get_type_desc_map();
    auto it = map.find(t.guid);
    if (it == map.end()) {
        it = map.insert(std::make_pair(t.guid, type_desc())).first;
    }
    return &it->second;
}

void type_dbg_print() {
    auto& map = get_type_desc_map();
    for (auto& kv : map) {
        LOG_DBG(kv.second.name << "(" << kv.second.guid << ")");
        for (int i = 0; i < kv.second.properties.size(); ++i) {
            auto& prop = kv.second.properties[i];
            LOG_DBG("\t" << prop.name << "(" << prop.t.get_name() << ")");
        }
    }
}


void type::construct(void* ptr) {
    auto desc = get_type_desc(*this);
    desc->pfn_construct(ptr);
}
void type::destruct(void* ptr) {
    auto desc = get_type_desc(*this);
    if (!desc->pfn_destruct) {
        LOG_ERR(get_name() << " has no in place destructor");
        assert(false);
        return;
    }
    desc->pfn_destruct(ptr);
}
void* type::construct_new() {
    auto desc = get_type_desc(*this);
    if (!desc->pfn_construct_new) {
        LOG_ERR(get_name() << " has no constructor");
        assert(false);
        return 0;
    }
    return desc->pfn_construct_new();
}
void  type::destruct_delete(void* ptr) {
    auto desc = get_type_desc(*this);
    if (!desc->pfn_destruct_delete) {
        LOG_ERR(get_name() << " has no destructor");
        assert(false);
        return;
    }
    desc->pfn_destruct_delete(ptr);
}
void type::copy_construct(void* ptr, const void* other) {
    auto desc = get_type_desc(*this);
    desc->pfn_copy_construct(ptr, other);
}

void type::serialize_json(nlohmann::json& j, void* object) {
    auto desc = get_type_desc(*this);
    if (desc->pfn_custom_serialize_json) {
        desc->pfn_custom_serialize_json(j, object);
    } else {
        if (desc->properties.empty()) {
            desc->pfn_serialize_json(j, object);
        } else {
            j = {};
            for (int i = 0; i < desc->properties.size(); ++i) {
                auto& prop = desc->properties[i];
                auto desc_prop = get_type_desc(prop.t);
                nlohmann::json jprop;
                prop.fn_serialize_json(object, jprop);
                j[prop.name] = jprop;
            }
        }
    }
}
void type::deserialize_json(nlohmann::json& j, void* object) {
    auto desc = get_type_desc(*this);
    if (desc->pfn_custom_deserialize_json) {
        desc->pfn_custom_deserialize_json(j, object);
    } else {
        if (desc->properties.empty()) {
            desc->pfn_deserialize_json(j, object);
        } else {
            if (!j.is_object()) {
                assert(false);
                LOG_ERR("type::deserialize_json: j must be an object");
                return;
            }
            for (int i = 0; i < desc->properties.size(); ++i) {
                auto& prop = desc->properties[i];
                auto desc_prop = get_type_desc(prop.t);
                auto it = j.find(prop.name);
                if (it == j.end()) {
                    LOG_WARN("No json property '" << prop.name << "'");
                    continue;
                }
                prop.fn_deserialize_json(object, it.value());
            }
        }
    }
}