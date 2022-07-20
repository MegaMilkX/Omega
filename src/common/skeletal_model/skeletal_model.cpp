#include "skeletal_model.hpp"
#include "log/log.hpp"

#include "util/static_block.hpp"
#include "resource/resource.hpp"
STATIC_BLOCK{
    sklmSkeletalModelEditable::reflect();

    sklmMeshComponent::reflect();
    sklmSkinComponent::reflect();

    resAddCache<sklmSkeletalModelEditable>(new resCacheDefault<sklmSkeletalModelEditable>);
}

void sklmMeshComponent::reflect() {
    type_register<sklmMeshComponent>("sklmMeshComponent")
        .parent<sklmComponent>()
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            auto o = (sklmMeshComponent*)object;
            serializeJson(j["name"], o->getName());
            serializeJson(j["bone_name"], o->bone_name);
            serializeJson(j["mesh"], o->mesh);
            serializeJson(j["material"], o->material);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            auto o = (sklmMeshComponent*)object;
            std::string name;
            deserializeJson(j["name"], name);
            o->setName(name.c_str());
            deserializeJson(j["bone_name"], o->bone_name);
            deserializeJson(j["mesh"], o->mesh);
            //o->material = resGet<gpuMaterial>("materials/default.mat");
            deserializeJson(j["material"], o->material);
        });
}
#include "base64/base64.hpp"
#include "serialization/virtual_obuf.hpp"
#include "serialization/virtual_ibuf.hpp"
void sklmSkinComponent::reflect() {
    type_register<sklmSkinComponent>("sklmSkinComponent")
        .parent<sklmComponent>()
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            auto o = (sklmSkinComponent*)object;

            std::string b64_bone_data;
            vofbuf vof;
            vof.write_string_vector(o->bone_names, true);
            vof.write_vector(o->inv_bind_transforms, true);
            base64_encode(vof.getData(), vof.getSize(), b64_bone_data);

            serializeJson(j["name"], o->getName());
            j["bone_data"] = b64_bone_data;
            serializeJson(j["mesh"], o->mesh);
            serializeJson(j["material"], o->material);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            auto o = (sklmSkinComponent*)object;
            std::string name;
            deserializeJson(j["name"], name);
            o->setName(name.c_str());
            
            std::string b64_bone_data = j["bone_data"];
            std::vector<char> bone_data_bytes;
            base64_decode(b64_bone_data.data(), b64_bone_data.size(), bone_data_bytes);
            vifbuf vif((unsigned char*)bone_data_bytes.data(), bone_data_bytes.size());
            vif.read_string_vector(o->bone_names);
            vif.read_vector(o->inv_bind_transforms);

            deserializeJson(j["mesh"], o->mesh);
            deserializeJson(j["material"], o->material);
        });

}
void sklmSkeletalModelEditable::reflect() {
    type_register<sklmSkeletalModelEditable>("sklmSkeletalModelEditable")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            auto o = (sklmSkeletalModelEditable*)object;
            RHSHARED<sklSkeletonEditable> skeleton = o->getSkeleton();
            serializeJson(j["skeleton"], skeleton);
            serializeJson(j["components"], o->components);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            auto o = (sklmSkeletalModelEditable*)object;
            RHSHARED<sklSkeletonEditable> skeleton;
            deserializeJson(j["skeleton"], skeleton);
            o->setSkeleton(skeleton);
            deserializeJson(j["components"], o->components);
        });
}


HSHARED<sklmSkeletalModelInstance> sklmSkeletalModelEditable::createInstance() {
    return createInstance(getSkeleton()->createInstance());
}

HSHARED<sklmSkeletalModelInstance> sklmSkeletalModelEditable::createInstance(HSHARED<sklSkeletonInstance>& skl_inst) {
    HSHARED<sklmSkeletalModelInstance> hs(HANDLE_MGR<sklmSkeletalModelInstance>::acquire());
    instances.insert(hs);

    hs->instance_data.skeleton_instance = skl_inst;

    auto& instance_data = hs->instance_data;
    for (auto& c : components) {
        c->_appendInstance(instance_data, getSkeleton().get());
    }

    return hs;
}


void sklmSkeletalModelEditable::dbgLog() {
    LOG("sklmSkeletalModelEditable components:");
    for (auto& c : components) {
        LOG(c->getName());
    }
}
