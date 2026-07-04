#pragma once

#include "gpu_mesh_desc.hpp"
#include "gpu/common/shader_sampler_set.hpp"


bool gpuUtilInit();
void gpuUtilCleanup();

class gpuRenderTarget;
class gpuPass;
void gpuBindSamplers(gpuRenderTarget*, gpuPass*, const ShaderSamplerSet*);

void gpuMakeDrawBuffersArray(gpuPass* pip_pass, GLuint programid, GLenum* draw_buffers, int max_count);

void gpuDrawFullscreenTriangle();
void gpuDrawCubeMapCube();