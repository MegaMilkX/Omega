#pragma once

#include "node_decal.auto.hpp"

#include "world/world.hpp"
#include "render_scene/render_scene.hpp"

#include "resource/resource.hpp"
#include "render_scene/render_object/scn_decal.hpp"


[[cppi_class]];
class DecalNode : public TActorNode<scnRenderScene> {
    scnDecal scn_decal;

    gfxm::vec4 color_cache = gfxm::vec4(1, 1, 1, 1);
public:
    TYPE_ENABLE();

    DecalNode() {
        scn_decal.setTransformNode(getTransformHandle());
    }

    [[cppi_decl, set("material")]]
    void setMaterial(const ResourceRef<gpuMaterial>& mat) {
        scn_decal.setMaterial(mat);
    }
    [[cppi_decl, get("material")]]
    ResourceRef<gpuMaterial> getMaterial() const {
        return scn_decal.getMaterial();
    }

    [[cppi_decl, set("color")]]
    void setColor(const gfxm::vec4& col) {
        scn_decal.setColor(col);
        color_cache = col;
    }
    [[cppi_decl, get("color")]]
    gfxm::vec4 getColor() const {
        return color_cache;
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

    void onDefault() override {
        scn_decal.setTransformNode(getTransformHandle());
    }
    void onSpawnActorNode(scnRenderScene* scn) override {
        scn->addRenderObject(&scn_decal);
    }
    void onDespawnActorNode(scnRenderScene* scn) override {
        scn->removeRenderObject(&scn_decal);
    }
};

