#pragma once

#include "common/render/gpu_pipeline.hpp"
#include "common/render/gpu_renderable.hpp"

bool gpuInit(gpuPipeline* pp);
void gpuCleanup();

gpuPipeline* gpuGetPipeline();

void gpuDrawRenderable(gpuRenderable* r);

void gpuClearQueue();
void gpuDraw();