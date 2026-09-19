#include "reflection.hpp"

#include <unordered_map>


namespace rtti {

type_id_t typeNextGuid() {
    static uint64_t guid = 0;
    ++guid;
    return guid;
}


type_desc_map_t& get_type_desc_map() {
    static type_desc_map_t types;
    return types;
}

std::unordered_map<std::string, type>& get_type_name_map() {
    static std::unordered_map<std::string, type> names;
    return names;
}

type_desc* get_type_desc(type t) {
    auto& map = get_type_desc_map();
    auto it = map.find(t.id);
    if (it == map.end()) {
        it = map.insert(std::make_pair(t.id, type_desc())).first;
    }
    return &it->second;
}

bool read_snapshot(const nlohmann::json& json, PropSnapshot& out) {    
    if (!json.is_object()) {
        LOG_ERR("PropSnapshot json must be an object");
        assert(false);
        return false;
    }

    std::string stype = json.value("@type", "");
    type t = type_get(stype.c_str());
    if (!t.is_valid()) {
        LOG_ERR("Failed to read snapshot, type: '" << t.get_name() << "'");
        assert(false);
        return false;
    }

    if (!t.is_derived_from(rtti::type_get<rtti::MetaObject>())) {
        LOG_ERR("Failed to read snapshot: type '" << t.get_name() << "' is not derived from MetaObject");
        assert(false);
        return false;
    }

    // TODO: Cache schema-generating default object or just it's schema-snapshot
    rtti::MetaObject* default_mo = t.construct_new<rtti::MetaObject>();
            
    rtti::PropSnapshot schema;
    default_mo->makeSnapshot(schema);
    out.clear();
    out.fromJson(schema, json);

    delete default_mo;
    return true;
}

void type_dbg_print() {
    auto& map = get_type_desc_map();
    for (auto& kv : map) {
        LOG_DBG(kv.second.name << "(" << kv.second.id << ")");
        for (int i = 0; i < kv.second.properties.size(); ++i) {
            auto& prop = kv.second.properties[i];
            LOG_DBG("\t" << prop.name << "(" << prop.t.get_name() << ")");
        }
    }
}


void type::construct(void* ptr) const {
    auto desc = get_type_desc(*this);
    desc->pfn_construct(ptr);
}
void type::destruct(void* ptr) const {
    auto desc = get_type_desc(*this);
    if (!desc->pfn_destruct) {
        LOG_ERR(get_name() << " has no in place destructor");
        assert(false);
        return;
    }
    desc->pfn_destruct(ptr);
}
void* type::construct_new() const {
    auto desc = get_type_desc(*this);
    if (!desc->pfn_construct_new) {
        LOG_ERR(get_name() << " has no constructor");
        assert(false);
        return 0;
    }
    return desc->pfn_construct_new();
}
Resource* type::construct_as_resource() const {
    auto desc = get_type_desc(*this);
    if (!desc->pfn_construct_as_resource) {
        LOG_ERR(get_name() << " has no constructor");
        assert(false);
        return 0;
    }
    return desc->pfn_construct_as_resource();
}
void  type::destruct_delete(void* ptr) const {
    auto desc = get_type_desc(*this);
    if (!desc->pfn_destruct_delete) {
        LOG_ERR(get_name() << " has no destructor");
        assert(false);
        return;
    }
    desc->pfn_destruct_delete(ptr);
}
void type::copy_construct(void* ptr, const void* other) const {
    auto desc = get_type_desc(*this);
    desc->pfn_copy_construct(ptr, other);
}
ResourceRefBase* type::as_resource_ref_base(void* object) const {
    if(!object) return nullptr;
    auto desc = get_type_desc(*this);
    if(!desc->pfn_as_resource_ref_base) return nullptr;
    return desc->pfn_as_resource_ref_base(object);
}

void type::serialize_json(nlohmann::json& j, const void* object) const {
    if (j.is_null()) {
        j = {};
    } else if(!j.is_object()) {
        assert(false);
        LOG_ERR("type::serialize_json(): j is not null and not an object, existing data will be lost");
        j = {};
    }
    auto desc = get_type_desc(*this);
    /*
    for (const auto& parent_info : desc->parent_types) {
        parent_info.parent_type.serialize_json(j, object);
    }*/

    if (desc->properties.empty()) {
        if (desc->pfn_serialize_json) {
            desc->pfn_serialize_json(j, object);
        }
    } else {
        j["@class"] = get_name();
        for (int i = 0; i < desc->properties.size(); ++i) {
            auto& prop = desc->properties[i];
            auto desc_prop = get_type_desc(prop.t);
            nlohmann::json jprop;
            prop.fn_serialize_json(object, jprop);
            j[prop.name] = jprop;
        }
    }

    if (desc->pfn_custom_serialize_json) {
        desc->pfn_custom_serialize_json(j, object);
    }
}
bool type::deserialize_json(const nlohmann::json& j, void* object) const {
    auto desc = get_type_desc(*this);
    /*
    for (const auto& parent_info : desc->parent_types) {
        parent_info.parent_type.deserialize_json(j, object);
    }*/
    
    if (desc->properties.empty()) {
        if (desc->pfn_deserialize_json) {
            desc->pfn_deserialize_json(j, object);
        }
    } else {
        if (!j.is_object()) {
            assert(false);
            LOG_ERR("type::deserialize_json: j must be an object");
            return false;
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

    if (desc->pfn_custom_deserialize_json) {
        desc->pfn_custom_deserialize_json(j, object);
    }
    return true;
}
void type::serialize_json(const char* filename, const void* object) {
    nlohmann::json j;
    serialize_json(j, object);
    std::ofstream of(filename);
    of << j.dump(4);
    of.close();
}
bool type::deserialize_json(const char* filename, void* object) {
    nlohmann::json j;
    std::ifstream f(filename);
    if (!f) {
        LOG_ERR("Failed to open file " << filename);
        assert(false);
        return false;
    }

    try {
        f >> j;
    } catch(const nlohmann::json::exception& ex) {
        LOG_ERR("json exeption: " << ex.what());
        assert(false);
        return false;
    }
    f.close();

    return deserialize_json(j, object);
}


}

