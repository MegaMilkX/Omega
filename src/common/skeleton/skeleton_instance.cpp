#include "skeleton_instance.hpp"

#include "skeleton_editable.hpp"

#include "util/static_block.hpp"
STATIC_BLOCK{
    SkeletonPose::reflect();
}

SkeletonPose::SkeletonPose() {

}

const int* SkeletonPose::getParentArrayPtr() {
    return prototype->getParentArrayPtr();
}

int SkeletonPose::findBoneIndex(const char* name) const {
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

void SkeletonPose::calcWorldTransforms() {
    for (int j = 1; j < prototype->boneCount(); ++j) {
        int parent = prototype->getParentArrayPtr()[j];;
        world_transforms[j]
            = world_transforms[parent]
            * local_transforms[j];
    }
}

void SkeletonPose::reflect() {
    // TODO
}