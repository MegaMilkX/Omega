#pragma once

#include "skeletal_model.auto.hpp"
#include "actor_node.hpp"
#include "world/common_systems/scene_system.hpp"
#include "m3d/m3d_model.hpp"
#include "m3d/skeletal_instance.hpp"


[[cppi_class]];
class SkeletalModelNode2 : public ActorNode, public SceneProxy {
    ResourceRef<m3dModel> model;
    m3dSkeletalInstance instance;
    HSHARED<SkeletonInstance> external_skeleton;
public:
    TYPE_ENABLE();

    SkeletalModelNode2();

    [[cppi_decl, set("model")]]
    void setModel(const ResourceRef<m3dModel>& mdl);
    [[cppi_decl, get("model")]]
    ResourceRef<m3dModel> getModel() const;

    // ActorNode
    void onSpawnActorNode(WorldSystemRegistry& reg) override;
    void onDespawnActorNode(WorldSystemRegistry& reg) override;
    const NodeSlotDescArray& getSlots() override;
    void onLinkRead(int slot, const varying& in) override;

    // SceneProxy
    void updateBounds() override;
    void submit(gpuRenderBucket*) override;
};