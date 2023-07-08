#pragma once

#include "particle_emitter_renderer.hpp"

#include "resource/resource.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu.hpp"


class ptclQuadRenderer : public ptclRenderer {
    HSHARED<scnMeshObject> scn_mesh;

    RHSHARED<gpuTexture2d> texture;
    HSHARED<gpuShaderProgram> prog;
    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;
    gpuMeshDesc meshDesc;    
    gpuMaterial* mat = 0;
    //std::unique_ptr<gpuRenderable> renderable;

public:
    void init(ptclParticleData* pd) {
        scn_mesh.reset_acquire();

        texture = resGet<gpuTexture2d>("textures/particles/explosion.png");
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
        auto tech = mat->addTechnique("VFX");
        auto pass = tech->addPass();
        pass->setShader(prog);
        pass->blend_mode = GPU_BLEND_MODE::ADD;
        pass->depth_write = 0;
        mat->compile();

        scn_mesh->setMeshDesc(&meshDesc);
        scn_mesh->setMaterial(mat);
        scn_mesh->getRenderable(0)->setInstancingDesc(&pd->instDesc);
        //renderable.reset(new gpuRenderable(mat, &meshDesc, &pd->instDesc));
    }
    void onSpawn(scnRenderScene* scn) override {
        scn->addRenderObject(scn_mesh.get());
    }
    void onDespawn(scnRenderScene* scn) override {
        scn->removeRenderObject(scn_mesh.get());
    }/*
    void draw(ptclParticleData* pd, float dt) {
        if (pd->aliveCount() > 0) {
            gpuDrawRenderable(renderable.get());
        }
    }*/
};
