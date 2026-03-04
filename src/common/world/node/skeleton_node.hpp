#pragma once

#include "skeleton_node.auto.hpp"
#include "actor_node.hpp"
#include "skeleton/skeleton_editable.hpp"
#include "debug_draw/debug_draw.hpp"


[[cppi_class]];
class SkeletonNode : public ActorNode {
    ResourceRef<Skeleton> skeleton;
    HSHARED<SkeletonInstance> skeleton_instance;
public:
    TYPE_ENABLE();

    [[cppi_decl, set("skeleton")]]
    void setSkeleton(ResourceRef<Skeleton> skl) {
        skeleton = skl;
        skeleton_instance = skeleton->createInstance();
        skeleton_instance->setExternalRootTransform(getTransformHandle());
        // TODO: Update dependents somehow
    }
    [[cppi_decl, get("skeleton")]]
    ResourceRef<Skeleton> getSkeleton() const {
        return skeleton;
    }

    HSHARED<SkeletonInstance> getSkeletonInstance() {
        return skeleton_instance;
    }

    const NodeSlotDescArray& getSlots() override {
        static NodeSlotDescArray slots = {
            NodeSlotDesc{ type_get<HSHARED<SkeletonInstance>>(), LINK_WRITE, eSlotUpstream },
            NodeSlotDesc{ type_get<HSHARED<SkeletonInstance>>(), LINK_WRITE, eSlotDownstream },

            NodeSlotDesc{ type_get<TestDummyLinkData>(), LINK_READ, eSlotUpstream },
            NodeSlotDesc{ type_get<TestDummyLinkData>(), LINK_WRITE, eSlotDownstream }
        };
        return slots;
    }
    void onLinkWrite(int slot, varying& out) override {
        // Both slots are the same, can treat them the same way
        if(slot == 0 || slot == 1) {
            out = varying::make(skeleton_instance);
        }
    }
    void onSpawnActorNode(WorldSystemRegistry& reg) override {}
    void onDespawnActorNode(WorldSystemRegistry& reg) override {}

    void dbgDraw() const {
        if(!skeleton) return;
        if(!skeleton_instance) return;
        auto skel = const_cast<Skeleton*>(skeleton.get());
        auto inst = const_cast<SkeletonInstance*>(skeleton_instance.get());

        for (int j = int(skel->boneCount()) - 1; j > 0; --j) {
            int parent = skel->getParentArrayPtr()[j];
            dbgDrawLine(
                inst->getBoneNode(j)->getWorldTranslation(),
                inst->getBoneNode(parent)->getWorldTranslation(),
                0xFFFFFFFF
            );
        }
    }
};

