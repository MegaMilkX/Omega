#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/gpu_cube_map.hpp"
#include "gpu/ibl_maps.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "resource/resource.hpp"


class gpuSkyboxPass : public gpuPass {
    RHSHARED<gpuShaderProgram> prog_skybox;
    //RHSHARED<gpuCubeMap> sky_cube_map;

    IBLMaps ibl_maps;

public:
    gpuSkyboxPass();
    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) override;
};
