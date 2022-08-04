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