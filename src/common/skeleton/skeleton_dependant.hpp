#pragma once

#include "handle/hshared.hpp"

class Skeleton;
class sklBone;
class sklSkeletonDependant {
    HSHARED<Skeleton> skeleton;

public:
    virtual ~sklSkeletonDependant() {}

    virtual void onSkeletonSet(Skeleton* skeleton) {}
    virtual void onSkeletonRemoved(Skeleton* skeleton) {}
    virtual void onBoneAdded(sklBone* bone) {}
    virtual void onBoneRemoved(sklBone* bone) {}

    void setSkeleton(HSHARED<Skeleton>& skeleton);
    const HSHARED<Skeleton>& getSkeleton() const { return skeleton; }
    HSHARED<Skeleton>& getSkeleton() { return skeleton; }

};