#include "gpu/gpu.hpp"

#include "platform/platform.hpp"
#include "gpu/render_bucket.hpp"

#include "reflection/reflection.hpp"

static build_config::gpuPipelineCommon* s_pipeline = 0;
//static gpuRenderBucket* s_renderBucket = 0;

#include "readwrite/rw_gpu_material.hpp"
#include "readwrite/rw_gpu_texture_2d.hpp"
#include "readwrite/rw_gpu_shader_program.hpp"
#include "readwrite/rw_gpu_mesh.hpp"

#include "resource_cache/res_cache_gpu_material.hpp"
#include "resource_cache/res_cache_shader_program.hpp"
#include "resource_cache/res_cache_texture_2d.hpp"
#include "resource_cache/res_cache_gpu_mesh.hpp"

bool gpuInit(build_config::gpuPipelineCommon* pp) {
    s_pipeline = pp;
    //s_renderBucket = new gpuRenderBucket(pp, 10000);

    type_register<gpuMesh>("gpuMesh")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuMeshJson(j, (gpuMesh*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuMeshJson(j, (gpuMesh*)object);
        });
    type_register<gpuMaterial>("gpuMaterial")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuMaterialJson(j, (gpuMaterial*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuMaterialJson(j, (gpuMaterial*)object);
        });
    type_register<gpuTexture2d>("gpuTexture2d")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuTexture2dJson(j, (gpuTexture2d*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuTexture2dJson(j, (gpuTexture2d*)object);
        });
    type_register<gpuShaderProgram>("gpuShaderProgram")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuShaderProgramJson(j, (gpuShaderProgram*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuShaderProgramJson(j, (gpuShaderProgram*)object);
        });


    resAddCache<gpuShaderProgram>(new resCacheShaderProgram);
    resAddCache<gpuTexture2d>(new resCacheTexture2d);
    resAddCache<gpuMaterial>(new resCacheGpuMaterial(pp));
    resAddCache<gpuMesh>(new resCacheGpuMesh());

    return true;
}
void gpuCleanup() {
    //delete s_renderBucket;
}

build_config::gpuPipelineCommon* gpuGetPipeline() {
    return s_pipeline;
}

/*
void gpuDrawRenderable(gpuRenderable* r) {
    s_renderBucket->add(r);
}

void gpuClearQueue() {
    s_renderBucket->clear();
}*/


void drawPass(gpuPipeline* pipe, gpuRenderTarget* target, gpuRenderBucket* bucket, const char* technique_name, int pass) {
    auto pipe_tech = pipe->findTechnique(technique_name);
    auto pipe_pass = pipe_tech->getPass(pass);

    auto framebuffer_id = pipe_pass->getFrameBufferId();
    assert(framebuffer_id >= 0);
    gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

    glViewport(0, 0, target->getWidth(), target->getHeight());
    glScissor(0, 0, target->getWidth(), target->getHeight());

    auto group = bucket->getTechniqueGroup(pipe_tech->getId());
    for (int i = group.start; i < group.end;) { // all commands of the same technique
        auto& cmd = bucket->commands[i];
        int material_end = cmd.next_material_id;

        const gpuMaterial* material = cmd.renderable->getMaterial();
        //material->bindSamplers();
        material->bindUniformBuffers();

        for (; i < material_end;) { // iterate over commands with the same material
            auto material_tech = cmd.renderable->getMaterial()->getTechniqueByPipelineId(cmd.id.getTechnique());
            auto pass = material_tech->getPass(cmd.id.getPass());
            pass->depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
            pass->stencil_test ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
            pass->cull_faces ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
            pass->depth_write ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
            switch (pass->blend_mode) {
            case GPU_BLEND_MODE::NORMAL:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case GPU_BLEND_MODE::ADD:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case GPU_BLEND_MODE::MULTIPLY:
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
            default:
                assert(false);
            }
            pass->bindSamplers();
            for (int pobid = 0; pobid < pass->passOutputBindingCount(); ++pobid) {
                auto& pob = pass->getPassOutputBinding(pobid);
                glActiveTexture(GL_TEXTURE0 + pob.texture_slot);
                auto& texture = target->textures[pipe_pass->getTargetSamplerTextureIndex(pob.strid)];
                glBindTexture(GL_TEXTURE_2D, texture->getId());
            }
            pass->bindDrawBuffers();
            pass->bindShaderProgram();

            int pass_end = cmd.next_pass_id;
            for (; i < pass_end; ++i) { // iterate over commands with the same shader(pass)
                auto& cmd = bucket->commands[i];
                if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                    cmd.renderable->bindUniformBuffers();
                    gpuUseMeshBinding(cmd.binding);
                    cmd.renderable->getMeshDesc()->_drawInstanced(cmd.renderable->getInstancingDesc()->getInstanceCount());
                } else {
                    cmd.renderable->bindUniformBuffers();
                    gpuUseMeshBinding(cmd.binding);
                    cmd.renderable->getMeshDesc()->_draw();
                }
            }
        }
    }
};

#include "debug_draw/debug_draw.hpp"
void gpuDraw(gpuRenderBucket* bucket, gpuRenderTarget* target) {
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_TRUE);

    //glClearColor(0.129f, 0.586f, 0.949f, 1.0f);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    for (int i = 0; i < target->framebuffers.size(); ++i) {
        gpuFrameBufferBind(target->framebuffers[i].get());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }    

    bucket->sort();
    s_pipeline->bindUniformBuffers();
    
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    GLuint gvao;
    glGenVertexArrays(1, &gvao);
    glBindVertexArray(gvao);

    drawPass(s_pipeline, target, bucket, "Normal", 0);
    drawPass(s_pipeline, target, bucket, "Decals", 0);
    drawPass(s_pipeline, target, bucket, "VFX", 0);
    drawPass(s_pipeline, target, bucket, "Debug", 0);
    drawPass(s_pipeline, target, bucket, "GUI", 0);

    glDeleteVertexArrays(1, &gvao);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}