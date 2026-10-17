#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/texture/cube_texture.hpp"
#include "gpu/ibl_maps.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "resource/resource.hpp"


class gpuSkyboxPass : public gpuPass {
public:
    gpuSkyboxPass();
    void onDraw(gpuPassInstance* inst, gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, int fb_idx, const DRAW_PARAMS& params) override;
};
