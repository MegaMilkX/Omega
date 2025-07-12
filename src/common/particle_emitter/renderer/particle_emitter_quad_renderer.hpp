#pragma once

#include "particle_emitter_renderer.hpp"

#include "resource/resource.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu.hpp"
#include "render_scene/render_object/scn_mesh_object.hpp"


class QuadParticleRendererInstance;
class QuadParticleRendererMaster : public IParticleRendererMasterT<QuadParticleRendererInstance> {

    RHSHARED<gpuTexture2d> texture;
    HSHARED<gpuShaderProgram> prog;
    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;
    gpuMeshDesc meshDesc;
    gpuMaterial* mat = 0;
    //std::unique_ptr<gpuRenderable> renderable;
public:
    TYPE_ENABLE();
    void init() override {/*
        texture = resGet<gpuTexture2d>("textures/particles/particle_star.png");*/
        prog = resGet<gpuShaderProgram>("shaders/particle2.glsl");

        float vertices[] = {
            -.5f, -.5f, 0,
            0.5f, -.5f, 0,
            -.5f, 0.5f, 0,
            0.5f, 0.5f, 0
        };
        float uvs[] = { .0f, .0f, 1.f, .0f,   .0f, 1.f, 1.f, 1.f };
        vertexBuffer.setArrayData(vertices, sizeof(vertices));
        uvBuffer.setArrayData(uvs, sizeof(uvs));

        meshDesc.setAttribArray(VFMT::Position_GUID, &vertexBuffer);
        meshDesc.setAttribArray(VFMT::UV_GUID, &uvBuffer);
        meshDesc.setVertexCount(4);
        meshDesc.setDrawMode(MESH_DRAW_TRIANGLE_STRIP);

        mat = gpuGetPipeline()->createMaterial();
        mat->addSampler("tex", texture);
        auto pass = mat->addPass("VFX");
        pass->setShaderProgram(prog);
        pass->blend_mode = GPU_BLEND_MODE::ADD;
        pass->depth_write = 0;
        mat->compile();
    }

    void setTexture(const RHSHARED<gpuTexture2d>& tex) {
        texture = tex;
        if (mat) {
            mat->addSampler("tex", texture);
            mat->compile();
        }
    }
    RHSHARED<gpuTexture2d> getTexture() const {
        return texture;
    }

    const gpuMeshDesc* getMeshDesc() const {
        return &meshDesc;
    }
    gpuMaterial* getMaterial() {
        return mat;
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
        scn_mesh->setMaterial(master->getMaterial());
        scn_mesh->getRenderable(0)->setInstancingDesc(&pd->instDesc);
    }
    void onSpawn(scnRenderScene* scn) override {
        scn->addRenderObject(scn_mesh.get());
    }
    void onDespawn(scnRenderScene* scn) override {
        scn->removeRenderObject(scn_mesh.get());
    }
};
