#pragma once

#include "gpu_pass.hpp"


class gpuGeometryPass : public gpuPass {
public:
    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id) override {
        if (framebuffer_id < 0) {
            assert(false);
            return;
        }
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

        glViewport(0, 0, target->getWidth(), target->getHeight());
        glScissor(0, 0, target->getWidth(), target->getHeight());

        auto group = bucket->getTechniqueGroup(technique_id);
        for (int i = group.start; i < group.end;) { // all commands of the same technique
            auto& cmd = bucket->commands[i];
            int material_end = cmd.next_material_id;

            const gpuMaterial* material = cmd.renderable->getMaterial();
            //material->bindSamplers();
            material->bindUniformBuffers();

            for (; i < material_end;) { // iterate over commands with the same material
                auto material_tech = cmd.renderable->getMaterial()->getTechniqueByPipelineId(cmd.id.getTechnique());
                auto mat_pass = material_tech->getPass(cmd.id.getPass());
                mat_pass->depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
                mat_pass->stencil_test ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
                mat_pass->cull_faces ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
                mat_pass->depth_write ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
                switch (mat_pass->blend_mode) {
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
                mat_pass->bindSamplers();
                for (int pobid = 0; pobid < mat_pass->passOutputBindingCount(); ++pobid) {
                    auto& pob = mat_pass->getPassOutputBinding(pobid);
                    glActiveTexture(GL_TEXTURE0 + pob.texture_slot);
                    auto& texture = target->textures[pipe_pass->getTargetSamplerTextureIndex(pob.strid)];
                    glBindTexture(GL_TEXTURE_2D, texture->getId());
                }
                mat_pass->bindDrawBuffers();/*
                GLenum draw_buffers[] = {
                    GL_COLOR_ATTACHMENT0 + 0,
                    GL_COLOR_ATTACHMENT0 + 1,
                    GL_COLOR_ATTACHMENT0 + 2,
                    GL_COLOR_ATTACHMENT0 + 3,
                    GL_COLOR_ATTACHMENT0 + 4,
                    0,
                    0,
                    0,
                    0,
                };
                glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);*/
                mat_pass->bindShaderProgram();

                int pass_end = cmd.next_pass_id;
                for (; i < pass_end; ++i) { // iterate over commands with the same shader(pass)
                    auto& cmd = bucket->commands[i];
                    if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                        cmd.renderable->bindUniformBuffers();
                        gpuBindMeshBinding(cmd.binding);
                        gpuDrawMeshBindingInstanced(cmd.binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
                    } else {
                        cmd.renderable->bindUniformBuffers();
                        gpuBindMeshBinding(cmd.binding);
                        gpuDrawMeshBinding(cmd.binding);
                    }
                }
            }
        }
    }
};