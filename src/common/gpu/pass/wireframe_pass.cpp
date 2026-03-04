#include "wireframe_pass.hpp"


gpuWireframePass::gpuWireframePass() {
    setSortMode(GPU_SORT_MODE::NONE);
}

void gpuWireframePass::onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) {
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

    uint32_t last_prog_id = -1;
    uint32_t last_state_id = -1;
    uint32_t last_sampler_set_id = -1;
    auto& commands = bucket->getPassCommands(pass_id);
    for (int i = 0; i < commands.size(); ++i) { // all commands of the same technique
        auto& cmd = commands[i];
        if (last_sampler_set_id != cmd.sampler_set_id) {
            gpuBindSamplers(target, this, &cmd.rdr_pass->sampler_set);
            last_sampler_set_id = cmd.sampler_set_id;
        }
        if (last_prog_id != cmd.program_id) {
            gpuBindDrawBuffers(cmd);
            gpuBindProgram(cmd);
            last_prog_id = cmd.program_id;
        }
        if (last_state_id != cmd.state_id) {
            gpuSetModes(cmd);
            gpuSetBlending(cmd);
            last_state_id = cmd.state_id;
        }

        cmd.renderable->bindSamplerOverrides(cmd.renderable_pass_id);
        cmd.renderable->bindUniformBuffers();
        cmd.renderable->uploadUniforms(cmd.renderable_pass_id);

        auto binding = &cmd.rdr_pass->binding;
        if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
            gpuBindMeshBinding(binding);
            gpuDrawMeshBindingInstanced(binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
        } else {
            gpuBindMeshBinding(binding);
            gpuDrawMeshBinding(binding);
        }
        /*
        int material_end = cmd.next_material_id;

        const gpuMaterial* material = cmd.renderable->getMaterial();
        material->bindUniformBuffers();

        gpuBindSamplers(target, this, &cmd.rdr_pass->sampler_set);
        //gpuBindSamplers(target, this, &mat_pass->getSamplerSet());

        gpuBindDrawBuffers(cmd);
        //mat_pass->bindDrawBuffers();

        gpuBindProgram(cmd);
        //mat_pass->bindShaderProgram();

        for (; i < material_end; ++i) { // iterate over commands with the same material
            auto& cmd = bucket->commands[i];
            if (count < target->dbg_geomRangeBegin || count >= target->dbg_geomRangeEnd) {
                ++count;
                continue;
            }

            gpuSetModes(cmd);
            gpuSetBlending(cmd);

            auto binding = &cmd.rdr_pass->binding;
            if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                //cmd.renderable->bindSamplerOverrides(material_tech->material_local_tech_id, cmd.id.getPass());
                cmd.renderable->bindUniformBuffers();
                gpuBindMeshBinding(binding);
                gpuDrawMeshBindingInstanced(binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
            } else {
                //cmd.renderable->bindSamplerOverrides(material_tech->material_local_tech_id, cmd.id.getPass());
                cmd.renderable->bindUniformBuffers();
                gpuBindMeshBinding(binding);
                gpuDrawMeshBinding(binding);
            }

            ++count;
        }*/
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

