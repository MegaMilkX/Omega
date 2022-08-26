#include "skeleton_instance.hpp"

#include "skeleton_prototype.hpp"
#include "skeleton_editable.hpp"

#include "util/static_block.hpp"
STATIC_BLOCK{
    sklSkeletonInstance::reflect();
}

sklSkeletonInstance::sklSkeletonInstance() {

}

const int* sklSkeletonInstance::getParentArrayPtr() {
    return prototype->getParentArrayPtr();
}

int sklSkeletonInstance::findBoneIndex(const char* name) const {
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

void sklSkeletonInstance::calcWorldTransforms() {
    for (int j = 1; j < prototype->boneCount(); ++j) {
        int parent = prototype->getParentArrayPtr()[j];;
        world_transforms[j]
            = world_transforms[parent]
            * local_transforms[j];
    }
}

void sklSkeletonInstance::reflect() {
    // TODO
}