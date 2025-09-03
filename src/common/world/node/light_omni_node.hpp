#pragma once

#include "light_omni_node.auto.hpp"
#include "world/world.hpp"
#include "render_scene/render_object/light_omni.hpp"


[[cppi_class]];
class LightOmniNode : public TActorNode<scnRenderScene> {
    scnLightOmni light_object;

public:
    TYPE_ENABLE();

    LightOmniNode() {
        light_object.enable_shadows = false;
    }
    
    void setColor(const gfxm::vec3& col) {
        light_object.color = gfxm::vec3(col.x, col.y, col.z);
    }
    void setIntensity(float intensity) {
        light_object.intensity = intensity;
    }
    void setRadius(float radius) {
        light_object.radius = radius;
    }
    
    void onDefault() override {}
    void onUpdateTransform() override {}
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(scnRenderScene* scn) override {
        light_object.setTransformNode(getTransformHandle());
        scn->addRenderObject(&light_object);
    }
    void onDespawn(scnRenderScene* scn) override {
        scn->removeRenderObject(&light_object);
    }
};

