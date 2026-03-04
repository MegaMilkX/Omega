#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include "resource_manager/resource_ref.hpp"
#include "gpu/types.hpp"
#include "gpu/gpu_types.hpp"
#include "gpu/shader_set.hpp"


class gpuRenderablePass {
public:
    struct {
        uint8_t depth_test : 1;
        uint8_t stencil_test : 1;
        uint8_t cull_faces : 1;
        uint8_t depth_write : 1;
    };
    GPU_BLEND_MODE blend_mode = GPU_BLEND_MODE::BLEND;
    std::vector<ResourceRef<gpuShaderSet>> shader_sets;
    pipe_pass_id_t pass_id;

    gpuRenderablePass* addShaderSet(ResourceRef<gpuShaderSet> shaders) {
        shader_sets.push_back(shaders);
        return this;
    }
    int  shaderSetCount() const {
        return shader_sets.size();
    }
    gpuShaderSet* getShaderSet(int i) const {
        return const_cast<gpuShaderSet*>(shader_sets[i].get());
    }
};

