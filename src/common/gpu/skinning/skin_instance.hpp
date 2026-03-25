#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_mesh_desc.hpp"
#include "gpu/skinning/skinning_compute.hpp"

#include "m3d/m3d_model.hpp"
#include "skeleton/skeleton_instance.hpp"


struct gpuSkinInstance {
    std::vector<int> bone_indices; // TODO: m3dModel should store these instead of names, and this should be a pointer same as inv_bind_transforms
    const gfxm::mat4* inv_bind_transforms = nullptr;
    std::vector<gfxm::mat4> pose_transforms;
    gpuBuffer   vertexBuffer;
    gpuBuffer   normalBuffer;
    gpuBuffer   tangentBuffer;
    gpuBuffer   bitangentBuffer;
    gpuMeshDesc mesh_desc;
    gpuSkinTask* skin_task = nullptr;

    ~gpuSkinInstance();
    bool build(const m3dMesh* m3d_mesh, const SkeletonInstance* skl_inst);
    // Pose matrices are in WORLD space, not MODEL space
    // The result of a skinning operation gives a mesh in WORLD space
    // Therefore the TransformBlock used while rendering it must be IDENTITY
    void updatePose(SkeletonInstance* skl_inst);
};

