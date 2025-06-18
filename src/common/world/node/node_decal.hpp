#pragma once

#include "node_decal.auto.hpp"

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "render_scene/render_object/scn_decal.hpp"


[[cppi_class]];
class DecalNode : public ActorNode {
    scnDecal scn_decal;

    gfxm::vec4 color_cache = gfxm::vec4(1, 1, 1, 1);
public:
    TYPE_ENABLE();

    DecalNode() {
        scn_decal.setTransformNode(getTransformHandle());
    }

    void setMaterial(const RHSHARED<gpuMaterial>& mat) {
        scn_decal.setMaterial(mat);
    }
    RHSHARED<gpuMaterial> getMaterial() const {
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
    void onUpdateTransform() override {}
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(RuntimeWorld* world) override {
        world->getRenderScene()->addRenderObject(&scn_decal);
    }
    void onDespawn(RuntimeWorld* world) override {
        world->getRenderScene()->removeRenderObject(&scn_decal);
    }

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j) override {        
        type_write_json(j["size"], scn_decal.getBoxSize());
        type_write_json(j["color"], color_cache);
        RHSHARED<gpuMaterial> material = scn_decal.getMaterial();
        type_write_json(j["material"], material);
        //type_write_json(j["blend_mode"], scn_decal.getBlending());
    }
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j) override {
        gfxm::vec3 size;
        type_read_json(j["size"], size);
        scn_decal.setBoxSize(size);
        type_read_json(j["color"], color_cache);
        scn_decal.setColor(color_cache);
        RHSHARED<gpuMaterial> material;
        type_read_json(j["material"], material);
        scn_decal.setMaterial(material);
        return true;
    }
};

