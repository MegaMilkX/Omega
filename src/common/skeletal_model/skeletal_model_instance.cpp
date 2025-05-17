#include "skeletal_model_instance.hpp"
#include "skeletal_model.hpp"


#include "animation/model_sequence/model_sequence.hpp"


mdlSkeletalModelInstance::~mdlSkeletalModelInstance() {
    if (!prototype) {
        return;
    }
    prototype->destroyInstance(this);
}

Handle<TransformNode> mdlSkeletalModelInstance::getBoneProxy(const std::string& name) {
    if (!getSkeletonMaster()) {
        return 0;
    }

    return instance_data.skeleton_instance->getBoneNode(name.c_str());/*
    auto it = bone_proxies.find(name);
    if (it == bone_proxies.end()) {
        sklBone* bone = getSkeletonMaster()->findBone(name.c_str());
        if (!bone) {
            return 0;
        }
        BoneProxy prox;
        prox.bone_idx = bone->getIndex();
        prox.proxy.reset_acquire();
        it = bone_proxies.insert(
            std::make_pair(name, prox)
        ).first;
    }
    return it->second.proxy.getHandle();*/
}

Handle<TransformNode> mdlSkeletalModelInstance::getBoneProxy(int idx) {
    return instance_data.skeleton_instance->getBoneNode(idx);
}

void mdlSkeletalModelInstance::enableTechnique(const char* path, bool value) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->enableTechnique(this, path, value);
}
void mdlSkeletalModelInstance::setParam(const char* param_name, GPU_TYPE type, const void* pvalue) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->setParam(this, param_name, type, pvalue);
}

void mdlSkeletalModelInstance::setExternalRootTransform(Handle<TransformNode> node) {
    instance_data.skeleton_instance->setExternalRootTransform(node);
}

void mdlSkeletalModelInstance::applySampleBuffer(animModelSampleBuffer& buf) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->applySampleBuffer(this, buf);
}

void mdlSkeletalModelInstance::updateWorldTransform(const gfxm::mat4& world) {}

void mdlSkeletalModelInstance::spawn(scnRenderScene* scn) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->spawnInstance(this, scn);
}
void mdlSkeletalModelInstance::despawn(scnRenderScene* scn) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->despawnInstance(this, scn);
}