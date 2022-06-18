#pragma once

#include <map>
#include <queue>
#include "log/log.hpp"

#include "mdl_mutable.hpp"

#include "mdl_component_prototype.hpp"

struct mdlModelPrototype {
    std::vector<int32_t>                parents;
    std::vector<gfxm::mat4>             default_local_transforms;
    std::map<std::string, uint32_t>     name_to_node;
    std::unordered_map<type, std::unique_ptr<mdlComponentPrototype>> components;

    void make(mdlModelMutable* mm);
};

#include "serialization/serialization.hpp"
inline void mdlSerializePrototype(const char* fpath, mdlModelPrototype* proto) {
    FILE* f = fopen(fpath, "wb");

    nlohmann::json j;
    type_get<mdlModelPrototype>().serialize_json(j, proto);
    fwrite_string(j.dump(4), f);
    /*
    const uint32_t tag = *(uint32_t*)"MDLP";
    const uint32_t version = 0;
    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);

    fwrite_vector(proto->parents, f, true);
    fwrite_vector(proto->default_local_transforms, f, false);
    
    for (auto& kv : proto->name_to_node) {
        fwrite_string(kv.first, f);
        fwrite(&kv.second, sizeof(kv.second), 1, f);
    }

    for (auto& kv : proto->components) {
        auto type_desc = mdlComponentGetTypeDesc(kv.first);
        if (!type_desc) {
            LOG_ERR("mdl Component type is not registered: " << kv.first.get_name());
            assert(false);
            continue;
        }
        fwrite_string(type_desc->name, f);
        type t = kv.second->get_type();
        nlohmann::json j;
        t.serialize_json(j, kv.second.get());
        fwrite_string(j.dump(4), f);
    }*/

    fclose(f);
}
inline void mdlDeserializePrototype(const char* fpath, mdlModelPrototype* proto) {
    std::ifstream f(fpath);
    nlohmann::json j;
    f >> j;
    type_get<mdlModelPrototype>().deserialize_json(j, proto);
}