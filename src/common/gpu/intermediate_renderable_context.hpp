#pragma once

#include <vector>
#include <unordered_map>
#include "gpu/types.hpp"
#include "gpu/gpu_types.hpp"
#include "gpu/shader_set.hpp"
#include "gpu/common/shader_sampler_set.hpp"


struct GPU_INTERMEDIATE_PASS_DESC {
    std::vector<const gpuCompiledShader*> shaders;
    std::vector<gpuShaderSet*> base_shaders;
    gpuShaderSet* attrib_vertex_shaders = nullptr;
    gpuShaderSet* attrib_geometry_shaders = nullptr;
    gpuShaderSet* attrib_fragment_shaders = nullptr;
    gpuShaderSet* to_world_vertex_shaders = nullptr;
    gpuShaderSet* material_vertex_shaders = nullptr;
    gpuShaderSet* material_fragment_shaders = nullptr;
    const ShaderKey* material_shader_key = nullptr;

    draw_flags_t draw_flags = 0;
    GPU_BLEND_MODE blend_mode;

    // TODO: Use predefined slots (mesh desc, inst desc, material desc, all should know their sampler binding locations)
    // or not, idk
    // this is actually done at renderable compile time, so doesn't really matter?
    std::map<std::string, ShaderSamplerSet::Sampler> textures;

    void addTexture(const std::string& name, GLuint id, SHADER_SAMPLER_TYPE type) {
        auto& sampler = textures[name];
        sampler.pipe_channel_index = 0; // TODO: Figure out what's this for
        sampler.slot = 0; // Slot gets resolved during renderable's compilation
        sampler.source = SHADER_SAMPLER_SOURCE_GPU;
        sampler.texture_id = id;
        sampler.type = type;    
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

