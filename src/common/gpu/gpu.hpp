#pragma once

#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/gpu_asset_cache.hpp"
#include "config.hpp"

bool gpuInit();
void gpuCleanup();

build_config::gpuPipelineCommon* gpuGetPipeline();

gpuRenderTarget* gpuGetDefaultRenderTarget();


void gpuDraw(
    gpuRenderBucket* bucket, gpuRenderTarget* target,
    const DRAW_PARAMS& params
);
void gpuDrawLightmapSample(
    gpuMeshShaderBinding** bindings, int count, const DRAW_PARAMS& params
);
void gpuDrawFullscreenTriangle();
void gpuDrawCubeMapCube();
void gpuDrawTextureToDefaultFrameBuffer(gpuTexture2d* texture, gpuTexture2d* depth = 0, const gfxm::rect& rc_ratio = gfxm::rect(0,0,1,1));
void gpuDrawToDefaultFrameBuffer(gpuRenderTarget* target, const gfxm::rect& rc_ratio);
void gpuDrawTextureToFramebuffer(gpuTexture2d* texture, GLuint framebuffer, int* vp);

gpuAssetCache* gpuGetAssetCache();
