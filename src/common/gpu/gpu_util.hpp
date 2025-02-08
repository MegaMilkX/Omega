#pragma once

#include "gpu_mesh_desc.hpp"
#include "gpu/common/shader_sampler_set.hpp"


bool gpuUtilInit();
void gpuUtilCleanup();

// NOTE: Basically glBindVertexArray() if there was an actual VAO
// keeping it this way for now
void gpuBindMeshBinding(const gpuMeshShaderBinding* b);
void gpuBindMeshBindingDirect(const gpuMeshShaderBinding* b);
void gpuDrawMeshBinding(const gpuMeshShaderBinding* b);
void gpuDrawMeshBindingInstanced(const gpuMeshShaderBinding* binding, int instance_count);

class gpuRenderTarget;
class gpuPass;
void gpuBindSamplers(gpuRenderTarget*, gpuPass*, const ShaderSamplerSet*);

void gpuDrawFullscreenTriangle();
void gpuDrawCubeMapCube();