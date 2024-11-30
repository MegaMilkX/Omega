#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/ibl_maps.hpp"
#include "gpu/common_resources.hpp"


class EnvironmentIBLPass : public gpuPass {
    RHSHARED<gpuShaderProgram> prog_env_ibl;
    IBLMaps ibl_maps;
    // TODO:
    //GLuint ub_common;
public:
    EnvironmentIBLPass();
    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) override;
};