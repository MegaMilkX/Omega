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
    if (!desc->pfn_copy_construct) {
        LOG_ERR(get_name() << " is not copy-constructable");
        assert(false);
        return;
    }
    desc->pfn_destruct(ptr);
}
void type::copy_construct(void* ptr, const void* other) {
    auto desc = get_type_desc(*this);
    desc->pfn_copy_construct(ptr, other);
}

void type::serialize_json(nlohmann::json& j, void* object) {
    auto desc = get_type_desc(*this);
    if (desc->properties.empty()) {
        desc->pfn_serialize_json(j, object);
    } else {
        j = {};
        for (int i = 0; i < desc->properties.size(); ++i) {
            auto& prop = desc->properties[i];
            auto desc_prop = get_type_desc(prop.t);
            nlohmann::json jprop;
            prop.fn_serialize_json(object, jprop);
            /*
            std::vector<unsigned char> buf(prop.t.get_size());
            desc_prop->pfn_construct(buf.data());
            prop.fn_getter(object, buf.data());
            prop.t.serialize_json(jprop, buf.data());
            desc_prop->pfn_destruct(buf.data());
            */
            j[prop.name] = jprop;
        }
    }
}
void type::deserialize_json(nlohmann::json& j, void* object) {
    auto desc = get_type_desc(*this);
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
            /*
            std::vector<unsigned char> buf(prop.t.get_size());
            desc_prop->pfn_construct(buf.data());
            prop.t.deserialize_json(it.value(), buf.data());
            prop.fn_setter(object, buf.data());            
            desc_prop->pfn_destruct(buf.data());*/
        }
    }
}