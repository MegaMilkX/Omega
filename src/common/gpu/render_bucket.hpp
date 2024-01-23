#pragma once

#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu_pipeline.hpp"


struct gpuRenderCmd {
    RenderId id;
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
    struct TechniqueGroup {
        int start;
        int end;
    };
private:
    std::vector<TechniqueGroup> technique_groups;
public:
    gpuRenderBucket(gpuPipeline* pipeline, int queue_reserve) {
        commands.reserve(queue_reserve);
        technique_groups.resize(pipeline->techniqueCount());
        std::fill(technique_groups.begin(), technique_groups.end(), TechniqueGroup{ 0, 0 });
    }
    void clear() {
        lights_direct.clear();
        lights_omni.clear();
        
        commands.clear();
        for (auto& g : technique_groups) {
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
            gpuRenderCmd cmd = { 0 };
            cmd.id.setTechnique(binding.technique);
            cmd.id.setPass(binding.pass);
            cmd.id.setMaterial(p_material->getGuid());
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

        int technique_id = 0;
        technique_groups[0].start = 0;

        int material_id = -1;
        int material_entry_id = 0;
        int pass_id = -1;
        int pass_entry_id = 0;
        for (int i = 0; i < commands.size(); ++i) {
            auto& cmd = commands[i];

            int cmd_tech_id = cmd.id.getTechnique();
            int cmd_pass_id = cmd.id.getPass();
            int cmd_mat_id = cmd.id.getMaterial();
            if (technique_id != cmd.id.getTechnique()) {
                technique_groups[technique_id].end = i;
                technique_id = cmd.id.getTechnique();
                technique_groups[technique_id].start = i;

                commands[material_entry_id].next_material_id = i;
                commands[pass_entry_id].next_pass_id = i;
                material_id = -1;
                material_entry_id = i;
                pass_id = -1;
                pass_entry_id = i;
            }
            else if (material_id != cmd.id.getMaterial()) {
                commands[material_entry_id].next_material_id = i;
                commands[pass_entry_id].next_pass_id = i;
                material_entry_id = i;
                pass_entry_id = i;

                material_id = cmd.id.getMaterial();
                pass_id = cmd.id.getPass();
            }
            else if (pass_id != cmd.id.getPass()) {
                commands[pass_entry_id].next_pass_id = i;
                pass_entry_id = i;

                pass_id = cmd.id.getPass();
            }
        }
        technique_groups[technique_id].end = commands.size();
        commands[material_entry_id].next_material_id = commands.size();
        commands[pass_entry_id].next_pass_id = commands.size();
    }

    const TechniqueGroup& getTechniqueGroup(int tech_id) {
        return technique_groups[tech_id];
    }
};