#pragma once

#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_renderable.hpp"

bool gpuInit(gpuPipeline* pp);
void gpuCleanup();

gpuPipeline* gpuGetPipeline();

void gpuDrawRenderable(gpuRenderable* r);

void gpuClearQueue();
void gpuDraw();