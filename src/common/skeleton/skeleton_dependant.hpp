#pragma once

#include "handle/hshared.hpp"

class sklSkeletonEditable;
class sklBone;
class sklSkeletonDependant {
    HSHARED<sklSkeletonEditable> skeleton;

public:
    virtual ~sklSkeletonDependant() {}

    virtual void onSkeletonSet(sklSkeletonEditable* skeleton) {}
    virtual void onSkeletonRemoved(sklSkeletonEditable* skeleton) {}
    virtual void onBoneAdded(sklBone* bone) {}
    virtual void onBoneRemoved(sklBone* bone) {}

    void setSkeleton(HSHARED<sklSkeletonEditable>& skeleton);
    const HSHARED<sklSkeletonEditable>& getSkeleton() const { return skeleton; }
    HSHARED<sklSkeletonEditable>& getSkeleton() { return skeleton; }

};