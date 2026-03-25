#pragma once

#include "node_skeletal_model.auto.hpp"
#include "world/world.hpp"
#include "render_scene/render_scene.hpp"
#include "world/common_systems/dirty_system.hpp"
#include "world/common_systems/scene_system.hpp"

#include "world/node/skeleton_node.hpp"

#include "resource/resource.hpp"
#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"


[[cppi_class]];
class SkeletalModelNode : public IDirty, public TActorNode<SceneSystem, scnRenderScene, DirtySystem> {
    ResourceRef<SkeletalModel> model;
    RHSHARED<SkeletalModelInstance> instance;
    HSHARED<SkeletonInstance> external_skeleton;
    scnRenderScene* current_scene = nullptr;
    SceneSystem* current_scene_sys = nullptr;

public:
    TYPE_ENABLE();

    SkeletalModelNode() {}

    [[cppi_decl, set("model")]]
    void setModel(ResourceRef<SkeletalModel> mdl) {
        model = mdl;
        markDirty();
    }
    [[cppi_decl, get("model")]]
    const ResourceRef<SkeletalModel>& getModel() const {
        return model;
    }
    SkeletalModelInstance* getModelInstance() { return instance.get(); }

    Handle<TransformNode> getBoneProxy(const std::string& name) {
        if (!instance.isValid()) {
            return 0;
        }
        return instance->getBoneProxy(name);
    }

    const NodeSlotDescArray& getSlots() override {
        static NodeSlotDescArray slots = {
            NodeSlotDesc{ type_get<HSHARED<SkeletonInstance>>(), LINK_READ | LINK_WRITE, eSlotUpstream },
            NodeSlotDesc{ type_get<TestDummyLinkData>(), LINK_READ, eSlotUpstream }
        };
        return slots;
    }
    void onLinkRead(int slot, const varying& in) override {
        if(slot == 0) {
            external_skeleton = *in.get<HSHARED<SkeletonInstance>>();
            markDirty();
        }
    }

    void onDefault() override {}
    void onSpawnActorNode(SceneSystem* sys) override {
        instance->spawnModel(sys, nullptr);
        current_scene_sys = sys;
    }
    void onDespawnActorNode(SceneSystem* sys) override {
        instance->despawnModel(sys, nullptr);
        current_scene_sys = nullptr;
    }
    void onSpawnActorNode(DirtySystem* sys) override {
        sys->addObject(this);
    }
    void onDespawnActorNode(DirtySystem* sys) override {
        sys->removeObject(this);
    }
    void onSpawnActorNode(scnRenderScene* scn) override {
        LOG_ERR("SkeletalModelNode::onSpawnActorNode(scnRenderScene* scn)");
        current_scene = scn;
        instance->spawnModel(nullptr, scn);
    }
    void onDespawnActorNode(scnRenderScene* scn) override {
        LOG_ERR("SkeletalModelNode::onDespawnActorNode(scnRenderScene* scn)");
        instance->despawnModel(nullptr, scn);
        current_scene = nullptr;
    }

    void onResolveDependencies() override {
        LOG_DBG("SkeletalModelNode: onResolveDependencies");
        markDirty();
    }

    void onResolveDirty() override {
        LOG_DBG("SkeletalModelNode: onResolveDirty");

        if(instance) {
            if(current_scene) {
                instance->despawnModel(current_scene_sys, current_scene);
            }
            instance.reset();
        }

        if(model) {
            if (external_skeleton) {
                instance = model->createInstance(external_skeleton);
            } else {
                instance = model->createInstance();
                instance->setExternalRootTransform(getTransformHandle());
            }

            if (current_scene) {
                instance->spawnModel(current_scene_sys, current_scene);
            }
        }
    }

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j) override {
        type_write_json(j["model"], model);
    }
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j) override {
        type_read_json(j["model"], model);
        setModel(model);
        return true;
    }
};

