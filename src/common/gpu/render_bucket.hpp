#pragma once

#include "gpu/gpu_types.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu_pipeline.hpp"


struct gpuRenderCmd {
    RenderId id;
    mat_pass_id_t material_pass_id;
    int next_material_id;
    int next_pass_id;
    gpuRenderable* renderable;
    const gpuMeshShaderBinding* binding;
    int instance_count;
};
struct gpuRenderCmdLightOmni {
    gfxm::vec3 position;
    gfxm::vec3 color;
    float intensity;
};
struct gpuRenderCmdLightDirect {
    gfxm::vec3 direction;
    gfxm::vec3 color;
    float intensity;
};

class gpuRenderBucket {
public:
    std::vector<gpuRenderCmd> commands;
    std::vector<gpuRenderCmdLightOmni> lights_omni;
    std::vector<gpuRenderCmdLightDirect> lights_direct;
    struct PassGroup {
        int start;
        int end;
    };
private:
    std::vector<PassGroup> pass_groups;
public:
    gpuRenderBucket(gpuPipeline* pipeline, int queue_reserve) {
        commands.reserve(queue_reserve);
        pass_groups.resize(pipeline->passCount());
        std::fill(pass_groups.begin(), pass_groups.end(), PassGroup{ 0, 0 });
    }
    void clear() {
        lights_direct.clear();
        lights_omni.clear();
        
        commands.clear();
        for (auto& g : pass_groups) {
            g.start = 0;
            g.end = 0;
        }
    }
    void addLightOmni(const gfxm::vec3& pos, const gfxm::vec3& color, float intensity) {
        gpuRenderCmdLightOmni light;
        light.position = pos;
        light.color = color;
        light.intensity = intensity;
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
        auto p_binding = renderable->desc_binding;
        auto p_instancing_desc = renderable->getInstancingDesc();

        for (int j = 0; j < p_binding->binding_array.size(); ++j) {
            auto& binding = p_binding->binding_array[j];
            if (!renderable->pass_states[j]) {
                continue;
            }
            gpuRenderCmd cmd = { 0 };
            cmd.id.setPass(binding.pass);
            cmd.id.setMaterial(p_material->getGuid());
            cmd.material_pass_id = binding.material_pass;
            cmd.renderable = p_renderable;
            cmd.binding = &binding.binding;
            if (p_instancing_desc) {
                cmd.instance_count = p_instancing_desc->getInstanceCount();
            }
            commands.push_back(cmd);
        }
    }
    void sort() {
        if (commands.empty()) {
            return;
        }
        std::sort(commands.begin(), commands.end(), [](const gpuRenderCmd& a, const gpuRenderCmd& b)->bool {
            return a.id.key < b.id.key;
        });

        pass_groups[0].start = 0;

        int pass_idx = 0;
        int first_cmd_of_pass_idx = 0;
        int material_idx = 0;
        int first_cmd_of_mat_idx = 0;
        for (int i = 0; i < commands.size(); ++i) {
            auto& cmd = commands[i];

            if (pass_idx != cmd.id.getPass()) {
                pass_groups[pass_idx].end = i;
                pass_idx = cmd.id.getPass();
                pass_groups[pass_idx].start = i;

                commands[first_cmd_of_pass_idx].next_pass_id = i;
                first_cmd_of_pass_idx = i;
            }

            if (material_idx != cmd.id.getMaterial()) {
                material_idx = i;
                commands[first_cmd_of_mat_idx].next_material_id = i;
                first_cmd_of_mat_idx = i;
            }
        }
    }

    const PassGroup& getPassGroup(pipe_pass_id_t i) {
        return pass_groups[i];
    }
};