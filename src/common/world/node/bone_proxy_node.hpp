#pragma once

#include "bone_proxy_node.auto.hpp"
#include "actor_node.hpp"
#include "skeleton/skeleton_instance.hpp"


[[cppi_class]];
class BoneProxyNode : public ActorNode {
    std::string bone_name;
public:
    TYPE_ENABLE();

    [[cppi_decl, set("bone_name")]]
    void setBoneName(const std::string& name) {
        bone_name = name;
    }
    [[cppi_decl, get("bone_name")]]
    const std::string& getBoneName() const {
        return bone_name;
    }

    const NodeSlotDescArray& getSlots() {
        static NodeSlotDescArray slots = {
            NodeSlotDesc{ type_get<HSHARED<SkeletonInstance>>(), LINK_READ, eSlotUpstream }
        };
        return slots;
    }

    void onLinksReset() override {}
    void onLinkRead(int slot, const varying& in) override {
        auto skl = *in.get<HSHARED<SkeletonInstance>>();
        assert(skl);
        HTransform bone_transform = skl->getBoneNode(bone_name.c_str());
        transformNodeAttach(bone_transform, getTransformHandle());
    }

    void onSpawnActorNode(WorldSystemRegistry& reg) override {}
    void onDespawnActorNode(WorldSystemRegistry& reg) override {}
    
    void dbgDraw() const override {
        dbgDrawText(getWorldTranslation(), getName(), 0xFFFFFFFF);
        dbgDrawCross(getWorldTransform(), .1f, 0xFFFFFFFF);
    }
};

