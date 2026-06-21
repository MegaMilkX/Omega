#pragma once

#include "components.auto.hpp"
#include "actor_component.hpp"

#include "math/gfxm.hpp"

#include "resource_manager/resource_ref.hpp"

#include "skeleton/skeleton_editable.hpp"
#include "skeleton/skeleton_instance.hpp"

#include "animation/animator/animator.hpp"
#include "animation/animator/animator_instance.hpp"


[[cppi_class]];
class AnimatorComponent : public ActorComponent {
    ResourceRef<AnimMachine> animator;
    AnimMachineInstance anim_inst;
public:
    TYPE_ENABLE();

    AnimatorComponent() {}
    AnimatorComponent(AnimatorComponent&&) = delete;
    AnimatorComponent& operator=(AnimatorComponent&&) = delete;

    void setAnimatorMaster(const ResourceRef<AnimMachine>& master) {
        animator = master;
        anim_inst.init(const_cast<ResourceRef<AnimMachine>&>(master));
    }

    AnimMachineInstance* getAnimatorInstance() { return &anim_inst; }
    AnimMachine* getAnimatorMaster() { return animator.get(); }
    Skeleton* getSkeletonMaster() { return animator->getSkeleton(); }

    [[cppi_decl]]
    int test0 = 13;
    [[cppi_decl]]
    float test1 = 13;
    [[cppi_decl]]
    std::string test2 = "Hello, World!";
};

