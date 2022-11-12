#include "skeleton_dependant.hpp"

#include "skeleton_editable.hpp"


void sklSkeletonDependant::setSkeleton(HSHARED<sklSkeletonMaster>& skeleton) {
    if (this->skeleton) {
        onSkeletonRemoved(skeleton.get());
        this->skeleton->removeDependant(this);
    }
    this->skeleton = skeleton;
    this->skeleton->addDependant(this);
    onSkeletonSet(skeleton.get());
}