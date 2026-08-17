#pragma once

#include "particle_emitter_renderer.hpp"

#include "resource/resource.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/mesh/mesh_base.hpp"
#include "gpu/mesh/cube_mesh.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu.hpp"
#include "render_scene/render_object/scn_mesh_object.hpp"


class QuadParticleRendererInstance;
class QuadParticleRendererMaster : public IParticleRendererMasterT<QuadParticleRendererInstance> {
    /*
    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;
    gpuMeshDesc meshDesc;
    */
    ResourceRef<Mesh> mesh;
    ResourceRef<gpuMaterial> mat;
public:
    TYPE_ENABLE();
    void init() override {
        mesh = gpuGetDevice()->getSharedResources()->getQuadMesh();
    }

    void setMesh(const ResourceRef<Mesh>& m) { mesh = m; }
    ResourceRef<Mesh> getMesh() const { return mesh; }

    void setMaterial(const ResourceRef<gpuMaterial>& m) { mat = m; }
    ResourceRef<gpuMaterial> getMaterial() const { return mat; }

    const gpuMeshDesc* getMeshDesc() const {
        return mesh ? mesh->getMeshDesc() : nullptr;
    }

    void onInstanceCreated(QuadParticleRendererInstance* inst) const override {
        // TODO:
    }
};

class QuadParticleRendererInstance : public IParticleRendererInstanceT<QuadParticleRendererMaster> {
    HSHARED<scnMeshObject> scn_mesh;
public:
    void init(ptclParticleData* pd) {
        scn_mesh.reset_acquire();

        auto master = getMaster();

        scn_mesh->setMeshDesc(master->getMeshDesc());
        scn_mesh->setMaterial(master->getMaterial().get());
        scn_mesh->getRenderable(0)->setInstancingDesc(&pd->instDesc);
        scn_mesh->getRenderable(0)->dbg_billboard = true;
    }
    void onSpawn(scnRenderScene* scn) override {
        scn->addRenderObject(scn_mesh.get());
    }
    void onDespawn(scnRenderScene* scn) override {
        scn->removeRenderObject(scn_mesh.get());
    }
};
