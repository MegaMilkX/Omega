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

    void onBuild() override {
        anim_inst.init(animator);
    }

    const NodeSlotDescArray& getSlots() override {
        static NodeSlotDescArray slots = {
            NodeSlotDesc{ rtti::type_get<HSHARED<SkeletonInstance>>(), LINK_READ, eSlotDownstream },
        };
        return slots;
    }
    void onLinkRead(int slot, const rtti::varying& in) override {
        assert(in.get_type() == rtti::type_get<HSHARED<SkeletonInstance>>());
        skl_inst = *in.get<HSHARED<SkeletonInstance>>();
    }
    void onReady() override {
        if (!skl_inst) {
            anim_obj.reset(nullptr);
            return;
        }

        anim_obj.reset(new AnimObject);
        anim_obj->anim_inst = &anim_inst;
        anim_obj->skl_inst = skl_inst.get();
    }
public:
    TYPE_ENABLE();

    AnimMachineNode() {}
    AnimMachineNode(AnimMachineNode&&) = delete;
    AnimMachineNode& operator=(AnimMachineNode&&) = delete;

    void setAnimatorMaster(const ResourceRef<AnimMachine>& master) {
        animator = master;
        requestRebuild();
    }

    AnimMachineInstance* getAnimatorInstance() { return &anim_inst; }
    AnimMachine* getAnimatorMaster() {
        if (!animator) {
            return nullptr;
        }
        return animator.get();
    }

    void onSpawnActorNode(WorldSystemRegistry& reg) {
        if (!anim_obj) {
            return;
        }
        if(auto sys = reg.getSystem<AnimationSystem>()) {
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

