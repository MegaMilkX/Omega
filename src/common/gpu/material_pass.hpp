#pragma once

#include <vector>
#include <memory>
#include "math/gfxm.hpp"
#include "platform/gl/glextutil.h"
#include "util/strid.hpp"
#include "gpu_shader_program.hpp"
#include "gpu/common/shader_sampler_set.hpp"
#include "shader_interface.hpp"

#include "gpu/shader_set.hpp"
#include "resource_manager/resource_ref.hpp"



class gpuMaterial;
class gpuMaterialPass {
    friend gpuMaterial;

    pipe_pass_id_t pipeline_idx = -1;
    std::string path;
    std::vector<ResourceRef<gpuShaderSet>> shader_sets;

public:
    struct {
        uint8_t depth_test : 1;
        uint8_t stencil_test : 1;
        uint8_t cull_faces : 1;
        uint8_t depth_write : 1;
    };
    GPU_BLEND_MODE blend_mode = GPU_BLEND_MODE::BLEND;

    gpuMaterialPass(const char* path);

    SHADER_INTERFACE_GENERIC shaderInterface;

    pipe_pass_id_t getPipelineIdx() const;
    const std::string& getPath() const;

    void addShaderSet(ResourceRef<gpuShaderSet> shaders);
    int shaderSetCount() const;
    gpuShaderSet* getShaderSet(int i) const;

    template<typename UNIFORM>
    void setUniform(typename const UNIFORM::VALUE_T& value) {
        shaderInterface.setUniform<UNIFORM>(value);
    }
};

