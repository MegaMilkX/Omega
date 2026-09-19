#pragma once

#include "node_static_model.auto.hpp"
#include "world/world.hpp"
#include "render_scene/render_scene.hpp"
#include "resource/resource.hpp"
#include "static_model/static_model.hpp"


[[cppi_class]];
class StaticModelNode : public TActorNode<scnRenderScene> {
    ResourceRef<StaticModel> model;
    HSHARED<StaticModelInstance> model_instance;

public:
    TYPE_ENABLE();
    
    void setModel(ResourceRef<StaticModel> model) {
        this->model = model;
        model_instance = model->createInstance();
        model_instance->setTransformNode(getTransformHandle());
    }

    void onDefault() override {}

    void onSpawnActorNode(scnRenderScene* scn) override {
        model_instance->spawn(scn);
    }
    void onDespawnActorNode(scnRenderScene* scn) override {
        model_instance->despawn(scn);
    }
};

