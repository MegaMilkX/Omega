#pragma once

#include "handle/hshared.hpp"

class sklSkeletonMaster;
class sklBone;
class sklSkeletonDependant {
    HSHARED<sklSkeletonMaster> skeleton;

public:
    virtual ~sklSkeletonDependant() {}

    virtual void onSkeletonSet(sklSkeletonMaster* skeleton) {}
    virtual void onSkeletonRemoved(sklSkeletonMaster* skeleton) {}
    virtual void onBoneAdded(sklBone* bone) {}
    virtual void onBoneRemoved(sklBone* bone) {}

    void setSkeleton(HSHARED<sklSkeletonMaster>& skeleton);
    const HSHARED<sklSkeletonMaster>& getSkeleton() const { return skeleton; }
    HSHARED<sklSkeletonMaster>& getSkeleton() { return skeleton; }

};