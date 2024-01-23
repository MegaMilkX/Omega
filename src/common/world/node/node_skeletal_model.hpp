#pragma once

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"


class SkeletalModelNode : public gameActorNode {
    TYPE_ENABLE();
    RHSHARED<mdlSkeletalModelInstance> mdl_inst;
public:
    void setModel(RHSHARED<mdlSkeletalModelMaster>& model) {
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
};
