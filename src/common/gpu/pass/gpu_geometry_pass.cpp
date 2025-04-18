#include "gpu_geometry_pass.hpp"

void gpuGeometryPass::onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) {
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    if (framebuffer_id < 0) {
        assert(false);
        return;
    }
    gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        assert(false);
        return;
    }
    

    glViewport(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
    glScissor(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);

    int count = 0;
    auto group = bucket->getTechniqueGroup(technique_id);
    for (int i = group.start; i < group.end;) { // all commands of the same technique
        auto& cmd = bucket->commands[i];
        int material_end = cmd.next_material_id;

        const gpuMaterial* material = cmd.renderable->getMaterial();
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

            gpuBindSamplers(target, this, &mat_pass->getSamplerSet());
            /*
            mat_pass->bindSamplers();
            for (int pobid = 0; pobid < mat_pass->passOutputBindingCount(); ++pobid) {
                auto& pob = mat_pass->getPassOutputBinding(pobid);
                glActiveTexture(GL_TEXTURE0 + pob.texture_slot);
                auto& texture = target->textures[getColorSourceTextureIndex(pob.strid)];
                glBindTexture(GL_TEXTURE_2D, texture->getId());
            }*/

            //gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
            /*GLenum draw_buffers[] = {
                GL_COLOR_ATTACHMENT0,
                GL_NONE,
                GL_NONE,
                GL_NONE,
                GL_NONE,
                GL_NONE,
                GL_NONE,
                GL_NONE,
            };
            glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);*/
            mat_pass->bindDrawBuffers();
            GL_CHECK(;);

            mat_pass->bindShaderProgram();

            int pass_end = cmd.next_pass_id;
            for (; i < pass_end; ++i) { // iterate over commands with the same shader(pass)
                auto& cmd = bucket->commands[i];
                if (count < target->dbg_geomRangeBegin || count >= target->dbg_geomRangeEnd) {
                    ++count;
                    continue;
                }
                if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                    cmd.renderable->bindSamplerOverrides(material_tech->material_local_tech_id, cmd.id.getPass());
                    cmd.renderable->bindUniformBuffers();
                    gpuBindMeshBinding(cmd.binding);
                    gpuDrawMeshBindingInstanced(cmd.binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
                } else {
                    cmd.renderable->bindSamplerOverrides(material_tech->material_local_tech_id, cmd.id.getPass());
                    cmd.renderable->bindUniformBuffers();
                    gpuBindMeshBinding(cmd.binding);
                    gpuDrawMeshBinding(cmd.binding);
                }

                ++count;
            }
        }
    }
}
