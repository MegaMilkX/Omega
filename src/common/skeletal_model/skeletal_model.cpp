#include "skeletal_model.hpp"
#include "log/log.hpp"

#include "util/static_block.hpp"
#include "resource/resource.hpp"
STATIC_BLOCK{
    SkeletalModel::reflect();

    sklmMeshComponent::reflect();
    sklmSkinComponent::reflect();
}

void sklmMeshComponent::reflect() {
    rtti::type_register<sklmMeshComponent>("sklmMeshComponent")
        .parent<sklmComponent>()
        .custom_serialize_json([](nlohmann::json& j, const void* object) {
            auto o = (sklmMeshComponent*)object;
            rtti::serializeJson(j["name"], o->getName());
            rtti::serializeJson(j["bone_name"], o->bone_name);
            rtti::serializeJson(j["mesh"], o->mesh);
            rtti::serializeJson(j["material"], o->material);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            auto o = (sklmMeshComponent*)object;
            std::string name;
            rtti::deserializeJson(j["name"], name);
            o->setName(name.c_str());
            rtti::deserializeJson(j["bone_name"], o->bone_name);
            rtti::deserializeJson(j["mesh"], o->mesh);
            if (o->mesh) {
                const_cast<gpuMeshDesc*>(o->mesh->getMeshDesc())->mesh_type = GPU_MESH_DESC_TYPE::GENERIC;
            }
            //o->material = resGet<gpuMaterial>("materials/default.mat");
            rtti::deserializeJson(j["material"], o->material);
        });
}
#include "base64/base64.hpp"
#include "serialization/virtual_obuf.hpp"
#include "serialization/virtual_ibuf.hpp"
void sklmSkinComponent::reflect() {
    rtti::type_register<sklmSkinComponent>("sklmSkinComponent")
        .parent<sklmComponent>()
        .custom_serialize_json([](nlohmann::json& j, const void* object) {
            auto o = (sklmSkinComponent*)object;

            std::string b64_bone_data;
            vofbuf vof;
            vof.write_string_vector(o->bone_names, true);
            vof.write_vector(o->inv_bind_transforms, true);
            base64_encode(vof.getData(), vof.getSize(), b64_bone_data);

            rtti::serializeJson(j["name"], o->getName());
            j["bone_data"] = b64_bone_data;
            rtti::serializeJson(j["mesh"], o->mesh);
            rtti::serializeJson(j["material"], o->material);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            auto o = (sklmSkinComponent*)object;
            std::string name;
            rtti::deserializeJson(j["name"], name);
            o->setName(name.c_str());
            
            std::string b64_bone_data = j["bone_data"];
            std::vector<char> bone_data_bytes;
            base64_decode(b64_bone_data.data(), b64_bone_data.size(), bone_data_bytes);
            vifbuf vif((unsigned char*)bone_data_bytes.data(), bone_data_bytes.size());
            vif.read_string_vector(o->bone_names);
            vif.read_vector(o->inv_bind_transforms);

            rtti::deserializeJson(j["mesh"], o->mesh);
            rtti::deserializeJson(j["material"], o->material);
        });

}
void SkeletalModel::reflect() {
    rtti::type_register<SkeletalModel>("SkeletalModel")
        .custom_serialize_json([](nlohmann::json& j, const void* object) {
            auto o = (SkeletalModel*)object;
            ResourceRef<Skeleton> skeleton = o->getSkeleton();
            rtti::serializeJson(j["skeleton"], skeleton);
            rtti::serializeJson(j["components"], o->components);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            auto o = (SkeletalModel*)object;
            ResourceRef<Skeleton> skeleton;
            rtti::deserializeJson(j["skeleton"], skeleton);
            o->setSkeleton(skeleton);
            rtti::deserializeJson(j["components"], o->components);
        });
}


SkeletalModel::SkeletalModel() {
    setSkeleton(createResource<Skeleton>(""));
}

HSHARED<SkeletalModelInstance> SkeletalModel::createInstance() {
    auto h = getSkeleton()->createInstance();
    return createInstance(h);
}

HSHARED<SkeletalModelInstance> SkeletalModel::createInstance(HSHARED<SkeletonInstance>& skl_inst) {
    HSHARED<SkeletalModelInstance> hs(HANDLE_MGR<SkeletalModelInstance>::acquire());
    instances.insert(hs);

    hs->prototype = this;
    auto& instance_data = hs->instance_data;
    instance_data.skeleton_instance = skl_inst;

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
void SkeletalModel::destroyInstance(SkeletalModelInstance* mdl_inst) {
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
void SkeletalModel::spawnInstance(SkeletalModelInstance* mdl_inst, scnRenderScene* scn) {
    if (mdl_inst->prototype != this) {
        assert(false);
        return;
    }
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        void* inst_ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_onSpawnInstance(inst_ptr, scn);
    }
}
void SkeletalModel::despawnInstance(SkeletalModelInstance* mdl_inst, scnRenderScene* scn) {
    if (mdl_inst->prototype != this) {
        assert(false);
        return;
    }
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        void* inst_ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_onDespawnInstance(inst_ptr, scn);
    }
}

void SkeletalModel::initSampleBuffer(animModelSampleBuffer& buf) {
    size_t sampleBufferSize = 0;
    for (auto& c : components) {
        sampleBufferSize += c->_getAnimSampleSize();
    }
    buf.buffer.resize(sampleBufferSize);
}

void SkeletalModel::applySampleBuffer(SkeletalModelInstance* mdl_inst, animModelSampleBuffer& buf) {
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        if (c->_getAnimSampleSize() == 0) {
            continue;
        }
        void* inst_ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_applyAnimSample(inst_ptr, buf[c->getAnimSampleBufOffset()]);
    }
}

void SkeletalModel::enableTechnique(SkeletalModelInstance* mdl_inst, const char* path, bool value) {
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        void* ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_enableTechnique(ptr, path, value);
    }
}
void SkeletalModel::setParam(SkeletalModelInstance* mdl_inst, const char* param_name, GPU_TYPE type, const void* pvalue) {
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        void* ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_setParam(ptr, param_name, type, pvalue);
    }
}
void SkeletalModel::setLayer(SkeletalModelInstance* mdl_inst, int i) {
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        void* ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_setLayer(ptr, i);
    }
}
void SkeletalModel::submit(SkeletalModelInstance* mdl_inst, gpuRenderBucket* bucket) {
    auto& instance_data = mdl_inst->instance_data;
    for (auto& c : components) {
        void* ptr = &instance_data.instance_data_bytes[c->instance_data_offset];
        c->_submit(ptr, bucket);
    }
}

void SkeletalModel::dbgLog() {
    LOG("SkeletalModel components:");
    for (auto& c : components) {
        LOG(c->getName());
    }
}

bool SkeletalModel::load(byte_reader& reader) {
    auto view = reader.try_slurp();
    if (!view) {
        return false;
    }
    std::string str_json(view.data, view.data + view.size);
    nlohmann::json json = nlohmann::json::parse(str_json);
    if (!json.is_object()) {
        return false;
    }

    ResourceRef<Skeleton> skeleton;
    rtti::deserializeJson(json["skeleton"], skeleton);
    setSkeleton(skeleton);
    rtti::deserializeJson(json["components"], components);
    return true;
}