#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/ibl_maps.hpp"
#include "gpu/common_resources.hpp"


class EnvironmentSSRPass : public gpuPass {
    gpuShaderProgram* prog_env_ibl = 0;
    // TODO:
    //GLuint ub_common;
public:
    EnvironmentSSRPass();
    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override;
};