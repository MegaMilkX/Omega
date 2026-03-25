#include "skeletal_model_instance.hpp"
#include "skeletal_model.hpp"


#include "animation/model_sequence/model_sequence.hpp"


SkeletalModelInstance::SkeletalModelInstance() {
    vis_proxy.setModel(this);
}
SkeletalModelInstance::~SkeletalModelInstance() {
    if (!prototype) {
        return;
    }
    prototype->destroyInstance(this);
}

void SkeletalModelInstance::submit(gpuRenderBucket* bucket) {
    if (!prototype) {
        return;
    }
    prototype->submit(this, bucket);
}

Handle<TransformNode> SkeletalModelInstance::getBoneProxy(const std::string& name) {
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

Handle<TransformNode> SkeletalModelInstance::getBoneProxy(int idx) {
    return instance_data.skeleton_instance->getBoneNode(idx);
}

void SkeletalModelInstance::enableTechnique(const char* path, bool value) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->enableTechnique(this, path, value);
}
void SkeletalModelInstance::setParam(const char* param_name, GPU_TYPE type, const void* pvalue) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->setParam(this, param_name, type, pvalue);
}

void SkeletalModelInstance::setExternalRootTransform(Handle<TransformNode> node) {
    instance_data.skeleton_instance->setExternalRootTransform(node);
    // TODO: What if no external root node?
    vis_proxy.setTransformNode(node);
}

void SkeletalModelInstance::applySampleBuffer(animModelSampleBuffer& buf) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->applySampleBuffer(this, buf);
}

void SkeletalModelInstance::updateWorldTransform(const gfxm::mat4& world) {}

void SkeletalModelInstance::spawnModel(SceneSystem* scene_sys, scnRenderScene* scn) {
    if (!prototype) {
        assert(false);
        return;
    }
    // TODO: remove the condition once fully transitioned to SceneSystem
    if (scene_sys) {
        scene_sys->addProxy(&vis_proxy);
    }
    if(scn) {
        prototype->spawnInstance(this, scn);
    }
}
void SkeletalModelInstance::despawnModel(SceneSystem* scene_sys, scnRenderScene* scn) {
    if (!prototype) {
        assert(false);
        return;
    }
    // TODO: remove the condition once fully transitioned to SceneSystem
    if (scene_sys) {
        scene_sys->removeProxy(&vis_proxy);
    }
    if(scn) {
        prototype->despawnInstance(this, scn);
    }
}