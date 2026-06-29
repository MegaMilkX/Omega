#pragma once

#include <vector>
#include <unordered_map>
#include "gpu/types.hpp"
#include "gpu/gpu_types.hpp"
#include "gpu/shader_set.hpp"


struct GPU_INTERMEDIATE_PASS_DESC {
    std::vector<const gpuCompiledShader*> shaders;
    std::vector<gpuShaderSet*> base_shaders;
    std::vector<gpuShaderSet*> extension_shaders;
    shader_flags_t shader_flags = 0; // TODO:
    int extended_by_material = 0x0;
    draw_flags_t draw_flags = 0;
    GPU_BLEND_MODE blend_mode;

    void clearBaseShaderSets() {
        base_shaders.clear();
    }
    void addBaseShaderSet(gpuShaderSet* shader_set) {
        base_shaders.push_back(shader_set);
    }
    void addExtensionShaderSet(gpuShaderSet* shader_set) {
        extension_shaders.push_back(shader_set);
    }
};
struct GPU_INTERMEDIATE_RENDERABLE_CONTEXT {
    std::unordered_map<int, GPU_INTERMEDIATE_PASS_DESC> pass_map;
    
    GPU_INTERMEDIATE_PASS_DESC* getOrCreatePass(pipe_pass_id_t id) {
        auto it = pass_map.find(id);
        if (it == pass_map.end()) {
            it = pass_map.insert(
                std::make_pair(id, GPU_INTERMEDIATE_PASS_DESC{})
            ).first;
        }
        return &it->second;
    }
};

