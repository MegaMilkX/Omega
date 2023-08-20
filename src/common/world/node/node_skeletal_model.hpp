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

    void onDefault() override {}
    void onUpdateTransform() override {
        mdl_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = getWorldTransform();
    }
    void onUpdate(GameWorld* world, float dt) override {}
    void onSpawn(GameWorld* world) override {
        mdl_inst->spawn(world->getRenderScene());
    }
    void onDespawn(GameWorld* world) override {
        mdl_inst->despawn(world->getRenderScene());
    }
};
