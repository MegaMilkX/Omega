#pragma once

#include "animation/animator/animator_instance.hpp"


struct AnimObject {
    HSHARED<AnimMachineInstance> anim_inst;
    HSHARED<SkeletonInstance> skl_inst;
};

class AnimationSystem {
    std::set<AnimObject*> objects;
public:
    void addAnimObject(AnimObject*);
    void removeAnimObject(AnimObject*);
    void update(float dt);
};