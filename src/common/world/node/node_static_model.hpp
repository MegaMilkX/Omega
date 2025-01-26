#pragma once

#include "node_static_model.auto.hpp"
#include "world/world.hpp"
#include "resource/resource.hpp"
#include "static_model/static_model.hpp"


[[cppi_class]];
class StaticModelNode : public ActorNode {
    RHSHARED<StaticModel> model;
    HSHARED<StaticModelInstance> model_instance;

public:
    TYPE_ENABLE();
    
    void setModel(RHSHARED<StaticModel> model) {
        this->model = model;
        model_instance = model->createInstance();
        model_instance->setTransformNode(getTransformHandle());
    }

    void onDefault() override {}
    void onUpdateTransform() override {
        // ???
    }

    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(RuntimeWorld* world) override {
        model_instance->spawn(world->getRenderScene());
    }
    void onDespawn(RuntimeWorld* world) override {
        model_instance->despawn(world->getRenderScene());
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