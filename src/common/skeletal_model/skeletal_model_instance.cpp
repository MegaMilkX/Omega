#include "skeletal_model_instance.hpp"
#include "skeletal_model.hpp"


#include "animation/model_sequence/model_sequence.hpp"


mdlSkeletalModelInstance::~mdlSkeletalModelInstance() {
    if (!prototype) {
        return;
    }
    prototype->destroyInstance(this);
}

void mdlSkeletalModelInstance::clearBoneProxies() {
    bone_proxies.clear();
}
Handle<TransformNode> mdlSkeletalModelInstance::getBoneProxy(const std::string& name) {
    if (!getSkeletonMaster()) {
        return 0;
    }
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
    return it->second.proxy.getHandle();
}

void mdlSkeletalModelInstance::applySampleBuffer(animModelSampleBuffer& buf) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->applySampleBuffer(this, buf);
}

void mdlSkeletalModelInstance::updateWorldTransform(const gfxm::mat4& world) {
    if (!prototype) {
        assert(false);
        return;
    }

    getSkeletonInstance()->getWorldTransformsPtr()[0] = world;
    for (auto& kv : bone_proxies) {
        // TODO: CHeck if this lags one frame behind
        gfxm::mat4& skeleton_space_tr = getSkeletonInstance()->getWorldTransformsPtr()[kv.second.bone_idx];
        gfxm::mat4 tr = skeleton_space_tr;
        kv.second.proxy->setTranslation(tr[3]);
        kv.second.proxy->setRotation(gfxm::to_quat(gfxm::to_orient_mat3(tr)));
    }
}

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