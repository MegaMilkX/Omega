#pragma once

#include "gpu/gpu_types.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/render_cmd.hpp"


class gpuRenderBucket {
    gpuPipeline* pipeline = nullptr;
public:
    std::vector<std::vector<gpuRenderCmd>> commands_per_pass;
    std::vector<gpuRenderCmdLightOmni> lights_omni;
    std::vector<gpuRenderCmdLightDirect> lights_direct;

    gpuRenderBucket() {}
    gpuRenderBucket(gpuPipeline* pipeline, int queue_reserve /*TODO: unused, should remove*/)
    : pipeline(pipeline) {
        commands_per_pass.resize(pipeline->passCount());
    }
    void clear() {
        lights_direct.clear();
        lights_omni.clear();
        
        for (int i = 0; i < commands_per_pass.size(); ++i) {
            commands_per_pass[i].clear();
        }
    }
    void addLightOmni(const gfxm::vec3& pos, const gfxm::vec3& color, float intensity, bool shadow) {
        gpuRenderCmdLightOmni light;
        light.position = pos;
        light.color = color;
        light.intensity = intensity;
        light.shadow = shadow;
        lights_omni.push_back(light);
    }
    void addLightDirect(const gfxm::vec3& dir, const gfxm::vec3& color, float intensity) {
        gpuRenderCmdLightDirect light;
        light.direction = dir;
        light.color = color;
        light.intensity = intensity;
        lights_direct.push_back(light);
    }
    void add(gpuRenderable* renderable) {
        auto p_renderable = renderable;
        auto p_material = p_renderable->getMaterial();
        const auto& p_binding = renderable->compiled_desc;
        auto p_instancing_desc = renderable->getInstancingDesc();

        if (!p_binding) {
            return;
        }

        for (int j = 0; j < p_binding->pass_array.size(); ++j) {
            auto& binding = p_binding->pass_array[j];
            if (!renderable->pass_states[j]) {
                continue;
            }
            gpuRenderCmd cmd = { 0 };
            //cmd.id.setPass(binding.pass);
            //cmd.id.setMaterial(p_material->getGuid());
            cmd.pass_id = binding.pass;
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
            commands_per_pass[cmd.pass_id].push_back(cmd);
        }
    }
    void sort(const DRAW_PARAMS& params) {
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
        /*
        static bool once = false;
        if (!once) {
            for(int i = 0; i < commands_per_pass.size(); ++i) {
                auto& commands = commands_per_pass[i];
                if (commands.empty()) {
                    return;
                }
                for (int j = 0; j < commands.size(); ++j) {
                    auto& c = commands[j];
                    LOG_DBG(
                        "pass: " << c.pass_id << ", prog: " << c.program_id << ", state: " << c.state_id << ", textures: " << c.sampler_set_id);
                }
                once = true;
            }
        }*/
    }

    const std::vector<gpuRenderCmd>& getPassCommands(pipe_pass_id_t i) {
        return commands_per_pass[i];
    }
};