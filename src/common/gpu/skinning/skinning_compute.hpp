#pragma once

#include "math/gfxm.hpp"
#include "gpu/gpu_buffer.hpp"


struct SkinUpdateData {
    int vertex_count;
    const gfxm::mat4* pose_transforms;
    int pose_count;
    gpuBuffer* bufVerticesSource;
    gpuBuffer* bufNormalsSource;
    gpuBuffer* bufBoneIndices;
    gpuBuffer* bufBoneWeights;
    gpuBuffer* bufVerticesOut;
    gpuBuffer* bufNormalsOut;
};


void updateSkinVertexDataCompute(SkinUpdateData* skin_instances, int count);