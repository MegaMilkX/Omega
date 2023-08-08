#pragma once

#include "gpu_mesh_desc.hpp"


bool gpuUtilInit();
void gpuUtilCleanup();

// NOTE: Basically glBindVertexArray() if there was an actual VAO
// keeping it this way for now
void gpuBindMeshBinding(const gpuMeshShaderBinding* b);
void gpuDrawMeshBinding(const gpuMeshShaderBinding* b);
void gpuDrawMeshBindingInstanced(const gpuMeshShaderBinding* binding, int instance_count);

void gpuDrawFullscreenTriangle();
void gpuDrawCubeMapCube();