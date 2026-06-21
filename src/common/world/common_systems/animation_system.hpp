#pragma once

#include "animation/animator/animator_instance.hpp"


struct AnimObject {
    AnimMachineInstance* anim_inst = nullptr;
    SkeletonInstance* skl_inst = nullptr;
};

class AnimationSystem {
    std::set<AnimObject*> objects;
public:
    void addAnimObject(AnimObject*);
    void removeAnimObject(AnimObject*);
    void update(float dt);
};