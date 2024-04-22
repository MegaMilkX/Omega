#include "skeletal_model.hpp"
#include "log/log.hpp"

#include "util/static_block.hpp"
#include "resource/resource.hpp"
STATIC_BLOCK{
    mdlSkeletalModelMaster::reflect();

    sklmMeshComponent::reflect();
    sklmSkinComponent::reflect();

    resAddCache<mdlSkeletalModelMaster>(new resCacheDefault<mdlSkeletalModelMaster>);
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
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
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
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
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
void mdlSkeletalModelMaster::reflect() {
    type_register<mdlSkeletalModelMaster>("mdlSkeletalModelMaster")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            auto o = (mdlSkeletalModelMaster*)object;
            RHSHARED<Skeleton> skeleton = o->getSkeleton();
            serializeJson(j["skeleton"], skeleton);
            serializeJson(j["components"], o->components);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            auto o = (mdlSkeletalModelMaster*)object;
            RHSHARED<Skeleton> skeleton;
            deserializeJson(j["skeleton"], skeleton);
            o->setSkeleton(skeleton);
            deserializeJson(j["components"], o->components);
        });
}


mdlSkeletalModelMaster::mdlSkeletalModelMaster() {
    setSkeleton(HSHARED<Skeleton>(HANDLE_MGR<Skeleton>::acquire()));
}

HSHARED<mdlSkeletalModelInstance> mdlSkeletalModelMaster::createInstance() {
    auto h = getSkeleton()->createInstance();
    return createInstance(h);
}

HSHARED<mdlSkeletalModelInstance> mdlSkeletalModelMaster::createInstance(HSHARED<SkeletonPose>& skl_inst) {
    HSHARED<mdlSkeletalModelInstance> hs(HANDLE_MGR<mdlSkeletalModelInstance>::acquire());
    instances.insert(hs);

    hs->prototype = this;

    hs->instance_data.skeleton_instance = skl_inst;

    auto& instance_data = hs->instance_data;

    size_t instance_data_buf_size = 0;
    for (auto& c : components) {
        c->instance_data_offset = instance_data_buf_size;
        instance_data_buf_size += c->_getInstanceDataSize();
    }
    instance_data.instance_data_bytes.resize(instance_data_buf_size);
    for (auto& c : components) {
        void* inst_ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_constructInstanceData(inst_ptr, skl_inst.get());
    }

    return hs;
}
void mdlSkeletalModelMaster::destroyInstance(mdlSkeletalModelInstance* mdl_inst) {
    if (mdl_inst->prototype != this) {
        assert(false);
        return;
    }
    
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        void* inst_ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_destroyInstanceData(inst_ptr);
    }
    instance_data.instance_data_bytes.clear();
    instance_data.skeleton_instance.reset();
    
    mdl_inst->prototype = 0;
}
void mdlSkeletalModelMaster::spawnInstance(mdlSkeletalModelInstance* mdl_inst, scnRenderScene* scn) {
    if (mdl_inst->prototype != this) {
        assert(false);
        return;
    }
    auto& instance_data = mdl_inst->instance_data;
    instance_data.skeleton_instance->onSpawn(scn);
    for (auto& c : components) {
        void* inst_ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_onSpawnInstance(inst_ptr, scn);
    }
}
void mdlSkeletalModelMaster::despawnInstance(mdlSkeletalModelInstance* mdl_inst, scnRenderScene* scn) {
    if (mdl_inst->prototype != this) {
        assert(false);
        return;
    }
    auto& instance_data = mdl_inst->instance_data;
    instance_data.skeleton_instance->onDespawn(scn);
    for (auto& c : components) {
        void* inst_ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_onDespawnInstance(inst_ptr, scn);
    }
}

void mdlSkeletalModelMaster::initSampleBuffer(animModelSampleBuffer& buf) {
    size_t sampleBufferSize = 0;
    for (auto& c : components) {
        sampleBufferSize += c->_getAnimSampleSize();
    }
    buf.buffer.resize(sampleBufferSize);
}

void mdlSkeletalModelMaster::applySampleBuffer(mdlSkeletalModelInstance* mdl_inst, animModelSampleBuffer& buf) {
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        if (c->_getAnimSampleSize() == 0) {
            continue;
        }
        void* inst_ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_applyAnimSample(inst_ptr, buf[c->getAnimSampleBufOffset()]);
    }
}


void mdlSkeletalModelMaster::dbgLog() {
    LOG("mdlSkeletalModelMaster components:");
    for (auto& c : components) {
        LOG(c->getName());
    }
}
