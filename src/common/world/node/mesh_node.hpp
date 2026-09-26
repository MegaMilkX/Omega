#pragma once

#include "mesh_node.auto.hpp"
#include "actor_node.hpp"

#include "gpu/gpu.hpp"
#include "gpu/mesh/mesh_base.hpp"
#include "gpu/gpu_material.hpp"
#include "world/common_systems/scene_system.hpp"
#include "gpu/renderable/geometry.hpp"


[[cppi_class]];
class MeshNode : public ActorNode, public SceneProxy {
    ResourceRef<Mesh> mesh;
    ResourceRef<gpuMaterial> material;
    gpuGeoRenderable renderable;
    bool is_dirty = true;
public:
    TYPE_ENABLE();

    MeshNode() {
        SceneProxy::setTransformNode(ActorNode::getTransformHandle());
        gpuAddTransformSync(renderable.getTransformBlock(), ActorNode::getTransformHandle());
    }
    ~MeshNode() {
        gpuRemoveTransformSync(renderable.getTransformBlock());
    }

    [[cppi_decl, set("mesh")]] void setMesh(const ResourceRef<Mesh>& m) { mesh = m; is_dirty = true; }
    [[cppi_decl, get("mesh")]] ResourceRef<Mesh> getMesh() const { return mesh; }
    [[cppi_decl, set("material")]] void setMaterial(const ResourceRef<gpuMaterial>& m) { material = m; is_dirty = true; }
    [[cppi_decl, get("material")]] ResourceRef<gpuMaterial> getMaterial() const { return material; }
    [[cppi_decl, set("billboard")]] void setBillboard(bool v) { renderable.dbg_billboard = v; is_dirty = true; }
    [[cppi_decl, get("billboard")]] bool isBillboard() const { return renderable.dbg_billboard; }
    
    bool checkComplete() const override {
        if(!mesh) return false;
        return true;
    }

    void onSpawnActorNode(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<SceneSystem>()) {
            sys->addProxy(this);
        }
    }
    void onDespawnActorNode(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<SceneSystem>()) {
            sys->removeProxy(this);
        }
    }

    void updateBounds() override {
        // TODO: Use mesh bounds
        auto node = getTransformNode();
        setBoundingSphere(.5f, node->getWorldTranslation());
        setBoundingBox(gfxm::aabb(
            node->getWorldTranslation() - gfxm::vec3(.5, .5, .5),
            node->getWorldTranslation() + gfxm::vec3(.5, .5, .5)
        ));
    }
    void submit(gpuRenderBucket* bucket) override {
        if (is_dirty) {
            renderable.clear();
            if(mesh) {
                renderable.setMeshDesc(mesh->getMeshDesc());
            }
            if (material) {
                renderable.setMaterial(material.get());
            }
            renderable.compile();
            is_dirty = false;
        }
        bucket->add(&renderable);
    }
};

