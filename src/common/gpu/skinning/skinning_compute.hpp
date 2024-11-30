#pragma once

#include "math/gfxm.hpp"
#include "gpu/gpu_buffer.hpp"


struct SkinUpdateData {
    int vertex_count;
    const gfxm::mat4* pose_transforms;
    int pose_count;
    gpuBuffer* bufVerticesSource;
    gpuBuffer* bufNormalsSource;
    gpuBuffer* bufTangentsSource;
    gpuBuffer* bufBitangentsSource;
    gpuBuffer* bufBoneIndices;
    gpuBuffer* bufBoneWeights;
    gpuBuffer* bufVerticesOut;
    gpuBuffer* bufNormalsOut;
    gpuBuffer* bufTangentsOut;
    gpuBuffer* bufBitangentsOut;
    bool is_valid;
};

void initSkinning();
void cleanupSkinning();
void updateSkinVertexDataCompute(SkinUpdateData* skin_instances, int count);