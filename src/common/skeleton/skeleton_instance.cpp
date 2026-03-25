#include "skeleton_instance.hpp"

#include "skeleton_editable.hpp"
#include "gpu/gpu.hpp"


#include "util/static_block.hpp"
STATIC_BLOCK{
    SkeletonInstance::reflect();
}

SkeletonInstance::SkeletonInstance() {

}
SkeletonInstance::~SkeletonInstance() {
    for (auto kv : transform_blocks) {
        gpuRemoveTransformSync(kv.second);
        gpuGetDevice()->destroyParamBlock(kv.second);
    }
    transform_blocks.clear();
}

const int* SkeletonInstance::getParentArrayPtr() {
    return prototype->getParentArrayPtr();
}

Handle<TransformNode> SkeletonInstance::getBoneNode(const char* name) {
    assert(prototype);
    auto bone = prototype->findBone(name);
    if (!bone) {
        return Handle<TransformNode>();
    }
    return bone_nodes[bone->getIndex()];
}

gpuTransformBlock* SkeletonInstance::getTransformBlock(const char* name) {
    assert(prototype);
    auto bone = prototype->findBone(name);
    if (!bone) {
        return nullptr;
    }
    return getTransformBlock(bone->getIndex());
}
gpuTransformBlock* SkeletonInstance::getTransformBlock(int bone_idx) {
    auto it = transform_blocks.find(bone_idx);
    if (it != transform_blocks.end()) {
        return it->second;
    }
    it = transform_blocks.insert(
        std::make_pair(
            bone_idx,
            gpuGetDevice()->createParamBlock<gpuTransformBlock>()
        )
    ).first;
    HTransform transform_node = getBoneNode(bone_idx);
    gpuAddTransformSync(it->second, transform_node);
    return it->second;
}

void SkeletonInstance::setExternalRootTransform(Handle<TransformNode> node) {
    for (int i = 0; i < prototype->boneCount(); ++i) {
        int p = prototype->getParentArrayPtr()[i];
        if (p != -1) {
            continue;
        }
        transformNodeAttach(node, bone_nodes[i]);
    }
}

int SkeletonInstance::findBoneIndex(const char* name) const {
    if (!prototype) {
        assert(false);
        return -1;
    }
    auto bone = prototype->findBone(name);
    if (!bone) {
        assert(false);
        return -1;
    }
    return bone->getIndex();
}

/*
void SkeletonPose::calcWorldTransforms() {
    for (int j = 1; j < prototype->boneCount(); ++j) {
        int parent = prototype->getParentArrayPtr()[j];;
        world_transforms[j]
            = world_transforms[parent]
            * local_transforms[j];
    }
}*/

void SkeletonInstance::reflect() {
    // TODO
}