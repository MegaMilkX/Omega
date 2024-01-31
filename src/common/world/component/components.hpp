#pragma once

#include "components.auto.hpp"
#include "actor_component.hpp"

#include "math/gfxm.hpp"

#include "skeleton/skeleton_editable.hpp"
#include "skeleton/skeleton_instance.hpp"

#include "animation/animator/animator.hpp"
#include "animation/animator/animator_instance.hpp"

[[cppi_class]];
class AnimatorComponent : public ActorComponent {
    RHSHARED<AnimatorMaster> animator;
    HSHARED<animAnimatorInstance> anim_inst;
public:
    TYPE_ENABLE();

    AnimatorComponent() {}

    void setAnimatorMaster(const RHSHARED<AnimatorMaster>& master) {
        animator = master;
        anim_inst = animator->createInstance();
    }

    animAnimatorInstance* getAnimatorInstance() { return anim_inst.get(); }
    AnimatorMaster* getAnimatorMaster() { return animator.get(); }
    Skeleton* getSkeletonMaster() { return animator->getSkeleton(); }
};