#include "wireframe_pass.hpp"



void gpuWireframePass::onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) {
    if (!target->dbg_drawWireframe) {
        return;
    }

    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if (framebuffer_id < 0) {
        assert(false);
        return;
    }
    bindFramebuffer(target);

    glViewport(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
    glScissor(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);

    int count = 0;
    auto group = bucket->getPassGroup(technique_id);
    for (int i = group.start; i < group.end;) { // all commands of the same technique
        auto& cmd = bucket->commands[i];
        int material_end = cmd.next_material_id;

        const gpuMaterial* material = cmd.renderable->getMaterial();
        material->bindUniformBuffers();

        auto mat_pass = material->getPass(cmd.material_pass_id);
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

        mat_pass->bindDrawBuffers();
        mat_pass->bindShaderProgram();

        for (; i < material_end; ++i) { // iterate over commands with the same material
            auto& cmd = bucket->commands[i];
            if (count < target->dbg_geomRangeBegin || count >= target->dbg_geomRangeEnd) {
                ++count;
                continue;
            }
            if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                //cmd.renderable->bindSamplerOverrides(material_tech->material_local_tech_id, cmd.id.getPass());
                cmd.renderable->bindUniformBuffers();
                gpuBindMeshBinding(cmd.binding);
                gpuDrawMeshBindingInstanced(cmd.binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
            } else {
                //cmd.renderable->bindSamplerOverrides(material_tech->material_local_tech_id, cmd.id.getPass());
                cmd.renderable->bindUniformBuffers();
                gpuBindMeshBinding(cmd.binding);
                gpuDrawMeshBinding(cmd.binding);
            }

            ++count;
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

