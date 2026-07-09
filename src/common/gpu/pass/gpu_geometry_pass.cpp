#include "gpu_geometry_pass.hpp"

gpuGeometryPass::gpuGeometryPass() {

}

void gpuGeometryPass::onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) {
    auto& commands = bucket->getPassCommands(pass_id);
    if (commands.empty()) {
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

    bindFramebuffer(inst, target_map);    

    glViewport(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
    glScissor(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);

    uint32_t last_prog_id = -1;
    uint32_t last_state_id = -1;
    uint32_t last_sampler_set_id = -1;
    for (int i = 0; i < commands.size(); ++i) {
        auto& cmd = commands[i];
        if (params.layer >= 0 && cmd.layer != params.layer) {
            // TODO: Should only iterate the relevant layer
            continue;
        }

        if (last_sampler_set_id != cmd.sampler_set_id) {
            gpuBindSamplers(target_map->getTarget(), inst, &cmd.rdr_pass->sampler_set);
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
    }
}
