#pragma once

#include "anim_machine_node.auto.hpp"
#include "actor_node.hpp"

#include "animation/animator/animator.hpp"
#include "animation/animator/animator_instance.hpp"
#include "world/common_systems/animation_system.hpp"


[[cppi_class]];
class AnimMachineNode : public ActorNode {
    RHSHARED<AnimatorMaster> animator;
    HSHARED<AnimatorInstance> anim_inst;
    HSHARED<SkeletonInstance> skl_inst;
    std::unique_ptr<AnimObject> anim_obj;

    const NodeSlotDescArray& getSlots() override {
        static NodeSlotDescArray slots = {
            NodeSlotDesc{ type_get<HSHARED<SkeletonInstance>>(), LINK_READ, eSlotDownstream },
        };
        return slots;
    }
    void onLinkRead(int slot, const varying& in) override {
        assert(in.get_type() == type_get<HSHARED<SkeletonInstance>>());
        skl_inst = *in.get<HSHARED<SkeletonInstance>>();
    }
public:
    TYPE_ENABLE();

    void setAnimatorMaster(const RHSHARED<AnimatorMaster>& master) {
        animator = master;
        anim_inst = animator->createInstance();
    }

    AnimatorInstance* getAnimatorInstance() { return anim_inst.get(); }
    AnimatorMaster* getAnimatorMaster() { return animator.get(); }

    void onSpawnActorNode(WorldSystemRegistry& reg) {
        if (!anim_inst || !skl_inst) {
            return;
        }
        if(auto sys = reg.getSystem<AnimationSystem>()) {
            anim_obj.reset(new AnimObject);
            anim_obj->anim_inst = anim_inst;
            anim_obj->skl_inst = skl_inst;
            sys->addAnimObject(anim_obj.get());
        }
        
    }
    void onDespawnActorNode(WorldSystemRegistry& reg) {
        if (!anim_obj) {
            return;
        }
        if (auto sys = reg.getSystem<AnimationSystem>()) {
            sys->removeAnimObject(anim_obj.get());
        }
    }
};

