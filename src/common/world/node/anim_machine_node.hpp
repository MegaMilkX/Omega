#pragma once

#include "anim_machine_node.auto.hpp"
#include "actor_node.hpp"

#include "animation/animator/animator.hpp"
#include "animation/animator/animator_instance.hpp"
#include "world/common_systems/animation_system.hpp"


[[cppi_class]];
class AnimMachineNode : public ActorNode {
    ResourceRef<AnimMachine> animator;
    AnimMachineInstance anim_inst;
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

    AnimMachineNode() {}
    AnimMachineNode(AnimMachineNode&&) = delete;
    AnimMachineNode& operator=(AnimMachineNode&&) = delete;

    void setAnimatorMaster(const ResourceRef<AnimMachine>& master) {
        animator = master;
    }

    AnimMachineInstance* getAnimatorInstance() { return &anim_inst; }
    AnimMachine* getAnimatorMaster() { return animator.get(); }

    void onSpawnActorNode(WorldSystemRegistry& reg) {
        if (!skl_inst) {
            return;
        }
        
        anim_inst.init(const_cast<ResourceRef<AnimMachine>&>(animator));

        if(auto sys = reg.getSystem<AnimationSystem>()) {
            anim_obj.reset(new AnimObject);
            anim_obj->anim_inst = &anim_inst;
            anim_obj->skl_inst = skl_inst.get();
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

