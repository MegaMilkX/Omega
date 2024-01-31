#pragma once

#include "node_skeletal_model.auto.hpp"
#include "world/world.hpp"

#include "resource/resource.hpp"
#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"


[[cppi_class]];
class SkeletalModelNode : public gameActorNode {
    RHSHARED<mdlSkeletalModelMaster> mdl_master;
    RHSHARED<mdlSkeletalModelInstance> mdl_inst;
public:
    TYPE_ENABLE();
    void setModel(RHSHARED<mdlSkeletalModelMaster>& model) {
        mdl_master = model;
        mdl_inst = model->createInstance();
    }
    mdlSkeletalModelInstance* getModelInstance() { return mdl_inst.get(); }

    Handle<TransformNode> getBoneProxy(const std::string& name) {
        if (!mdl_inst.isValid()) {
            return 0;
        }
        return mdl_inst->getBoneProxy(name);
    }

    void onDefault() override {}
    void onUpdateTransform() override {
        mdl_inst->updateWorldTransform(getWorldTransform());
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(RuntimeWorld* world) override {
        mdl_inst->spawn(world->getRenderScene());
    }
    void onDespawn(RuntimeWorld* world) override {
        mdl_inst->despawn(world->getRenderScene());
    }

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j) override {
        type_write_json(j["model"], mdl_master);
    }
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j) override {
        type_read_json(j["model"], mdl_master);
        setModel(mdl_master);
        return true;
    }
};
