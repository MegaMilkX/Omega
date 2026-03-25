#pragma once

#include "math/gfxm.hpp"
#include "gpu/gpu_buffer.hpp"


struct gpuSkinTask {
    int index = -1;
    int vertex_count = 0;
    const gfxm::mat4* pose_transforms = nullptr;
    int pose_count = 0;
    gpuBuffer* bufVerticesSource = nullptr;
    gpuBuffer* bufNormalsSource = nullptr;
    gpuBuffer* bufTangentsSource = nullptr;
    gpuBuffer* bufBitangentsSource = nullptr;
    gpuBuffer* bufBoneIndices = nullptr;
    gpuBuffer* bufBoneWeights = nullptr;
    gpuBuffer* bufVerticesOut = nullptr;
    gpuBuffer* bufNormalsOut = nullptr;
    gpuBuffer* bufTangentsOut = nullptr;
    gpuBuffer* bufBitangentsOut = nullptr;
    bool is_valid = false;
};

void gpuInitSkinning();
void gpuCleanupSkinning();

gpuSkinTask* gpuCreateSkinTask();
void         gpuDestroySkinTask(gpuSkinTask*);
void         gpuScheduleSkinTask(gpuSkinTask*);
void         gpuRunSkinTasks();
int          gpuGetSkinTaskExecCount();

void gpuUpdateSkinVertexDataCompute(gpuSkinTask* skin_instances, int count);