#pragma once

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"


class nodeSkeletalModel : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
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
    void onUpdate(gameWorld* world, float dt) override {}
    void onSpawn(gameWorld* world) override {
        mdl_inst->spawn(world->getRenderScene());
    }
    void onDespawn(gameWorld* world) override {
        mdl_inst->despawn(world->getRenderScene());
    }
};
STATIC_BLOCK{
    type_register<nodeSkeletalModel>("nodeSkeletalModel")
        .parent<gameActorNode>();
};
