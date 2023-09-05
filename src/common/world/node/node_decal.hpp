#pragma once

#include "node_decal.auto.hpp"

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "render_scene/render_object/scn_decal.hpp"


[[cppi_class]];
class DecalNode : public gameActorNode {
    TYPE_ENABLE();
    scnNode scn_node;
    scnDecal scn_decal;
public:
    [[cppi_decl, set("color")]]
    void setColor(const gfxm::vec4& col) {
        scn_decal.setColor(col);
    }
    [[cppi_decl, get("color")]]
    const gfxm::vec4 getColor() const {
        // TODO:
        return gfxm::vec4(1, 1, 1, 1);
    }

    void setTexture(RHSHARED<gpuTexture2d> tex) {
        scn_decal.setTexture(tex);
    }

    [[cppi_decl, set("size")]]
    void setSize(const gfxm::vec3& sz) {
        scn_decal.setBoxSize(sz);
    }
    void setSize(float x, float y, float z) {
        setSize(gfxm::vec3(x, y, z));
    }
    [[cppi_decl, get("size")]]
    const gfxm::vec3& getSize() const {
        return scn_decal.getBoxSize();
    }

    void setBlendMode(GPU_BLEND_MODE mode) {
        scn_decal.setBlending(mode);
    }

    void onDefault() override {
        scn_decal.setNode(&scn_node);
    }
    void onUpdateTransform() override {
        scn_node.local_transform = getWorldTransform();
        scn_node.world_transform = getWorldTransform();
    }
    void onUpdate(GameWorld* world, float dt) override {}
    void onSpawn(GameWorld* world) override {
        world->getRenderScene()->addRenderObject(&scn_decal);
    }
    void onDespawn(GameWorld* world) override {
        world->getRenderScene()->removeRenderObject(&scn_decal);
    }
};

