#pragma once

#include "skeletal_model.auto.hpp"
#include "actor_node.hpp"
#include "world/common_systems/scene_system.hpp"
#include "m3d/m3d_model.hpp"
#include "m3d/skeletal_instance.hpp"

#include "world/node/skeleton_node.hpp"


[[cppi_class]];
class SkeletalModelNode2 : public ActorNode, public SceneProxy {
    ResourceRef<m3dModel> model;
    m3dSkeletalInstance instance;
    HSHARED<SkeletonInstance> external_skeleton;
    SceneSystem* scene_sys = nullptr;
public:
    TYPE_ENABLE();

    SkeletalModelNode2();

    [[cppi_decl, set("model")]]
    void setModel(const ResourceRef<m3dModel>& mdl);
    [[cppi_decl, get("model")]]
    ResourceRef<m3dModel> getModel() const;

    [[cppi_decl, set("layer")]]
    void setLayer(int i);
    [[cppi_decl, get("layer")]]
    int getLayer() const;

    HTransform getBoneProxy(const std::string& name);
    HTransform getBoneProxy(int idx);

    void enableTechnique(const std::string path, bool value);
    void setRenderParam(const char* param_name, GPU_TYPE type, const void* pvalue);

    // ActorNode
    void onBuild() override;
    const NodeSlotDescArray& getSlots() override;
    void onLinkRead(int slot, const rtti::varying& in) override;
    void onLinkWrite(int slot, rtti::varying& out) override;
    void onSpawnActorNode(WorldSystemRegistry& reg) override;
    void onDespawnActorNode(WorldSystemRegistry& reg) override;
    void onReady() override;
    bool checkComplete() const override;

    // SceneProxy
    void updateBounds() override;
    void submit(gpuRenderBucket*) override;

    // MetaObject
    void onSnapshot() override;
};