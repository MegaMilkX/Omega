#include "render_bucket.hpp"

#include "gpu/gpu_pipeline.hpp"


gpuRenderBucket::gpuRenderBucket(gpuPipeline* pipeline, int queue_reserve /*TODO: unused, should remove*/)
    : pipeline(pipeline) {
    commands_per_pass.resize(pipeline->passCount());
}
void gpuRenderBucket::clear() {
    layers.clear();
    lights_direct.clear();
    lights_omni.clear();

    for (int i = 0; i < commands_per_pass.size(); ++i) {
        commands_per_pass[i].clear();
    }
}
void gpuRenderBucket::addLightOmni(const gfxm::vec3& pos, const gfxm::vec3& color, float intensity, bool shadow) {
    gpuRenderCmdLightOmni light;
    light.position = pos;
    light.color = color;
    light.intensity = intensity;
    light.shadow = shadow;
    lights_omni.push_back(light);
}
void gpuRenderBucket::addLightDirect(const gfxm::vec3& dir, const gfxm::vec3& color, float intensity) {
    gpuRenderCmdLightDirect light;
    light.direction = dir;
    light.color = color;
    light.intensity = intensity;
    lights_direct.push_back(light);
}
void gpuRenderBucket::add(gpuRenderable* renderable) {
    auto p_renderable = renderable;
    if (!p_renderable->compiled_desc) {
        p_renderable->compile();
    }

    auto p_material = p_renderable->getMaterial();
    if (p_material && p_material->getVersion() != p_renderable->getMaterialVersion()) {
        p_renderable->compile();
    }

    const gpuCompiledRenderableDesc* compiled_desc = renderable->compiled_desc.get();
    auto p_instancing_desc = renderable->getInstancingDesc();

    if (!compiled_desc) {
        return;
    }

    for (int j = 0; j < compiled_desc->pass_array.size(); ++j) {
        auto& binding = compiled_desc->pass_array[j];
        if (!renderable->pass_states[j]) {
            continue;
        }
        pipe_pass_id_t pass_id = binding.pass;
        gpuRenderCmd cmd = { 0 };
        cmd.program_id = binding.prog->getId(); // TODO: Should use own id instead of gl id
        cmd.state_id = binding.state_identity;
        cmd.sampler_set_id = binding.sampler_set_identity;
        cmd.renderable_pass_id = j;
        cmd.renderable = p_renderable;
        cmd.rdr_pass = &binding;
        if (p_instancing_desc) {
            cmd.instance_count = p_instancing_desc->getInstanceCount();
        }
        cmd.program = binding.prog->getId();
        cmd.layer = renderable->layer_idx;
        commands_per_pass[pass_id].push_back(cmd);
        layers.insert(cmd.layer);
    }
}
void gpuRenderBucket::sort(const DRAW_PARAMS& params) {
    // NOTE: At the moment commands_per_pass contains an array for each pass of the pipeline,
    // so i is equivalent to pipe_pass_id_t. Careful if changing in the future.
    for(int i = 0; i < commands_per_pass.size(); ++i) {
        auto& commands = commands_per_pass[i];
        if (commands.size() < 2) {
            continue;
        }
        auto pass = pipeline->getPass(i);
        if (pass->hasAnyFlags(PASS_FLAG_DISABLED | PASS_FLAG_NO_DRAW)) {
            continue;
        }
        pass->sortCommands(commands.data(), commands.size(), params);
    }
}

