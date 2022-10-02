#pragma once

#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/render_bucket.hpp"
#include "config.hpp"

bool gpuInit(build_config::gpuPipelineCommon* pp);
void gpuCleanup();

build_config::gpuPipelineCommon* gpuGetPipeline();

//void gpuDrawRenderable(gpuRenderable* r);

//void gpuClearQueue();
void gpuDraw(gpuRenderBucket* bucket, gpuRenderTarget* target);