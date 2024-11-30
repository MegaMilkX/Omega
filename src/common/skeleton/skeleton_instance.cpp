#include "skeleton_instance.hpp"

#include "skeleton_editable.hpp"

#include "util/static_block.hpp"
STATIC_BLOCK{
    SkeletonInstance::reflect();
}

SkeletonInstance::SkeletonInstance() {

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