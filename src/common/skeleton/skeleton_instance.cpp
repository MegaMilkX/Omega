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

void sklSkeletonInstance::reflect() {
    // TODO
}